/***********************************************************************
COPYRIGHT (C) STMicroelectronics 2000

Source file name : atapitest.c

 Test Harness for the ATA/ATAPI driver.

************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define STTBX_INPUT

#include "stlite.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stack.h"
#include "stpio.h"
#include "stsys.h"
#if defined(ST_OS20)
#include "debug.h"
#else
#include <os21.h>
#include "os21debug.h"
#endif

#include "stos.h"

#include "sttbx.h"
#include "stboot.h"
#include "stevt.h"
#include "stcommon.h"
#include "statapi.h"

#if defined(ARCHITECTURE_ST20)
#if defined(ST_5514) || defined(ST_5516) || \
    defined(ST_5517) || defined(ST_5528)
#include "stcfg.h"
#endif
#endif

#if defined(ST_5525)
#include <machine/bsp/mmu.h>
#include <machine/bsp.h>
#endif

#if defined(ATAPI_FDMA)
#include "stfdma.h"
#elif defined(STATAPI_CRYPT) 
#include "stcrypt.h"
#endif

#if defined(ST_5512)
#include "stfpga.h"
#endif
#include "atapitest.h"

#if (!defined FDMA_INTERRUPT)
#define FDMA_INTERRUPT 38
#endif

#ifndef FDMA_INTERRUPT_NUMBER
#define FDMA_INTERRUPT_NUMBER FDMA_INTERRUPT
#endif


/* Test configuration information ------------------------------------------*/

#if defined(ATAPI_DEBUG)
/* Temporary variable; used to turn on and off some tracing in driver
 * (eg commands written to registers echoed, etc.)
 */
extern BOOL ATAPI_Trace;
extern BOOL ATAPI_Verbose;
#endif

/* We'll be reading 256-sectors - but allocating the extra space means
 * we can get the buffers on the required alignment for the DMA tests
 */
#if !defined(ATAPI_FDMA) && !defined(STATAPI_CRYPT)
static U8 BigBuffer1[257 * 512];
static U8 BigBuffer2[257 * 512];
#endif

#if defined(STATAPI_CRYPT) && !defined(ATAPI_FDMA)
#ifndef CRYPT_INTERRUPT_LEVEL
    #define CRYPT_INTERRUPT_LEVEL   11
#endif 
#endif

/* Other Private types/constants ------------------------------------------ */

#pragma ST_device(DU8)
typedef volatile U8 DU8;

#pragma ST_device(DU16)
typedef volatile U16 DU16;

#pragma ST_device(DU32)
typedef volatile U32 DU32;

/* We seem to need to reset the interconnect; see DoTheTests() for more
 * detail. Define these if they don't already exist.
 */
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
#if !defined(INTERCONNECT_BASE)
    #define INTERCONNECT_BASE                   0x20010000
#endif
#if !defined(INTERCONNECT_CONFIG_CONTROL_REG_D)
    #define INTERCONNECT_CONFIG_CONTROL_REG_D   0x0C
#endif
#if !defined(INTERCONNECT_REG_D_SOFT_RESET_HDDI)
    #define INTERCONNECT_REG_D_SOFT_RESET_HDDI  0x00040000
#endif
#endif

#define K   1024
#if (defined(ST_5508) || defined(ST_5518))
#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS      (volatile U32 *)             0x50000000
#endif
    #define     ATA_INTERRUPT_NUMBER                EXTERNAL_1_INTERRUPT
    #define     ATA_HRD_RST           (volatile U16 *)             0x50000000

#elif defined(ST_5512)

#if defined(STATAPI_OLD_FPGA)
    #define     ATA_INTERRUPT_NUMBER                EXTERNAL_0_INTERRUPT
    #define     ATA_BASE_ADDRESS      (volatile U32 *)             0x60000000
    #define     ATA_HRD_RST           (volatile U16 *)             0x60100000
#else
    #define     ATA_INTERRUPT_NUMBER                EXTERNAL_0_INTERRUPT
    #define     ATA_BASE_ADDRESS      (volatile U32 *)             0x70B00000
    #define     ATA_HRD_RST           (volatile U16 *)             0x70A00000
#endif

#elif defined(ST_5514)

#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS      (volatile U32 *)ST5514_HDDI_BASE_ADDRESS
#endif
#if !defined(ATA_INTERRUPT_NUMBER)
    #define     ATA_INTERRUPT_NUMBER    ST5514_HDDI_INTERRUPT
#endif
    #define     ATA_HRD_RST           (volatile U16 *)ATA_BASE_ADDRESS + 0x84

#elif defined(ST_5528)
#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS      (volatile U32 *)HDDI_BASE_ADDRESS
#endif
#if !defined(ATA_INTERRUPT_NUMBER)
    #define     ATA_INTERRUPT_NUMBER    HDDI_INTERRUPT
#endif
  #define     ATA_HRD_RST           (volatile U16 *)ATA_BASE_ADDRESS + 0x84

#elif defined(ST_7100) && defined (SATA_SUPPORTED)
#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS            ST7100_SATA_BASE_ADDRESS
#endif
#if !defined(ST7100_SATA_INTERRUPT)
    #define ST7100_SATA_INTERRUPT          OS21_INTERRUPT_SATA
#endif
    #define  ATA_INTERRUPT_NUMBER          ST7100_SATA_INTERRUPT
    #define     ATA_HRD_RST                  ATA_BASE_ADDRESS

#elif defined(ST_7100) || defined (ST_7109) && !defined (SATA_SUPPORTED)
#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS        0xA2800000
#endif
    #define     ATA_INTERRUPT_NUMBER    OS21_INTERRUPT_IRL_ENC_7
    #define     ATA_HRD_RST             ATA_BASE_ADDRESS

#define PERIPH_BASE                  0xB8000000
#define COMM_BASE                   (PERIPH_BASE + 0x00000000)
#define ILC_BASE                    (COMM_BASE + 0x00000000)

#define COMMS_ILC_EXT_PRIORITY0 0xA00
#define COMMS_ILC_EXT_PRIORITY1 0xA08
#define COMMS_ILC_EXT_PRIORITY2 0xA10
#define COMMS_ILC_EXT_PRIORITY3 0xA18

#define COMMS_ILC_EXT_MODE0 0xA04
#define COMMS_ILC_EXT_MODE1 0xA0C
#define COMMS_ILC_EXT_MODE2 0xA14
#define COMMS_ILC_EXT_MODE3 0xA1C

#define COMMS_ILC_WAKEUP_ACTIVE_LEVEL0      0x680
#define COMMS_ILC_SET_ENABLE2               0x508

#elif defined(ST_5525) && !defined (SATA_SUPPORTED)
#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS        0x0F800000
#endif
    #define     ATA_INTERRUPT_NUMBER       OS21_INTERRUPT_EXT_INT_IN0
    #define     ATA_HRD_RST                ATA_BASE_ADDRESS
    #define     FMICONFIG_BASE_ADDRESS     FMI_BASE_ADDRESS

#elif defined(ST_7109) && defined (SATA_SUPPORTED)
#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS        ST7109_SATA_BASE_ADDRESS
#endif
#if !defined(ST7109_SATA_INTERRUPT)
    #define ST7109_SATA_INTERRUPT       OS21_INTERRUPT_SATA
#endif
    #define  ATA_INTERRUPT_NUMBER       ST7109_SATA_INTERRUPT
    #define  ATA_HRD_RST                ATA_BASE_ADDRESS

#elif defined(ST_7200) && defined (SATA_SUPPORTED)
#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS        SATA_0_BASE_ADDRESS
#endif

#if !defined(ATA2_BASE_ADDRESS)
    #define     ATA2_BASE_ADDRESS       SATA_1_BASE_ADDRESS
#endif
#if !defined(ST7200_SATA_INTERRUPT)
    #define ST7200_SATA_INTERRUPT       OS21_INTERRUPT_SATA0_DMAC   /*IRQ_SATA0_DMAC*/
#endif
#if !defined(ST7200_SATA2_INTERRUPT)
    #define ST7200_SATA2_INTERRUPT      OS21_INTERRUPT_SATA1_DMAC   /*IRQ_SATA1_DMAC*/
#endif
    #define  ATA_INTERRUPT_NUMBER       ST7200_SATA_INTERRUPT
    #define  ATA_INTERRUPT_NUMBER2      ST7200_SATA2_INTERRUPT
    #define  ATA_HRD_RST                ATA_BASE_ADDRESS    

#elif defined(ST_8010)
#if !defined(ATA_BASE_ADDRESS)
    #define     ATA_BASE_ADDRESS      (volatile U32 *)ST8010_HDDI_BASE_ADDRESS
#endif
#if !defined(ATA_INTERRUPT_NUMBER)
    #define     ATA_INTERRUPT_NUMBER    ST8010_HDDI_INTERRUPT
#endif
    #define     ATA_HRD_RST           (volatile U16 *)ATA_BASE_ADDRESS + 0x84

#elif defined(mb390)
    #if !defined(ATA_BASE_ADDRESS)
        #define ATA_BASE_ADDRESS        0x40000000
    #endif
    #define ATA_INTERRUPT_NUMBER                    EXTERNAL_1_INTERRUPT
    #define ATA_HRD_RST                             ATA_BASE_ADDRESS
#elif defined(mb395)
    #if !defined(ATA_BASE_ADDRESS)
        #define ATA_BASE_ADDRESS        0x40000000
    #endif
    #define ATA_INTERRUPT_NUMBER                    EXTERNAL_0_INTERRUPT
    #define ATA_HRD_RST                             ATA_BASE_ADDRESS
#elif defined(ST_7710)
    #if !defined(ATA_BASE_ADDRESS)
        #define ATA_BASE_ADDRESS        0x43000000
    #endif
    #define ATA_INTERRUPT_NUMBER                    EXTERNAL_3_INTERRUPT
    #define ATA_HRD_RST                             ATA_BASE_ADDRESS
#elif defined(ST_5516) || defined(ST_5517)
    /* ATA_BASE_ADDRESS defined in board file */
    #define ATA_INTERRUPT_NUMBER                    EXTERNAL_1_INTERRUPT
    #define ATA_HRD_RST                             ATA_BASE_ADDRESS
#endif

#if defined(mb411) && defined(TEST_TWO_DEVICE)

#define PERIPH_BASE                  0xB8000000
#define COMM_BASE                   (PERIPH_BASE + 0x00000000)
#define ILC_BASE                    (COMM_BASE + 0x00000000)

#define COMMS_ILC_EXT_PRIORITY0 0xA00
#define COMMS_ILC_EXT_PRIORITY1 0xA08
#define COMMS_ILC_EXT_PRIORITY2 0xA10
#define COMMS_ILC_EXT_PRIORITY3 0xA18

#define COMMS_ILC_EXT_MODE0 0xA04
#define COMMS_ILC_EXT_MODE1 0xA0C
#define COMMS_ILC_EXT_MODE2 0xA14
#define COMMS_ILC_EXT_MODE3 0xA1C

#define COMMS_ILC_WAKEUP_ACTIVE_LEVEL0      0x680
#define COMMS_ILC_SET_ENABLE2               0x508

#endif/*TEST_TWO_DEVICE*/
#if defined(ST_5514)
    #define DMA_PRESENT
    #if !defined(ATA_INTERRUPT_LEVEL)
        #define ATA_INTERRUPT_LEVEL		5
    #endif
#endif
#if defined(ST_5528)
    #define DMA_PRESENT
    #if !defined(ATA_INTERRUPT_LEVEL)
        #define ATA_INTERRUPT_LEVEL		5
    #endif
#endif
#if defined(ST_8010)
    #define DMA_PRESENT
    #if !defined(ATA_INTERRUPT_LEVEL)
        #define ATA_INTERRUPT_LEVEL		5
    #endif
#endif
#if ((defined(ST_7100) || defined(ST_7109)) && (defined (SATA_SUPPORTED)))
    #define DMA_PRESENT
    #if !defined(ATA_INTERRUPT_LEVEL)
        #define ATA_INTERRUPT_LEVEL		5
    #endif
#endif

#if ((defined(ST_7200) && defined (SATA_SUPPORTED)))
    #define DMA_PRESENT
    #if !defined(ATA_INTERRUPT_LEVEL)
        #define ATA_INTERRUPT_LEVEL		5
    #endif
#endif

#if defined(ST_5525)
#if !defined(ATA_INTERRUPT_LEVEL)
        #define ATA_INTERRUPT_LEVEL		5
#endif
#endif

/* Makes things slightly neater later on */
#if !defined(ST_5512) && !defined(ST_5518)
#define CFG_REQUIRED
#endif

/* Sizes of partitions */
#if defined (ST_OS20)
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1500)
#elif defined (ST_OS21) /* OS21 */
  #if defined (ST_5528)
    #define INTERNAL_PARTITION_SIZE      (ST20_INTERNAL_MEMORY_SIZE-1500)
  #endif
#endif

#if defined (ST_7100) || defined (ST_7109) || defined(ST_5525) || defined(ST_7200)
#define SYSTEM_PARTITION_SIZE            4*K*K
#else
#define SYSTEM_PARTITION_SIZE            2*K*K
#endif

#if defined(ST_5514) && !defined(UNIFIED_MEMORY) && !defined(mb290)
#define TINY_PARTITION_SIZE              2*K*K  /* 5514 emi */
#elif defined(NCACHE_SIZE)
#define TINY_PARTITION_SIZE              NCACHE_SIZE
#elif defined (ST_5100)
#define TINY_PARTITION_SIZE              512*K
#elif defined (ST_7100) || defined (ST_7109)  || defined(ST_5525) || defined(ST_7200)
#define TINY_PARTITION_SIZE              4*K*K
#else
#define TINY_PARTITION_SIZE              512*K  /*default */
#endif

#define TEST_PARTITION_1       the_system_partition_p
#define TEST_PARTITION_2       the_internal_partition_p
#define TINY_PARTITION_1       the_tiny_partition_p

#define BLOCK_SIZE                  512
#define LBA48_SECTOR_COUNT          128
#define HDD_SECTOR_SIZE			    512
#define	DVD_SECTOR_SIZE		        2048
#define	CD_SECTOR_SIZE		        2352
#define	LSN_ADDRESS_OFFSET	        1
#define ATAPI_XFER_LENGTH_OFFSET    5
#define ITERATION_NUMBER            32
#define	REQ_SENSE_DATA_LENGTH		18
#define	SENSE_KEY_OFFSET			02
#define	ASC_OFFSET 					12
#define	ASCQ_OFFSET					13


/* Private variables ------------------------------------------------------ */
ATAPI_Address_t     AddressTable[36]={   /*    Ch   Cl   H     S  */
                                     {FALSE,{0x02,0x02,0x01,0x10}},  /* Sector variation*/
                                     {FALSE,{0x02,0x02,0x01,0x13}},
                                     {FALSE,{0x02,0x02,0x01,0x15}},
                                     {FALSE,{0x02,0x02,0x01,0x19}},
                                     {FALSE,{0x02,0x02,0x01,0x20}},
                                     {FALSE,{0x02,0x02,0x01,0x23}},
                                     {FALSE,{0x02,0x02,0x01,0x10}}, /* Head & Sector variation */
                                     {FALSE,{0x02,0x02,0x01,0x13}},
                                     {FALSE,{0x02,0x02,0x02,0x15}},
                                     {FALSE,{0x02,0x02,0x03,0x19}},
                                     {FALSE,{0x02,0x02,0x04,0x20}},
                                     {FALSE,{0x02,0x02,0x05,0x10}},
                                     {FALSE,{0x02,0x02,0x06,0x13}},
                                     {FALSE,{0x02,0x02,0x07,0x15}},
                                     {FALSE,{0x02,0x02,0x08,0x19}},
                                     {FALSE,{0x02,0x02,0x09,0x20}},
                                     {FALSE,{0x02,0x02,0x0A,0x23}},
                                     {FALSE,{0x02,0x02,0x0B,0x10}},
                                     {FALSE,{0x02,0x02,0x0B,0x13}},
                                     {FALSE,{0x02,0x02,0x0C,0x15}},
                                     {FALSE,{0x02,0x02,0x0D,0x19}},
                                     {FALSE,{0x02,0x02,0x0E,0x20}},
                                     {FALSE,{0x02,0x02,0x0E,0x19}},/* Cyl, Head & Sector variation*/
                                     {FALSE,{0x02,0x02,0x04,0x20}},
                                     {FALSE,{0x02,0x04,0x05,0x10}},
                                     {FALSE,{0x02,0x07,0x06,0x13}},
                                     {FALSE,{0x02,0x08,0x07,0x15}},
                                     {FALSE,{0x02,0x0C,0x08,0x19}},
                                     {FALSE,{0x02,0x0F,0x09,0x20}},
                                     {FALSE,{0x02,0x20,0x0A,0x23}},
                                     {FALSE,{0x02,0x10,0x0B,0x10}},
                                     {FALSE,{0x02,0x30,0x0B,0x13}},
                                     {FALSE,{0x02,0x40,0x0C,0x15}},
                                     {FALSE,{0x02,0x50,0x0D,0x19}},
                                     {FALSE,{0x02,0x60,0x0E,0x20}},
                                     {FALSE,{0x02,0x70,0x02,0x23}}
                                    };

#if defined(STATAPI_SET_EMIREGISTER_MAP)
/*PATA Regmasks*/
#define         STATAPI_NUM_REG_MASKS       12
#if defined (ST_5512)
U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={ (aCS1 | nCS0 | aDA2 | aDA1 | nDA0),
                                       (nCS1 | aCS0 | nDA2 | nDA1 | nDA0),
                                       (nCS1 | aCS0 | nDA2 | nDA1 | aDA0),
                                       (nCS1 | aCS0 | nDA2 | nDA1 | aDA0),
                                       (nCS1 | aCS0 | nDA2 | aDA1 | nDA0),
                                       (nCS1 | aCS0 | nDA2 | aDA1 | aDA0),
                                       (nCS1 | aCS0 | aDA2 | nDA1 | nDA0),
                                       (nCS1 | aCS0 | aDA2 | nDA1 | aDA0),
                                       (nCS1 | aCS0 | aDA2 | aDA1 | nDA0),
                                       (nCS1 | aCS0 | aDA2 | aDA1 | aDA0),
                                       (nCS1 | aCS0 | aDA2 | aDA1 | aDA0),
                                       (aCS1 | nCS0 | aDA2 | aDA1 | nDA0)
                                      };

#elif defined (ST_5508) || defined (ST_5518)
U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={(0x1c0000),
                                      (0x200000),
                                      (0x220000),
                                      (0x220000),
                                      (0x240000),
                                      (0x260000),
                                      (0x280000),
                                      (0x2a0000),
                                      (0x2c0000),
                                      (0x2e0000),
                                      (0x2e0000),
                                      (0x1c0000)
                                     };
#elif defined(mb361)
U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={(0x150000), /* ASR */
                                      (0x080000), /* Data */
                                      (0x0a0000), /* ERR */
                                      (0x0a0000), /* FEATURE */
                                      (0x090000), /* SC */
                                      (0x0b0000), /* SN */
                                      (0x0c0000), /* CL */
                                      (0x0e0000), /* CH */
                                      (0x0d0000), /* DH */
                                      (0x0f0000), /* STA */
                                      (0x0f0000), /* COMMAND */
                                      (0x150000)  /* CTL */
                                     };

#elif defined(mb382)
U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={(0x0e0000), /* ASR */
                                      (0x100000), /* Data */
                                      (0x110000), /* ERR */
                                      (0x110000), /* FEATURE */
                                      (0x120000), /* SC */
                                      (0x130000), /* SN */
                                      (0x140000), /* CL */
                                      (0x150000), /* CH */
                                      (0x160000), /* DH */
                                      (0x170000), /* STA */
                                      (0x170000), /* COMMAND */
                                      (0x0e0000)  /* CTL */
                                     };
#elif defined(mb390)
U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={(0x1c0000), /* ASR */
                                      (0x200000), /* Data */
                                      (0x220000), /* ERR */
                                      (0x220000), /* FEATURE */
                                      (0x240000), /* SC */
                                      (0x260000), /* SN */
                                      (0x280000), /* CL */
                                      (0x2a0000), /* CH */
                                      (0x2c0000), /* DH */
                                      (0x2e0000), /* STA */
                                      (0x2e0000), /* COMMAND */
                                      (0x1c0000)  /* CTL */
                                     };

#elif defined(mb391)
U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={(0x8c0000), /* ASR */
                                      (0x000000), /* Data */
                                      (0x020000), /* ERR */
                                      (0x020000), /* FEATURE */
                                      (0x040000), /* SC */
                                      (0x060000), /* SN */
                                      (0x080000), /* CL */
                                      (0x0a0000), /* CH */
                                      (0x0c0000), /* DH */
                                      (0x0e0000), /* STA */
                                      (0x0e0000), /* COMMAND */
                                      (0x8c0000)  /* CTL */
                                     };

#elif defined(mb395)
U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={(0x2c0000), /* ASR */
                                      (0x100000), /* Data */
                                      (0x120000), /* ERR */
                                      (0x120000), /* FEATURE */
                                      (0x140000), /* SC */
                                      (0x160000), /* SN */
                                      (0x180000), /* CL */
                                      (0x1a0000), /* CH */
                                      (0x1c0000), /* DH */
                                      (0x1e0000), /* STA */
                                      (0x1e0000), /* COMMAND */
                                      (0x2c0000)  /* CTL */
                                     };

#elif defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
/* Not actually masks in this case; offsets from base address for the
 * relevant registers
 */
U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={ (0x38),
                                       (0x40),
                                       (0x44),
                                       (0x44),
                                       (0x48),
                                       (0x4c),
                                       (0x50),
                                       (0x54),
                                       (0x58),
                                       (0x5c),
                                       (0x5c),
                                       (0x38)
                                     };
                                
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_5525)
#if !defined(SATA_SUPPORTED)

U32 PATARegMasks[STATAPI_NUM_REG_MASKS]={(0x1c0000), /* ASR */
                                      (0x200000), /* Data */
                                      (0x220000), /* ERR */
                                      (0x220000), /* FEATURE */
                                      (0x240000), /* SC */
                                      (0x260000), /* SN */
                                      (0x280000), /* CL */
                                      (0x2a0000), /* CH */
                                      (0x2c0000), /* DH */
                                      (0x2e0000), /* STA */
                                      (0x2e0000), /* COMMAND */
                                      (0x1c0000)  /* CTL */
                                     };
#endif/*SATA_SUPPORTED*/ 
#endif
#endif/* STATAPI_SET_EMIREGISTER_MAP*/                              

#define BUFFER_SIZES    6
    U32 BuffSizes[BUFFER_SIZES]={1*K,2*K,4*K,8*K,16*K,32*K};

#ifdef ST_OS21
#define STACK_SIZE  20 * 1024
#else
#define STACK_SIZE  4 * 1024
#endif

/* Test harness revision number */
static U8 Revision[] = STATAPI_REVISION;

/* Info to know what we have in the bus  */
static ATAPI_Drive_t  Drives[2];


/* Declarations for memory partitions */
#if !defined (ARCHITECTURE_ST200) && !defined (ARCHITECTURE_ST40)
static U8               internal_block [INTERNAL_PARTITION_SIZE];
#endif

#if defined (ST_OS20)
static partition_t      the_internal_partition;
static partition_t      *the_internal_partition_p = NULL;
#pragma ST_section      ( internal_block, "internal_section_noinit")
#endif

static U8               system_block [SYSTEM_PARTITION_SIZE];

#if defined (ST_OS20)
static partition_t      the_system_partition;
#endif
static partition_t      *the_system_partition_p = NULL;

#pragma ST_section      ( system_block,   "system_section_noinit")

static U8               tiny_block [TINY_PARTITION_SIZE];
#if defined (ST_OS20)
static partition_t      the_tiny_partition;
#endif
static partition_t      *the_tiny_partition_p = NULL;
#pragma ST_section      ( tiny_block,     "ncache_section")

/* These are to avoid linker warnings. I wish people would stop
 * inventing new sections.
 */
static unsigned char    internal_block_init[1];
#pragma ST_section      ( internal_block_init, "internal_section")

static unsigned char    system_block_init[1];
#pragma ST_section      ( system_block_init, "system_section")

static unsigned char    data_block[1];
#pragma ST_section      ( data_block, "data_section")

#if defined(ST_OS20)
/* Temporary, for compatibility with old drivers (EVT) */
partition_t     *system_partition = &the_system_partition;
partition_t     *internal_partition = &the_internal_partition;
#endif
semaphore_t		*StepSemaphore_p;
semaphore_t		*StepSemaphore2_p;

static U32 ClocksPerSec;

static    ST_DeviceName_t         AtapiDevName           = "ATAPI0";
ATAPI_DeviceInfo_t      HardInfo;

/* Event driver parameters */

static void ATAPI_Callback( STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p );
static    STEVT_DeviceSubscribeParams_t SubsParams={ ATAPI_Callback,
                                                     NULL };
                                                                                 
#if defined(TEST_TWO_DEVICE)
static    ST_DeviceName_t         AtapiDevName2          = "SATA";
static void ATAPI_Callback2( STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t SATAEvent,
                            const void *SATAEventData,
                            const void *SATASubscriberData_p );
static    STEVT_DeviceSubscribeParams_t SubsParams2={ ATAPI_Callback2,
                                                     NULL };                                
#endif                            
                            
static    STEVT_InitParams_t    EVTInitParams;
static    STEVT_TermParams_t    EVTTermParams;
static    ST_DeviceName_t       EVTDevName  = "EVT0";
static    STEVT_OpenParams_t    EVTOpenParams;
static    STEVT_Handle_t        EVTHandle;
#if defined(ATAPI_FDMA)
static    ST_DeviceName_t       FDMADevName = "FDMA";
#endif
                                                 
#if defined(ST_5518) || defined(ST_5508)
static    ST_DeviceName_t         PIODevName           = "PIO_0";
#endif

U8 MemTestDataBuffer[1024];

static U32 ClockFrequency = 0;

#if defined(DMA_PRESENT)
static BOOL IsUDMA = FALSE;
#endif

/* Set by ParseAndShow if drive indicates capability */
BOOL SMART_Capable = FALSE;
BOOL SMART_Enabled = FALSE;
BOOL LBA48_Capable = FALSE;
U32  MaxLBA     = 0;

/* Comes in 4 16-bit words... */
U32  MaxLBA48_1 = 0;
U32  MaxLBA48_2 = 0;
#if defined(TEST_TWO_DEVICE)
/* Type used for passing information between tasks and main application */
typedef struct
{
    ST_ErrorCode_t Error;
    task_t 		   *Task_p;
} ATAPI_TaskParams_t;
#endif

/* Private function prototypes -------------------------------------------- */

void DumpCmdStatus(STATAPI_CmdStatus_t CmdStatus);
static void DoTheTests (void);
static void ShowConfig(void);
static ST_ErrorCode_t SetTransferModePio(STATAPI_Handle_t Handle,
                                         STATAPI_PioMode_t PioMode);
#if defined(DMA_PRESENT)
static ST_ErrorCode_t ClearSectors(STATAPI_Handle_t Handle,
                                   ATAPI_Address_t *Addr,
                                   U8 BlockNo);
static ST_ErrorCode_t SetTransferModeDma(STATAPI_Handle_t Handle,
                                         STATAPI_DmaMode_t DmaMode);
#endif
#if defined(CFG_REQUIRED)
static ST_ErrorCode_t DoCfg(void);
#endif
#if defined(MEDIAREF)
static ST_ErrorCode_t ATAPI_UndoClkGen(void);
#endif
static U8 *AllocateAligned(partition_t *partition, U32 Size,
                           U32 Alignment, U8 **OriginalPtr);
#ifdef SOAK_TEST
static BOOL VerifyBuffer(U8 *Src, U8 *Dst, U32 Count);
#endif

/* Ancillary Test routines */
static void  ParseAndShow(ATAPI_DeviceInfo_t *Out,U8 *Data);
static ST_ErrorCode_t SeekTime(STATAPI_Handle_t Handle,U16 Iter,U32 *Acum);
static ST_ErrorCode_t WriteSeekTime(STATAPI_Handle_t Handle,U16 Iter,U32 *Acum);
static ST_ErrorCode_t ReadSeekTime(STATAPI_Handle_t Handle,U16 Iter,U32 *Acum);
static ST_ErrorCode_t ThrPutReadSimple(STATAPI_Handle_t Handle,U16 Iter,U32 *Acum);
static ST_ErrorCode_t ThrPutWriteSimple(STATAPI_Handle_t Handle,U16 Iter,U32 *Acum);
static ST_ErrorCode_t ThrPutWriteMultiple(STATAPI_Handle_t Handle,U8 Mode,U32 *Acum);
static ST_ErrorCode_t ThrPutReadMultiple(STATAPI_Handle_t Handle,U8 Mode,U32 *Acum);

#if defined(DMA_PRESENT)
static ST_ErrorCode_t ThrPutReadSimpleDma(STATAPI_Handle_t Handle,U16 Iter,U32 *Acum);
static ST_ErrorCode_t ThrPutReadMultipleDma(STATAPI_Handle_t Handle,U8 Mode,U32 *Acum);
static ST_ErrorCode_t ThrPutWriteMultipleDma(STATAPI_Handle_t Handle,U8 Mode,U32 *Acum);
#endif

/*ATAPI Functions*/
static ST_ErrorCode_t ControlTray ( STATAPI_Handle_t hATAPIDevHandle, U8 TrayOpen);
static ST_ErrorCode_t SimpleRTestDVD ( STATAPI_Handle_t hATAPIDevHandle, U32 StartSector, U32 NoOfSectors );
static ST_ErrorCode_t SimpleRTestCD (  STATAPI_Handle_t hATAPIDevHandle, U32 StartSector, U32 NoOfSectors );
static ST_ErrorCode_t RequestSense (STATAPI_Handle_t hATAPIDevHandle );
static ST_ErrorCode_t TestUnitReady (STATAPI_Handle_t hATAPIDevHandle);
static ST_ErrorCode_t PacketSeekTime(STATAPI_Handle_t hATAPIDevHandle,U16 Iter,U32 *Acum);
static ST_ErrorCode_t PacketReadSeekTime(STATAPI_Handle_t hATAPIDevHandle,U16 Iter,U32 *Acum,U32 *throughput);

ST_ErrorCode_t  SimpleRWTest(STATAPI_Handle_t Handle,
                             ATAPI_Address_t *Addr,
                             U8 BlockNo);
ST_ErrorCode_t  MultipleRWTest(STATAPI_Handle_t Handle,
                             ATAPI_Address_t *Addr,
                             U8 BlockNo, U8 Mode);
ST_ErrorCode_t  ReadTOCTest(STATAPI_Handle_t Handle);
ST_ErrorCode_t  DoSetGetTimingTests(STATAPI_Handle_t Handle);
#if defined(DMA_PRESENT)
ST_ErrorCode_t  SimpleRWTestDma(STATAPI_Handle_t Handle,
                                ATAPI_Address_t *Addr,
                                U8 BlockNo);
static ST_ErrorCode_t Rw256SectorsDma(STATAPI_Handle_t Handle,
                                      ATAPI_Address_t *Addr);
#endif

/* Test Routines */
static ATAPI_TestResult_t ATAPI_TestSimpleAPI(ATAPI_TestParams_t * TestParams);
static ATAPI_TestResult_t ATAPI_TestAPINormal(ATAPI_TestParams_t * TestParams);
#if defined(TEST_TWO_DEVICE)
static ATAPI_TestResult_t ATAPI_TestMultiInstance(ATAPI_TestParams_t * TestParams);
#endif
static ATAPI_TestResult_t ATAPI_TestIdentify(ATAPI_TestParams_t * TestParams);
static ATAPI_TestResult_t ATAPI_TestPerformance(ATAPI_TestParams_t * TestParams);
static ATAPI_TestResult_t ATAPI_TestPacket(ATAPI_TestParams_t * TestParams);
static ATAPI_TestResult_t ATAPI_LBA48Test(ATAPI_TestParams_t * TestParams);
static ATAPI_TestResult_t ATAPI_SMARTTest(ATAPI_TestParams_t * TestParams);

#if !defined(STATAPI_NO_PARAMETER_CHECK)
static ATAPI_TestResult_t ATAPI_TestAPIErrant(ATAPI_TestParams_t * TestParams);
#endif
#ifdef SOAK_TEST
static ATAPI_TestResult_t ATAPI_SoakTest(ATAPI_TestParams_t * TestParams);
#endif

static ATAPI_TestResult_t   ATAPI_MemoryTest(ATAPI_TestParams_t *TestParams);
void MemoryTest_Task (void *Data);
static ATAPI_TestResult_t ATAPI_RegTest(ATAPI_TestParams_t *TestParams);

/* ------------------------------------------------------------------------- */
/* Test table */
static ATAPI_TestEntry_t ATAPITestTable[] =
{
    {
        ATAPI_MemoryTest,
        "Test memory usage",
        1
    },
#if defined(TEST_TWO_DEVICE)
    {
        ATAPI_TestMultiInstance,
        "Test two devices in Parallel",
        1
    },
#endif  

    {
        ATAPI_TestIdentify,
        "Retrieve informaton about the device connected",
        1
    },
    {
        ATAPI_TestPacket,
        "Test the packet command interface (only ATAPI)",
        1
    },
   {
        ATAPI_TestSimpleAPI,
        "Basical API functionality test",
        1
    },
    {
        ATAPI_TestAPINormal,
        "Normal Use of the API",
        1
    },
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    {
        ATAPI_TestAPIErrant,
        "Errant use of the API",
        1
    },
#endif

    {
        ATAPI_LBA48Test,
        "Check LBA-48 access to disk",
        1
    },

#if !defined (ST_8010)
    {
        ATAPI_SMARTTest,
        "Check SMART commands",
        1
    },
#endif
    {
        ATAPI_RegTest,
        "Register test - change map",
        1
    },
    {
        ATAPI_TestPerformance,
        "Some basic performance tests",
        1
    },

#ifdef SOAK_TEST
    {
        ATAPI_SoakTest,
        "SOAK TEST:Check all sectors using (only ATA)",
        1
    },
#endif
    { 0, "", 0 }
};

/* Globally used function prototypes */
static void DisplayError( ST_ErrorCode_t ErrorCode );
static void DisplayErrorNew(ST_ErrorCode_t error);
static BOOL CompareData (const U8 *b1, const U8 *b2, U32 size);

ST_ErrorCode_t EvtError;
static BOOL     Quiet=TRUE;

STATAPI_Handle_t	        hATADevHandle = NULL,
						    hATAPIDevHandle = NULL;
U32 LastLBA,StartSector,NoOfSectors;

/***********************************************************************
   TEST HARNESS - A call to DoTheTests() is made from within
   main. This effectively isolates the 5512 init code within main().
   This allows easy reuse as your own custom tests can be added to
   this function without altering the setup code.
***********************************************************************/

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
/****************************************************************************
Name         : SwapString

Description  :
    Byte-swaps N words into a string; intended for use with
    info block on the STi5514 (since bytes come out byteswapped)

Parameters   :
    U16     *Info           Pointer to source block
    U8      *String         Pointer to destination buffer
    U32     LengthInWords   How many words to swap

Return Value :
    void
 ****************************************************************************/
static void SwapString(U16 *Info, U8 *String, U32 LengthInWords)
{
    U32 i, j;

    for (i = 0, j = 0; i < LengthInWords; i++)
    {
        String[j++] = (Info[i] >> 8) & 0xff;
        String[j++] = Info[i] & 0xff;
    }
    String[j] = 0;
}
#endif

int main (int argc, char *argv[])
{
    ST_ErrorCode_t        Code;
    ST_DeviceName_t       Name = "DEV0";
    STBOOT_InitParams_t   InitParams;
    STBOOT_TermParams_t   TermParams;

#if defined(ST_5525)
    int result;
#endif

#if defined(DISABLE_DCACHE)
    STBOOT_DCache_Area_t Cache[] =
    {
        { NULL, NULL }
    };
#elif defined(ST_8010)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x80400000, (U32 *)0x807FFFFF },
        { NULL, NULL }
    };

#elif defined(ST_5514) && defined(UNIFIED_MEMORY)
    /* Constants not defined, fallbacks from here on down */
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0xC0380000, (U32 *)0xc03fffff },
        { NULL, NULL}
    };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x40200000, (U32 *)0x407FFFFF },
        { NULL, NULL }
    };
#elif defined(ST_5512)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x40080000, (U32 *)0x5FFFFFFF },
        { NULL, NULL }
    };

#elif defined (ST_7100) || defined (ST_7109)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x40080000, (U32 *)0x5FFFFFFF },
        { NULL, NULL }
    };

#elif defined(ST_5525)
    STBOOT_DCache_Area_t Cache[] =
    {
       { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
       { NULL, NULL }
    };

#elif defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t Cache[] =
    {
        /* assumed ok */
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS },
        { NULL, NULL }
    };
#else
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x40080000, (U32 *)0x4FFFFFFF },
        { NULL, NULL }
    };
#endif

    ST_ClockInfo_t        ClockInfo;
    STTBX_InitParams_t    TBX_InitParams;

    /* Try and catch any uninit. variable problems */
    memset(system_block, 0x55, sizeof(system_block));
    memset(tiny_block, 0x55, sizeof(tiny_block));

#if defined(ST_OS20)
    the_internal_partition_p = &the_internal_partition;
    the_system_partition_p = &the_system_partition;
    the_tiny_partition_p = &the_tiny_partition;
    partition_init_heap (the_internal_partition_p, internal_block,
                         sizeof(internal_block));
    partition_init_heap (the_system_partition_p,   system_block,
                         sizeof(system_block));
    partition_init_heap (the_tiny_partition_p,   tiny_block,
                         sizeof(tiny_block));
#else   /* ST_OS21 */
    the_system_partition_p = partition_create_heap(
                                (void *)system_block,
                                sizeof(system_block));

    if (NULL == the_system_partition_p)
    {
        printf("Failed to create system partition!\n");
        return (1);
    }

#if defined (ARCHITECTURE_ST200)

    the_tiny_partition_p = partition_create_heap(
                                (void *)(tiny_block ),
                                sizeof(tiny_block));

#else

    the_tiny_partition_p = partition_create_heap(
                                (void *)ST40_NOCACHE_NOTRANSLATE(tiny_block),
                                 sizeof(tiny_block));
#endif
    if (NULL == the_tiny_partition_p)
    {
        printf("Failed to create tiny partition!\n");
        return (1);
    }
#endif

    /* Avoids a compiler warning */
    internal_block_init[0] = 0;

    system_block_init[0] = 0;
    data_block[0] = 0;

#if defined(mb390) || defined (mb395)
    printf("Using EPLD version: 0x%02x\n", *(U8 *)0x41000000);
#endif

#if defined(DISABLE_ICACHE)
    InitParams.ICacheEnabled             = FALSE;
#else
    InitParams.ICacheEnabled             = TRUE;
#endif
#if !defined (DISABLE_DCACHE)
    InitParams.DCacheMap                 = Cache;
#else
     InitParams.DCacheMap                = NULL;
#ifdef ST_OS21
    InitParams.DCacheEnabled             = FALSE;
#endif
#endif
    InitParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    InitParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    InitParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    InitParams.CacheBaseAddress          = (U32*) CACHE_BASE_ADDRESS;
    InitParams.MemorySize                = SDRAM_SIZE;
    InitParams.SDRAMFrequency            = SDRAM_FREQUENCY;

#if defined(ST_5525)

    result=(int) bsp_mmu_memory_map((void*)0x0F800000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)ATA_BASE_ADDRESS);


    if (result == BSP_FAILURE)
    {
    	printf(" ATA mapping \n");
    }

    result=(int) bsp_mmu_memory_map((void*)0x0FFFF000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)FMICONFIG_BASE_ADDRESS);

    if ( result== BSP_FAILURE )
    {
    	printf(" FMI Config registers on FMI Bank 3\n");
    }

#endif

    if ((Code = STBOOT_Init (Name, &InitParams)) != ST_NO_ERROR)
    {
        printf("ERROR: STBOOT_Init returned code %d\n", Code);
    }
    else
    {
        TBX_InitParams.CPUPartition_p      = TEST_PARTITION_1;
        TBX_InitParams.SupportedDevices    = STTBX_DEVICE_DCU;
        TBX_InitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
        TBX_InitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
        if ((Code = STTBX_Init( "TBX0", &TBX_InitParams )) != ST_NO_ERROR )
        {
            printf("Error: STTBX_Init = 0x%08X\n", Code);
        }
        else
        {
            ClocksPerSec = ST_GetClocksPerSecond();
            ST_GetClockInfo(&ClockInfo);
            ClockFrequency = ClockInfo.HDDI;
            if (ClockFrequency == 0)
                ClockFrequency = ClockInfo.CommsBlock;
   
#if defined(CFG_REQUIRED)
            /* Need this to Reset HDDI */
            if (DoCfg() != ST_NO_ERROR)
            {
                return 1;
            }
#endif
        
             DoTheTests();
        }

        STBOOT_Term (Name, &TermParams);
    }
    return 0;
}

#if defined(CFG_REQUIRED)
/* Error reporting not quite ideal (it or's all the errors together),
 * but if one fails the rest are likely to anyway...
 */
static ST_ErrorCode_t DoCfg(void)
{
    ST_ErrorCode_t      error = ST_NO_ERROR;

#if defined(ARCHITECTURE_ST20)

#if !defined(mb390) && !defined(mb391) && !defined(mb395)
    STCFG_InitParams_t  CfgInitParams;
    STCFG_TermParams_t  CfgTermParams;
    STCFG_Handle_t      CfgHandle;
    ST_DeviceName_t     Name = "DEV0";

    CfgInitParams.BaseAddress_p = (U32 *)CFG_BASE_ADDRESS;
#if defined(ST_5514)
    CfgInitParams.DeviceType = STCFG_DEVICE_5514;
#elif defined(ST_5516)
    CfgInitParams.DeviceType = STCFG_DEVICE_5516;
#elif defined(ST_5517)
    CfgInitParams.DeviceType = STCFG_DEVICE_5517;
#elif defined(ST_5528)
    CfgInitParams.DeviceType = STCFG_DEVICE_5528;
#endif
    CfgInitParams.Partition_p = TEST_PARTITION_1;
    error |= STCFG_Init(Name, &CfgInitParams);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error initialising STCFG\n"));
        return error;
    }

    error |= STCFG_Open(Name, NULL, &CfgHandle);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error opening STCFG\n"));
    }

#if defined(ST_5514) || defined(ST_5528)
    error |= STCFG_CallCmd(CfgHandle, STCFG_CMD_HDDI_ENABLE, STCFG_HDDI_RESET);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error enabling HDDI reset\n"));
    }
    /* Wait 1ms */
    task_delay(ClocksPerSec / 1000);
    error |= STCFG_CallCmd(CfgHandle, STCFG_CMD_HDDI_DISABLE, STCFG_HDDI_RESET);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error disabling HDDI reset\n"));
    }
#elif defined(ST_5516) || defined(ST_5517)
    error |= STCFG_CallCmd(CfgHandle, STCFG_CMD_HDDI_ENABLE);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error enabling HDDI\n"));
    }
    error |= STCFG_CallCmd(CfgHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                          STCFG_EXTINT_1, STCFG_EXTINT_INPUT);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error enabling interrupt\n"));
    }
#endif

    STCFG_Close(CfgHandle);

    CfgTermParams.ForceTerminate = TRUE;
    error |= STCFG_Term(Name, &CfgTermParams);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error terminating STCFG driver\n"));
    }
#elif defined(mb390) || defined(mb395)
    /* Ugly, but easy... */
    #define FMICONFIG_BASE_ADDRESS         0x20200000
    #define FMI_GENCFG                     (FMICONFIG_BASE_ADDRESS + 0x28)
    #define FMICONFIGDATA0_BANK0           (FMICONFIG_BASE_ADDRESS + 0x100)
    #define FMICONFIGDATA1_BANK0           (FMICONFIG_BASE_ADDRESS + 0x108)
    #define FMICONFIGDATA2_BANK0           (FMICONFIG_BASE_ADDRESS + 0x110)
    #define FMICONFIGDATA3_BANK0           (FMICONFIG_BASE_ADDRESS + 0x118)
    volatile U32 *regs;
    volatile U8 *regs_8;

    /* Enable HDDI on FMI is done by an EPLD register (EPLD in Bank 1) */
    regs_8 = (volatile U8 *)0x41a00000;
    *regs_8 |= 0x01;

    /* Configuring ATAPI is done using FMI_GENCFG bits 30 (bank0) and 31
     * (bank1). Due to a bug on Cut1.0 of 5100, bit 31 actually
     * configures both. We'll write both 30 and 31 anyway, so we don't
     * have to care which cut we're on.
     */
    regs = (volatile U32 *)FMI_GENCFG;
    *regs = 0xc0000000;

    /* 3- You need to update the ATAPI registers and Interrupt number
     * (as in stbgr-prj-vali5100\boot\interrupt.c and
     * stbgr-prj-vali5100\include\sti5100\sti5100.h) as i did it on 5100
     * validation VOB. */
    /* These are faster values than those found in mb390_mem.cfg */

    /* Configure bank0 for ATAPI */
    regs = (volatile U32 *)FMICONFIGDATA0_BANK0;
    *regs = 0x010f8791;
    regs = (volatile U32 *)FMICONFIGDATA1_BANK0;
    *regs = 0x9f028821;
    regs = (volatile U32 *)FMICONFIGDATA2_BANK0;
    *regs = 0x9f028821;
    regs = (volatile U32 *)FMICONFIGDATA3_BANK0;
    *regs = 0x0000000a;

#elif defined(mb391)
    #define EMI_GENCFG                       0x20102028
    #define EMI_BANK3_DATA0                 (EMI_BASE_ADDRESS + 0x1c0)
    #define EMI_BANK3_DATA1                 (EMI_BASE_ADDRESS + 0x1c8)
    #define EMI_BANK3_DATA2                 (EMI_BASE_ADDRESS + 0x1d0)
    #define EMI_BANK3_DATA3                 (EMI_BASE_ADDRESS + 0x1d8)

    volatile U8 *regs_8;
    U32 data = 0x00;

    /* Enable HDDI on FMI is done by an EPLD register */
    regs_8 = (volatile U8 *)EPLD_BASE_ADDRESS + EPLD_ATAPI_OFFSET;
    *regs_8 |= 0x01;
    
    data = STSYS_ReadRegDev32LE(((U32 *)EMI_GENCFG ));
    STSYS_WriteRegDev32LE(((U32 *)EMI_GENCFG), (data | 0x00000018));

  /* Optimised PIO Mode 4 timing */
   *(volatile U32 *)EMI_BANK3_DATA0 = 0x00118791;
   *(volatile U32 *)EMI_BANK3_DATA1 = 0x8d001100;
   *(volatile U32 *)EMI_BANK3_DATA2 = 0x8d001100;
   *(volatile U32 *)EMI_BANK3_DATA3 = 0x0000000a;

#else
#error Unknown board - cannot configure hard disk.
#endif

#elif defined(mb428) && !defined(SATA_SUPPORTED)/*ARCHITECTURE ST200*/

#define FMI_GENCFG                 (FMICONFIG_BASE_ADDRESS + 0x028)
#define FMICONFIGDATA0_BANK3       (FMICONFIG_BASE_ADDRESS + 0x1C0)
#define FMICONFIGDATA1_BANK3       (FMICONFIG_BASE_ADDRESS + 0x1C8)
#define FMICONFIGDATA2_BANK3       (FMICONFIG_BASE_ADDRESS + 0x1D0)
#define FMICONFIGDATA3_BANK3       (FMICONFIG_BASE_ADDRESS + 0x1D8)
#define FMIBUFF_BANK3_TOP    	   (FMICONFIG_BASE_ADDRESS + 0x830)

    U32 data = 0x00;

/* Optimised PIO Mode 4 timing on Bank 3 */
   *(volatile U32 *)FMICONFIGDATA0_BANK3 = 0x010f8791;
   *(volatile U32 *)FMICONFIGDATA1_BANK3 = 0xbe028f21;
   *(volatile U32 *)FMICONFIGDATA2_BANK3 = 0xbe028f21;
   *(volatile U32 *)FMICONFIGDATA3_BANK3 = 0x0000000a;

    /* Write EMI_GEN_CFG */
    data = STSYS_ReadRegDev32LE(((U32 *)FMI_GENCFG ));
    STSYS_WriteRegDev32LE(((U32 *)FMI_GENCFG), (data | 0x80000000));

#else  /* ARCHITECTURE_ST40 */

#if (defined (mb411) && !defined (SATA_SUPPORTED)) |\
(defined(mb411) && defined(TEST_TWO_DEVICE))

#define FMICONFIG_BASE_ADDRESS         0xBA100000
#define FMI_GENCFG                 (FMICONFIG_BASE_ADDRESS + 0x28)
#define FMICONFIGDATA0_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1C0)
#define FMICONFIGDATA1_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1C8)
#define FMICONFIGDATA2_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1D0)
#define FMICONFIGDATA3_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1D8)

    volatile U8 *regs_8;
    U32 config = 0x00;

    /* Write EMI_GEN_CFG */
    STSYS_WriteRegDev32LE(((U32 *)FMI_GENCFG), 0x10);
    config = STSYS_ReadRegDev32LE(((U32 *)FMI_GENCFG));
    /*printf("EMI_GEN_CFG Programmed to %x", config);*/

   /* Optimised PIO Mode 4 timing on Bank 3 */
   *(volatile U32 *)FMICONFIGDATA0_BANK0 = 0x00021791;
   *(volatile U32 *)FMICONFIGDATA1_BANK0 = 0x08004141;
   *(volatile U32 *)FMICONFIGDATA2_BANK0 = 0x08004141;
   *(volatile U32 *)FMICONFIGDATA3_BANK0 = 0x00000000;

    /* Enable HDD on FMI is done by an EPLD register */
    regs_8 = (volatile U8 *)0xA3900000;
    *regs_8 |= 0x01;

    while (*regs_8 != 0x01)
        *regs_8=0x01;
#if defined (ATAPI_USING_INTERRUPTS)
    /* Enable ATAPI Interrupt - Set bit 7 of ITM 0 */
    *(DU8 *)0xA3100000 = 0x80;
#endif

  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_EXT_PRIORITY0,0x8004);
  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_EXT_PRIORITY1,0x8005);
  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_EXT_PRIORITY2,0x8006);
  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_EXT_PRIORITY3,0x8007);

  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_EXT_MODE0,0x1);
  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_EXT_MODE1,0x1);
  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_EXT_MODE2,0x1);
  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_EXT_MODE3,0x1);

  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_WAKEUP_ACTIVE_LEVEL0,0xF);
  STSYS_WriteRegDev32LE(ILC_BASE + COMMS_ILC_SET_ENABLE2,0xF);

#else

    #define HDDI_CONFIG     14
    #define HDDI_RESET_BIT  (1 << 0)
    U32 config;

    /*
    ** Reset HDDI
    */
    config = STSYS_ReadRegDev32LE(((U32 *)CFG_BASE_ADDRESS + HDDI_CONFIG));
    /* Note reset is active low on 5528 */
    config &= ~HDDI_RESET_BIT;
    STSYS_WriteRegDev32LE(((U32 *)CFG_BASE_ADDRESS + HDDI_CONFIG), config);

    /* Wait 1ms */
    task_delay(ClocksPerSec / 1000);

    config = STSYS_ReadRegDev32LE(((U32 *)CFG_BASE_ADDRESS + HDDI_CONFIG));
    /* Note reset is active low on 5528 */
    config |= HDDI_RESET_BIT;
    STSYS_WriteRegDev32LE(((U32 *)CFG_BASE_ADDRESS + HDDI_CONFIG), config);

#endif
#endif
    return error;
}
#endif

/*---------------------------------------------------------------------------
   Other utility function for displaying error codes
 ---------------------------------------------------------------------------*/
static void DisplayError( ST_ErrorCode_t ErrorCode )
{
    switch( ErrorCode )
    {
        case ST_NO_ERROR:
            STTBX_Print(("ST_NO_ERROR "));
            break;
        case ST_ERROR_BAD_PARAMETER:
            STTBX_Print(("ST_ERROR_BAD_PARAMETER "));
            break;
        case ST_ERROR_ALREADY_INITIALIZED:
            STTBX_Print(("ST_ERROR_ALREADY_INITIALIZED "));
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            STTBX_Print(("ST_ERROR_UNKNOWN_DEVICE "));
            break;
        case ST_ERROR_INVALID_HANDLE:
            STTBX_Print(("ST_ERROR_INVALID_HANDLE "));
            break;
        case ST_ERROR_OPEN_HANDLE:
            STTBX_Print(("ST_ERROR_OPEN_HANDLE "));
            break;
        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            STTBX_Print(("ST_ERROR_FEATURE_NOT_SUPPORTED "));
            break;
        case ST_ERROR_INTERRUPT_INSTALL:
            STTBX_Print(("ST_ERROR_INTERRUPT_INSTALL "));
            break;
        case ST_ERROR_INTERRUPT_UNINSTALL:
            STTBX_Print(("ST_ERROR_INTERRUPT_UNINSTALL "));
            break;
        case ST_ERROR_NO_MEMORY:
            STTBX_Print(("ST_ERROR_NO_MEMORY "));
            break;
        case ST_ERROR_NO_FREE_HANDLES:
            STTBX_Print(("ST_ERROR_NO_FREE_HANDLES "));
            break;
        case ST_ERROR_TIMEOUT:
            STTBX_Print(("ST_ERROR_TIMEOUT "));
            break;
        case ST_ERROR_DEVICE_BUSY:
            STTBX_Print(("ST_ERROR_DEVICE_BUSY "));
            break;
        case STATAPI_ERROR_CMD_ABORT:
          STTBX_Print(("STATAPI_ERROR_CMD_ABORT "));
          break;
        case STATAPI_ERROR_CMD_ERROR:
          STTBX_Print(("STATAPI_ERROR_CMD_ERROR "));
          break;
        case STATAPI_ERROR_DEVICE_NOT_PRESENT:
          STTBX_Print(("STATAPI_ERROR_DEVICE_NOT_PRESENT "));
          break;
        case STATAPI_ERROR_PKT_CHECK:
          STTBX_Print(("STATAPI_ERROR_PKT_CHECK "));
          break;
        case STATAPI_ERROR_PKT_NOT_SUPPORTED:
          STTBX_Print(("STATAPI_ERROR_PKT_NOT_SUPPORTED "));
          break;
        case STATAPI_ERROR_USER_ABORT:
          STTBX_Print(("STATAPI_ERROR_USER_ABORT "));
          break;
        case STATAPI_ERROR_TERMINATE_BAT:
          STTBX_Print(("STATAPI_ERROR_TERMINATE_BAT "));
          break;
        case STATAPI_ERROR_DIAGNOSTICS_FAIL:
          STTBX_Print(("STATAPI_ERROR_DIAGNOSTICS_FAIL "));
          break;
        case STATAPI_ERROR_MODE_NOT_SET:
          STTBX_Print(("STATAPI_ERROR_MODE_NOT_SET "));
          break;
        case STATAPI_ERROR_PROTOCOL_NOT_SUPPORTED:
          STTBX_Print(("STATAPI_ERROR_PROTOCOL_NOT_SUPPORTED "));
          break;
        case STATAPI_ERROR_CRC_ERROR:
          STTBX_Print(("STATAPI_ERROR_CRC_ERROR "));
          break;
#if defined(ATAPI_FDMA)
        case STFDMA_ERROR_NOT_INITIALIZED:
            STTBX_Print(("STFDMA_ERROR_NOT_INITIALIZED "));
            break;
        case STFDMA_ERROR_DEVICE_NOT_SUPPORTED:
            STTBX_Print(("STFDMA_ERROR_DEVICE_NOT_SUPPORTED "));
            break;
        case STFDMA_ERROR_NO_CALLBACK_TASK:
            STTBX_Print(("STFDMA_ERROR_NO_CALLBACK_TASK "));
            break;
        case STFDMA_ERROR_BLOCKING_TIMEOUT:
            STTBX_Print(("STFDMA_ERROR_BLOCKING_TIMEOUT "));
            break;
        case STFDMA_ERROR_CHANNEL_BUSY:
            STTBX_Print(("STFDMA_ERROR_CHANNEL_BUSY "));
            break;
        case STFDMA_ERROR_NO_FREE_CHANNELS:
            STTBX_Print(("STFDMA_ERROR_NO_FREE_CHANNELS "));
            break;
        case STFDMA_ERROR_ALL_CHANNELS_LOCKED:
            STTBX_Print(("STFDMA_ERROR_ALL_CHANNELS_LOCKED "));
            break;
        case STFDMA_ERROR_CHANNEL_NOT_LOCKED:
            STTBX_Print(("STFDMA_ERROR_CHANNEL_NOT_LOCKED "));
            break;
        case STFDMA_ERROR_UNKNOWN_CHANNEL_ID:
            STTBX_Print(("STFDMA_ERROR_UNKNOWN_CHANNEL_ID "));
            break;
        case STFDMA_ERROR_INVALID_TRANSFER_ID:
            STTBX_Print(("STFDMA_ERROR_INVALID_TRANSFER_ID "));
            break;
        case STFDMA_ERROR_TRANSFER_ABORTED:
            STTBX_Print(("STFDMA_ERROR_TRANSFER_ABORTED "));
            break;
        case STFDMA_ERROR_TRANSFER_IN_PROGRESS:
            STTBX_Print(("STFDMA_ERROR_TRANSFER_IN_PROGRESS "));
            break;
#endif
        default:
          STTBX_Print(("unknown error code %u ", ErrorCode ));
          break;
      }
}

static void DisplayErrorNew(ST_ErrorCode_t error)
{
    DisplayError(error);
    STTBX_Print(("\n"));
}

/*---------------------------------------------------------------------------*/
/* Other utility functions for checking various things.                      */
/*---------------------------------------------------------------------------*/

static BOOL CompareData (const U8 *b1, const U8 *b2, U32 size)
{
    if (memcmp (b1, b2, size) == 0)
    {
        STTBX_Print(("Data read matches data written!\n"));
        return TRUE;
    }
    else
    {
        STTBX_Print(("ERROR: data read does NOT match data written\n"));
        {
            U32 i, x, y;

            /* Find out first different character */
            for (i = 0; i < size; i++)
                if (b1[i] != b2[i])
                    break;
          /* then go back to nearest 16-byte boundary */
            i = i / 16;
            for (x = i; x < (i + 4); x++)
            {
                y = 0;
                STTBX_Print(("%04x   ", (x * 16) + y));
                for (y = 0; y < 16; y++)
                {
                    STTBX_Print(("%02x ", b1[(x * 16) + y]));
                }
                STTBX_Print(("\n"));
            }
            STTBX_Print(("\n\n"));
            for (x = i; x < (i + 4); x++)
            {
                y = 0;
                STTBX_Print(("%04x   ", (x * 16) + y));
                for (y = 0; y < 16; y++)
                {
                    STTBX_Print(("%02x ", b2[(x * 16) + y]));
                }
                STTBX_Print(("\n"));
            }
        }
        return FALSE;
    }
}

static BOOL CheckCode (ST_ErrorCode_t actualCode, ST_ErrorCode_t expectedCode)
{
    if (actualCode == expectedCode)
    {
        STTBX_Print((" Got expected code:  "));
        DisplayError( expectedCode );
        STTBX_Print(( "\n" ));
        return FALSE;
    }
    else
    {
        STTBX_Print(("ERROR: got code "));
        DisplayError( actualCode );
        STTBX_Print(("expected code "));
        DisplayError( expectedCode );
        STTBX_Print(( "\n" ));
        return TRUE;
    }
}

static BOOL CheckCodeOk (ST_ErrorCode_t actualCode)
{
    return CheckCode (actualCode, ST_NO_ERROR);
}

#if defined(DMA_PRESENT)
static ST_ErrorCode_t SetTransferModeDma(STATAPI_Handle_t Handle,
                                         STATAPI_DmaMode_t DmaMode)
{
    STATAPI_Cmd_t Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    ST_ErrorCode_t Error;

    Cmd.Features = 3;
    Cmd.UseLBA = FALSE;
    Cmd.Sector = 0;
    Cmd.CylinderLow =  0;
    Cmd.CylinderHigh = 0;
    Cmd.Head = 0;
    Cmd.CmdCode = STATAPI_CMD_SET_FEATURES;

    if (DmaMode <= STATAPI_DMA_MWDMA_MODE_2)
    {
        IsUDMA = FALSE;
        Cmd.SectorCount = 0x20 + (DmaMode - STATAPI_DMA_MWDMA_MODE_2);
    }
    else
    {
        IsUDMA = TRUE;
        Cmd.SectorCount = 0x40 + (DmaMode - STATAPI_DMA_UDMA_MODE_0);
    }

    STTBX_Print(("Setting transfer mode... "));
    Error = STATAPI_CmdNoData(Handle, &Cmd, &CmdStatus);
    if (Error == ST_NO_ERROR)
    {
        STOS_SemaphoreWait(StepSemaphore_p);
        if ((CmdStatus.Error != ST_NO_ERROR) ||
            (EvtError != ST_NO_ERROR))
        {
            STTBX_Print(("\nError while setting features: "));
            DisplayErrorNew(CmdStatus.Error);
            DisplayErrorNew(EvtError);
        }
        else
        {
            STTBX_Print(("okay.\n"));
        }
    }
    else
    {
        STTBX_Print(("\nCouldn't set features: "));
        DisplayErrorNew(Error);
    }

    return Error;
}
#endif  /* defined(DMA_PRESENT) */

static ST_ErrorCode_t SetTransferModePio(STATAPI_Handle_t Handle,
                                         STATAPI_PioMode_t PioMode)
{
    STATAPI_Cmd_t Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    ST_ErrorCode_t Error;

    /* XXX ?!?! */
    /* return 0; */

    Cmd.Features = 3;
    Cmd.UseLBA = FALSE;
    Cmd.Sector = 0;
    Cmd.CylinderLow =  0;
    Cmd.CylinderHigh = 0;
    Cmd.Head = 0;
    Cmd.CmdCode = STATAPI_CMD_SET_FEATURES;

    /* PIO flow control mode */
    Cmd.SectorCount = 0x8 + (PioMode - STATAPI_PIO_MODE_0);

    STTBX_Print(("Setting transfer mode\n"));
    Error = STATAPI_CmdNoData(Handle, &Cmd, &CmdStatus);
    if (Error == ST_NO_ERROR)
    {
        STOS_SemaphoreWait(StepSemaphore_p);
        if ((CmdStatus.Error != ST_NO_ERROR) ||
            (EvtError != ST_NO_ERROR))
        {
            STTBX_Print(("Error while setting features: "));
            DisplayErrorNew(CmdStatus.Error);
            DisplayErrorNew(EvtError);
        }
    }
    else
    {
        STTBX_Print(("Couldn't set features: "));
        DisplayErrorNew(Error);
    }

    return Error;
}

#if defined(RW_INFO)
static void ShowStatus(STATAPI_CmdStatus_t * Stat)
{

  STTBX_Print(("Command Status info:  \n"));
  STTBX_Print(("    Error %x \n",Stat->Error));
  STTBX_Print(("    Status %x \n",Stat->Status));
  STTBX_Print(("    LBA %x \n",Stat->LBA));

  STTBX_Print(("    Sector %x \n",Stat->Sector));
  STTBX_Print(("    CylinderLow %x \n",Stat->CylinderLow));
  STTBX_Print(("    CylinderHigh %x \n",Stat->CylinderHigh));
  STTBX_Print(("    Head %x \n",Stat->Head));
  STTBX_Print(("    SectorCount %x \n",Stat->SectorCount));

  STTBX_Print(("--------------------------------------\n"));
}
#endif

/**************************************************************************
       THIS FUNCTION WILL CALL ALL THE TEST ROUTINES...
**************************************************************************/

#if defined(MEDIAREF)

/*-------------------------------------------------------------------------
 * Function : ATAPI_UndoClkGen
 * Notes    : On Mediaref board, turns off clkgen workaround that uses
 *            ATA data lines
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t ATAPI_UndoClkGen(void)
{
    STPIO_InitParams_t PIO_InitParams;
    STPIO_OpenParams_t PIO_OpenParams;
    STPIO_TermParams_t PIO_TermParams;
    STPIO_Handle_t     PIO_Handle;
    ST_ErrorCode_t     ErrCode;

    PIO_InitParams.BaseAddress = (U32 *)PIO_3_BASE_ADDRESS;
    PIO_InitParams.InterruptNumber = PIO_3_INTERRUPT;
    PIO_InitParams.InterruptLevel = PIO_3_INTERRUPT_LEVEL;
    PIO_InitParams.DriverPartition = TEST_PARTITION_1;
    ErrCode = STPIO_Init("PIO3", &PIO_InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Init PIO driver (port 3) failed  "));
        DisplayErrorNew(ErrCode);
        return ErrCode;
    }

    PIO_OpenParams.ReservedBits    = PIO_BIT_5;
    PIO_OpenParams.BitConfigure[5] = STPIO_BIT_OUTPUT;
    PIO_OpenParams.IntHandler      = NULL;

    ErrCode = STPIO_Open("PIO3", &PIO_OpenParams, &PIO_Handle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Open PIO driver (port 3) failed: "));
        DisplayErrorNew(ErrCode);
        return(ErrCode);
    }

    /* switch off clkgen workaround that used ATA data lines */
    STPIO_Set(PIO_Handle, PIO_BIT_5);
    STPIO_Close(PIO_Handle);

    PIO_TermParams.ForceTerminate = FALSE;
    ErrCode = STPIO_Term("PIO3", &PIO_TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Term PIO driver (port 3) failed: "));
        DisplayErrorNew(ErrCode);
    }

    return ErrCode;
}
#endif

static void DoTheTests(void)
{
    ST_ErrorCode_t Error;
#if defined(ST_5518) || defined(ST_5508)
    STPIO_InitParams_t  PIO_Init;
    STPIO_OpenParams_t  PIO_Open;
    STPIO_Handle_t  PIO_Handle;
#endif
#if defined(ATAPI_FDMA)
    STFDMA_InitParams_t FDMA_Init;
#endif
    ATAPI_TestEntry_t *Test_p;
    U32 Section = 1;
    ATAPI_TestParams_t TestParams;
    ATAPI_TestResult_t Total = TEST_RESULT_ZERO, Result;

    /* Init the EVT driver */
    EVTInitParams.EventMaxNum= 10;
    EVTInitParams.ConnectMaxNum=4;
    EVTInitParams.SubscrMaxNum=2;
    EVTInitParams.MemoryPartition=TEST_PARTITION_1;

    EVTTermParams.ForceTerminate=FALSE;

    Error = STEVT_Init(EVTDevName,&EVTInitParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Event Device initialization failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
#if defined(STATAPI_CRYPT) && !defined(ATAPI_FDMA) 
    Error = STEVT_Init("STCRYPT",&EVTInitParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Event Device initialization failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
    
    /* Initialize crypto engine */
    Error = STCRYPT_Init("STCRYPT", CRYPT_INTERRUPT_LEVEL);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("%d\n", Error));
    }
#endif    

    Error = STEVT_Open(EVTDevName,&EVTOpenParams,&EVTHandle);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Event Device open failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }

    Error = STEVT_SubscribeDeviceEvent( EVTHandle, AtapiDevName,
                                STATAPI_HARD_RESET_EVT, &SubsParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Subscribe STATAPI_HARD_RESET_EVT failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }

    Error = STEVT_SubscribeDeviceEvent( EVTHandle, AtapiDevName,
                                STATAPI_SOFT_RESET_EVT, &SubsParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Subscribe STATAPI_SOFT_RESET_EVT failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
    Error = STEVT_SubscribeDeviceEvent( EVTHandle, AtapiDevName,
                                STATAPI_CMD_COMPLETE_EVT, &SubsParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Subscribe STATAPI_CMD_COMPLETE_EVT failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
    Error = STEVT_SubscribeDeviceEvent( EVTHandle, AtapiDevName,
                                STATAPI_PKT_COMPLETE_EVT, &SubsParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Subscribe STATAPI_PKT_COMPLETE_EVT failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
#if defined(TEST_TWO_DEVICE)    
    Error = STEVT_SubscribeDeviceEvent( EVTHandle, AtapiDevName2,
                                STATAPI_HARD_RESET_EVT, &SubsParams2);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Subscribe STATAPI_HARD_RESET_EVT failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }

    Error = STEVT_SubscribeDeviceEvent( EVTHandle, AtapiDevName2,
                                STATAPI_SOFT_RESET_EVT, &SubsParams2);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Subscribe STATAPI_SOFT_RESET_EVT failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
    Error = STEVT_SubscribeDeviceEvent( EVTHandle, AtapiDevName2,
                                STATAPI_CMD_COMPLETE_EVT, &SubsParams2);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Subscribe STATAPI_CMD_COMPLETE_EVT failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
    Error = STEVT_SubscribeDeviceEvent( EVTHandle, AtapiDevName2,
                                STATAPI_PKT_COMPLETE_EVT, &SubsParams2);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Subscribe STATAPI_PKT_COMPLETE_EVT failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
    StepSemaphore2_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
#endif

    StepSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
 

#if defined(ATAPI_FDMA)
#if defined(ST_5517)
    FDMA_Init.DeviceType            = STFDMA_DEVICE_FDMA_1;
#elif defined(ST_5100) || (defined (ST_7100) || defined (ST_7109) || defined(ST_5525)\
                            && !defined(SATA_SUPPORTED))
    FDMA_Init.DeviceType            = STFDMA_DEVICE_FDMA_2;
#else
#error Do not know FDMA device type.
#endif

    FDMA_Init.DriverPartition_p     = TEST_PARTITION_1;
    FDMA_Init.NCachePartition_p     = TINY_PARTITION_1;
    FDMA_Init.BaseAddress_p         = (U32 *)FDMA_BASE_ADDRESS;
    FDMA_Init.InterruptNumber       = FDMA_INTERRUPT_NUMBER;
    FDMA_Init.InterruptLevel        = 5;        /* Random */
    FDMA_Init.NumberCallbackTasks   = 0;
    FDMA_Init.ClockTicksPerSecond   = ST_GetClocksPerSecond();
    FDMA_Init.FDMABlock             = STFDMA_1;

    Error = STFDMA_Init(FDMADevName, &FDMA_Init);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Initializing STFDMA... failed: "));
        DisplayErrorNew(Error);
        return;
    }
#endif

#if defined(ST_5512)
       /*---- Let's load the FPGA code ---*/
    Error =STFPGA_Init();
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("FPGA Init failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }

    /* Now change the EMI configuration (bank 3) to set PIO mode 4 */
#if defined(STATAPI_OLD_FPGA)
    *(volatile U32*) 0x2020 = 0x16D1;
    *(volatile U32*) 0x2024 = 0x50F0;
    *(volatile U32*) 0x2028 = 0x400F;
    *(volatile U32*) 0x202C = 0x0002;
#else
    /* "New" (supported) FPGA */
    *(volatile U32*) 0x00002030 = 0x1791;
    *(volatile U32*) 0x00002034 = 0x50f0;
    *(volatile U32*) 0x00002038 = 0x50f0;
    *(volatile U32*) 0x0000203C = 0x0002;
#endif

#elif defined(ST_5518) || defined(ST_5508)
    /* Set FEI_ATAPI_CFG register */
    STSYS_WriteRegDev8((void*)0x200387A0, 0x01);
    /* Configure PIO0-1 and PIO0-2  as  output */

    PIO_Init.BaseAddress=(U32*)PIO_0_BASE_ADDRESS;
    PIO_Init.InterruptNumber=PIO_0_INTERRUPT;
    PIO_Init.InterruptLevel=PIO_0_INTERRUPT_LEVEL;
    PIO_Init.DriverPartition=TEST_PARTITION_1;
    Error =STPIO_Init(PIODevName,&PIO_Init);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Init PIO driver (port 0) failed  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }

    PIO_Open.ReservedBits=(PIO_BIT_1| PIO_BIT_2);
    PIO_Open.BitConfigure[1]=STPIO_BIT_OUTPUT;
    PIO_Open.BitConfigure[2]=STPIO_BIT_OUTPUT;
    PIO_Open.IntHandler=NULL;
    Error =STPIO_Open(PIODevName,&PIO_Open,&PIO_Handle);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Open PIO driver failed  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }

#endif

#if defined(MEDIAREF)
    ATAPI_UndoClkGen();
#endif


    STTBX_Print(("*******************************************************  \n"));
    STTBX_Print(("*******************************************************  \n"));
    STTBX_Print(("                 ATAPI TEST HARNESS  \n"));
    STTBX_Print(("Test Harness Revision: %s\n", Revision));
    STTBX_Print(("Driver Revision: %s\n", STATAPI_GetRevision() ));

    ShowConfig();

    STTBX_Print(("*******************************************************  \n"));
    STTBX_Print(("*******************************************************  \n"));
    STTBX_Print(("\n"));


    Test_p = ATAPITestTable;

    while (Test_p->TestFunction != NULL)
    {
        TestParams.DevAddress= STATAPI_DEVICE_0;
        while (Test_p->RepeatCount > 0)
        {
            STTBX_Print(("**************************************************\n"));
            STTBX_Print(("SECTION %d - %s\n", Section,
                              Test_p->TestInfo));
            STTBX_Print(("**************************************************\n"));

            /* Set up parameters for test function */
            TestParams.Ref = Section;

            /* Call test */
            Result = Test_p->TestFunction(&TestParams);

            /* Update counters */
            Total.NumberPassed += Result.NumberPassed;
            Total.NumberFailed += Result.NumberFailed;
            Test_p->RepeatCount--;
            TestParams.DevAddress= STATAPI_DEVICE_1;
        }

        Test_p++;
        Section++;
    }

    /* Output running analysis */
    STTBX_Print(("**************************************************\n"));
    STTBX_Print(("PASSED: %d\n", Total.NumberPassed));
    STTBX_Print(("FAILED: %d\n", Total.NumberFailed));
    STTBX_Print(("**************************************************\n"));

    Error = STEVT_Close(EVTHandle);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("STEVT Close failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }

#if defined(ATAPI_FDMA)

    Error = STFDMA_Term(FDMADevName, FALSE, STFDMA_1);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("STFDMA term failed: "));
        DisplayErrorNew(Error);
    }
#endif

    Error = STEVT_Term(EVTDevName,&EVTTermParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("STEVT Close failed:  "));
        DisplayError(Error);
        STTBX_Print(("\n"));
    }
}

/* Somewhat messy function to print out the system configuration */
static void ShowConfig(void)
{
#if defined(DISABLE_ICACHE)
    char *icache = "disabled";
#else
    char *icache = "enabled";
#endif
#if defined(DISABLE_DCACHE)
    char *dcache = "disabled";
#else
    char *dcache = "enabled";
#endif
#if defined(UNIFIED_MEMORY)
    char *memory = "unified";
#else
    char *memory = "standard";
#endif

    STTBX_Print(("\nCPU running at %iMHz", ST_GetClockSpeed() / 1000000));
#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010) || ((defined(ST_7100)\
|| defined(ST_7109) || defined(ST_7200)) && (defined SATA_SUPPORTED))

    STTBX_Print(("; HDDI at %iMHz\n", ClockFrequency / 1000000));
#else
    STTBX_Print(("\n"));
#endif
    STTBX_Print(("Instruction cache %s", icache));
    STTBX_Print(("; data cache %s\n", dcache));
    STTBX_Print(("Running in %s memory\n", memory));
#if defined(DEBUG)
    STTBX_Print(("Debug is enabled\n"));
#endif
#if defined(ATAPI_DEBUG)
    STTBX_Print(("STATAPI debug is enabled\n"));
#endif
}

void test_overhead(void *dummy) { }

void test_init(void *dummy)
{
    /* Parameters, buffers, and structures */
    STATAPI_InitParams_t    InitParams;
    ST_ErrorCode_t error;

#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    InitParams.DeviceType           = STATAPI_HDD_UDMA4;
#else
    InitParams.DeviceType           = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
     InitParams.DeviceType          = STATAPI_SATA;
#else
    InitParams.DeviceType           = STATAPI_EMI_PIO4;
#endif
    InitParams.DriverPartition      = TEST_PARTITION_1;
#if defined(ATAPI_FDMA)
    InitParams.NCachePartition      = TINY_PARTITION_1;
#endif
    InitParams.BaseAddress          = (volatile U32 *)ATA_BASE_ADDRESS;
    InitParams.HW_ResetAddress      = (volatile U16 *)ATA_HRD_RST;
    InitParams.InterruptNumber      = ATA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel       = ATA_INTERRUPT_LEVEL;
    InitParams.ClockFrequency       = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    InitParams.RegisterMap_p        = (STATAPI_RegisterMap_t *)PATARegMasks;
#endif
    
    strcpy(InitParams.EVTDeviceName, EVTDevName);
    error = STATAPI_Init(AtapiDevName, &InitParams);
#if defined(SATA_SUPPORTED)
    if(error == ST_NO_ERROR)
            STTBX_Print(("\n ----SATA Host Initilization Done-----, OK.\n"))
#else
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_Init()\n"));
        DisplayErrorNew(error);
    }
#endif
}

/* No dma commands, since the stack variation will be minimal */
void test_typical(void *dummy)
{
    ST_ErrorCode_t          error;
    STATAPI_Handle_t        Handle;
    STATAPI_OpenParams_t    OpenParams;

    STATAPI_PioMode_t       PioMode;
    STATAPI_PioTiming_t     PioTimingParams;
    STATAPI_Capability_t    Capability;
    STATAPI_Cmd_t           Cmd;
    STATAPI_CmdStatus_t     CmdStatus;

    U32                     BytesTransferred;
    STATAPI_DriveType_t     enDriveType;

    OpenParams.DeviceAddress = STATAPI_DEVICE_0;

    memset(&Cmd, 0, sizeof(STATAPI_Cmd_t));

    error = STATAPI_Open(AtapiDevName, &OpenParams, &Handle);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_Open() - "));
        DisplayErrorNew(error);
    }

    error = STATAPI_SoftReset(AtapiDevName);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_SoftReset() - "));
        DisplayErrorNew(error);
    }

    STOS_SemaphoreWait(StepSemaphore_p);
    if (EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("Event carried result: "));
        DisplayErrorNew(EvtError);
    }

    error = STATAPI_Abort(Handle);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_Abort() - "));
        DisplayErrorNew(error);
    }

    error = STATAPI_GetCapability(AtapiDevName, &Capability);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_GetCapability() - "));
        DisplayErrorNew(error);
    }

#if defined(ST_5512) || defined(ST_5514) || defined(ST_5528) || defined(ST_8010) ||\
((defined(ST_7100) || defined(ST_7109) || defined(ST_7200)) && (defined (SATA_SUPPORTED)))
    error = STATAPI_HardReset(AtapiDevName);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_HardReset() - "));
        DisplayErrorNew(error);
    }

    STOS_SemaphoreWait(StepSemaphore_p);
    if (EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("Event from STATAPI_HardReset: "));
        DisplayErrorNew(EvtError);
    }
#endif

    error = STATAPI_SetPioMode(Handle, STATAPI_PIO_MODE_4);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_SetPioMode() - "));
        DisplayErrorNew(error);
    }

    error = STATAPI_GetPioMode(Handle, &PioMode,
                               &PioTimingParams);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_GetPioMode() - "));
        DisplayErrorNew(error);
    }

	error = STATAPI_GetDriveType ( Handle, &enDriveType );

	if ( error != ST_NO_ERROR )
	{
        STTBX_Print(("STATAPI_GetDriveType()\n"));
	}

	if ( enDriveType == STATAPI_ATA_DRIVE )
	{
        Cmd.CmdCode = STATAPI_CMD_IDENTIFY_DEVICE;
        hATADevHandle	= Handle;
	}
	else if ( enDriveType == STATAPI_ATAPI_DRIVE )
	{
        Cmd.CmdCode = STATAPI_CMD_IDENTIFY_PACKET_DEVICE;
        hATAPIDevHandle	= Handle;
	}

    /* Data-in command */
    Cmd.SectorCount = 1;
    Cmd.UseLBA = FALSE;
    error = STATAPI_CmdIn(Handle, &Cmd, MemTestDataBuffer,
                          10240, &BytesTransferred, &CmdStatus);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_CmdIn()\n"));
        DisplayErrorNew(error);
    }

    STOS_SemaphoreWait(StepSemaphore_p);
    if (EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("Event from STATAPI_CmdIn: "));
        DisplayErrorNew(EvtError);
        STTBX_Print(("Bytes transferred: %i\n", BytesTransferred));
    }
    if ( enDriveType == STATAPI_ATA_DRIVE )
	{
        /* Test no-data command */
        Cmd.CmdCode = STATAPI_CMD_FLUSH_CACHE;
        Cmd.UseLBA = FALSE;
        error = STATAPI_CmdNoData(Handle, &Cmd, &CmdStatus);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("STATAPI_CmdNoData() - "));
            DisplayErrorNew(error);
        }

        STOS_SemaphoreWait(StepSemaphore_p);
        if (EvtError != ST_NO_ERROR)
        {
            STTBX_Print(("Event from STATAPI_CmdOut: "));
            DisplayErrorNew(EvtError);
            STTBX_Print(("Bytes transferred: %i\n", BytesTransferred));
        }

        /* Data-out command */
        Cmd.CmdCode = STATAPI_CMD_WRITE_SECTORS;
        Cmd.SectorCount = 1;
        Cmd.UseLBA = FALSE;
        Cmd.Sector = 1;
        Cmd.CylinderLow =  1;
        Cmd.CylinderHigh = 0;
        Cmd.Head = 0;
        error = STATAPI_CmdOut(Handle, &Cmd, MemTestDataBuffer,
                               10240, &BytesTransferred, &CmdStatus);
        if (error != ST_NO_ERROR)
        {
            STTBX_Print(("STATAPI_CmdOut() - "));
            DisplayErrorNew(error);
        }

        STOS_SemaphoreWait(StepSemaphore_p);
        if (EvtError != ST_NO_ERROR)
        {
            STTBX_Print(("Event from STATAPI_CmdOut: "));
            DisplayErrorNew(EvtError);
        }
    }
    else
    {
        STTBX_Print(("%ATAPI Start/Open/Close Command : \n"));
        STTBX_Print(("Purpose: to ensure enable and disable MM Device for Access \n"));
        ControlTray ( hATAPIDevHandle, 1 );
		if ( ControlTray ( hATAPIDevHandle, 1 ) != ST_NO_ERROR )
		{
		    ControlTray ( hATAPIDevHandle, 1 );
		}

	    task_delay ( 10 * 100000 );
	    task_delay ( 10 * 100000 );

        STTBX_Print(("Put the DVD-ROM/CD-ROM inside the tray before "));
        STTBX_Print(("the tray\ngoes in \n"));

		ControlTray ( hATAPIDevHandle, 0 );

	 	task_delay ( 1000000 );


        STTBX_Print(("%ATAPI Request Sense Command : \n"));
        STTBX_Print(("Purpose: to request LU to transfer Sense data to initiator \n"));
		error= RequestSense (hATAPIDevHandle);
	}

    error = STATAPI_Close(Handle);

    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_Close() - "));
        DisplayErrorNew(error);
    }
}

void test_term(void *dummy)
{
    ST_ErrorCode_t error;
    STATAPI_TermParams_t TermParams;

    TermParams.ForceTerminate = TRUE;
    error = STATAPI_Term(AtapiDevName, &TermParams);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("STATAPI_Term()\n"));
        DisplayError(error);
    }
}

static ATAPI_TestResult_t ATAPI_MemoryTest(ATAPI_TestParams_t *TestParams)
{
    ATAPI_TestResult_t result = TEST_RESULT_ZERO;
    partition_status_t  partstatus;
    U32     MemPre, MemPost;
    task_t *task_p;
    task_status_t status;
#if defined(ST_OS20)
    task_t task;
    tdesc_t tdesc;
    char stack[4 * 1024];
#endif

    char *test_names[] = { "task overhead",
                           "driver init",
                           "typical functions",
                           "driver term",
                           NULL
                          };
    void (*func_table[])(void *) = { test_overhead,
                                     test_init,
                                     test_typical,
                                     test_term,
                                     NULL };
    U8 i;
    void (*func)(void *);

    /* Get initial memory free */
    partition_status(TEST_PARTITION_1, &partstatus,
                                (partition_status_flags_t)0);
    MemPre = partstatus.partition_status_free;

    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];
#if defined(ST_OS20)
        task_p = &task;
        if (task_init(func, NULL, stack, 4 * 1024, task_p, &tdesc,
                      MAX_USER_PRIORITY, "stack test", 0))
        {   /* Error: signal by setting task ptr to NULL */
            task_p = NULL;
        }
#else
        task_p = task_create(func, NULL, 16*4*1024,
                             MAX_USER_PRIORITY, "stack test",
                             0);
#endif
        if (NULL == task_p)
        {
            STTBX_Print(("Task create %s failed\n", test_names[i]));
            break;
        }
        task_wait(&task_p, 1, TIMEOUT_INFINITY);

        task_status(task_p, &status, task_status_flags_stack_used);
        STTBX_Print(("Checking %s, stack used %i\n",
                    test_names[i], status.task_stack_used));

        task_delete(task_p);
    }

    /* Get final memory free */
    partition_status(TEST_PARTITION_1, &partstatus,
                                (partition_status_flags_t)0);
    MemPost = partstatus.partition_status_free;
    if (MemPre != MemPost)
    {
        STTBX_Print(("Before tests: %i\t\tAfter tests: %i\n",
                    MemPre, MemPost));
        ATAPI_TestFailed(result, "Memory leak detected");
    }
    else
    {
        ATAPI_TestPassed(result);
    }

    return result;
}
static ATAPI_TestResult_t ATAPI_RegTest(ATAPI_TestParams_t *TestParams)
{
    ST_ErrorCode_t          error = ST_NO_ERROR;
    STATAPI_RegisterMap_t   GoodRegMap, BadRegMap, BadRegMapCopy;
    ATAPI_TestResult_t      result = TEST_RESULT_ZERO;

    /* Output user info */
    STTBX_Print(("%d.0 Register test \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure register map can be read/written\n"));

    /* Retrieve current register map */
    error = STATAPI_GetRegisters(&GoodRegMap);
    if (error != ST_NO_ERROR)
    {
        ATAPI_TestFailed(result, "Couldn't get register map");
    }
    else
    {
        U32 *TmpPtr;
        U32 i;

        /* Build sample register map */
        memcpy(&BadRegMap, &GoodRegMap, sizeof(BadRegMap));
        TmpPtr = (U32 *)&BadRegMap;
        for (i = 0; i < (sizeof(STATAPI_RegisterMap_t) / 4); i++)
            *(TmpPtr++) += 1;
        memcpy(&BadRegMapCopy, &BadRegMap, sizeof(BadRegMapCopy));

        /* Make call to set registers */
        error = STATAPI_SetRegisters(&BadRegMap);
        if (error != ST_NO_ERROR)
        {
            ATAPI_TestFailed(result, "Couldn't write bad map!");
        }
        else
        {
            /* Get new map */
            error = STATAPI_GetRegisters(&BadRegMap);
            if (error != ST_NO_ERROR)
            {
                ATAPI_TestFailed(result, "Couldn't get new map!");
            }
            else
            {
                /* Compare */
                if (memcmp(&BadRegMap, &BadRegMapCopy, sizeof(BadRegMap)) != 0)
                {
                    ATAPI_TestFailed(result, "Map mismatch!");
                }
                else
                {
                    ATAPI_TestPassed(result);
                }
            }
        }

        /* Restore old map */
        error = STATAPI_SetRegisters(&GoodRegMap);
        if (error != ST_NO_ERROR)
        {
            ATAPI_TestFailed(result, "Couldn't restore good map - WARNING!!");
        }
    }

    return result;
}

/*****************************************************************
    ATAPI_TestSimpleAPI

  This test is aimed to show how the basic functionalities of the driver
  work correctly.
  If This test is ran twice it test first the DEVICE 0 and then the DEVICE 1.

******************************************************************/

static ATAPI_TestResult_t ATAPI_TestSimpleAPI(ATAPI_TestParams_t * TestParams)
{

    ST_ErrorCode_t  Error;
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;
    U32 i;
    /* ATAPI driver parameters */
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;

#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif
    AtapiInitParams.DriverPartition= TEST_PARTITION_1;
#if defined(ATAPI_FDMA)
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#endif
    AtapiInitParams.BaseAddress= (U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress= (U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p  = (STATAPI_RegisterMap_t *)PATARegMasks;
#endif    
    strcpy(AtapiInitParams.EVTDeviceName,EVTDevName);

    /* STATAPI_Init() */
    STTBX_Print(("%d.0 STATAPI_Init() \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure a ATAPI device can be initialized\n"));
    {
        Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to initialize ATAPI device");
            goto api_end;
        }
        ATAPI_TestPassed(Result);

    }
  for(i=0;i < 2;i++)
  {
    if (Drives[i].Present)
    {
    /* STATAPI_Open() */
    if (i==0)
        AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_0;
    else
        AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_1;

    STTBX_Print(("%d.1 STATAPI_Open() \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure a ATAPI device connection can be opened\n"));
    {
        Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&Drives[i].Handle);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to open ATAPI device");
            goto api_end;
        }
        ATAPI_TestPassed(Result);

    }

     /* STATAPI_SetPioMode */

    STTBX_Print(("%d.2 Set Pio mode 4 \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure we can set the Pio mode 4\n "));
    {
        Error = STATAPI_SetPioMode(Drives[i].Handle,STATAPI_PIO_MODE_4);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to set Pio mode 4");
            goto api_end;
        }
        ATAPI_TestPassed(Result);

    }
   /* Simple Read/Write Test */
    if (Drives[i].Type== STATAPI_ATA_DRIVE)
    {
    /* Simple Read/Write Test */
      STTBX_Print((" \n"));
      STTBX_Print(("%d.3 Simple R/W Test for device %d\n",TestParams->Ref,i));
      STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
      {
        Error = SimpleRWTest(Drives[i].Handle,&AddressTable[0],3);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to Read/Write ATA device");
            goto api_end;
        }
        ATAPI_TestPassed(Result);
      }
    }
    else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
    {
        U8 repeat=0;
        STTBX_Print((" \n"));
        STTBX_Print(("%d.3 Simple R/W Test \n",TestParams->Ref));
        STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
 		do
		{
			if ( TestUnitReady (Drives[i].Handle) == ST_NO_ERROR )
			{
			    RequestSense (Drives[i].Handle);
				break;
			}
			task_delay ( 20 * 200000 );
		    repeat++;
		}while ( repeat < 10 );

		if ( repeat >= 10 )
		{
    	        STTBX_Print(("%Unit Not Ready\n",TestParams->Ref));
    	        goto api_end;
		}
		repeat = 0;

       	/*For testing disc read/write in pio mode */
		if ( LastLBA > 0x100000 )
		{
			StartSector = 16;
			NoOfSectors	= 4;
			Error = SimpleRTestDVD(Drives[i].Handle,StartSector, NoOfSectors );

            if (CheckCodeOk(Error))
            {
                ATAPI_TestFailed(Result,"Unable to Read(DVD) ATAPI device");
                goto api_end;
            }
            ATAPI_TestPassed(Result);

		}
		else
		{
			StartSector = 160;
			NoOfSectors	= 4;
			Error = SimpleRTestCD(Drives[i].Handle,StartSector, NoOfSectors );
            if (CheckCodeOk(Error))
            {
                ATAPI_TestFailed(Result,"Unable to Read(CD) ATAPI device");
                goto api_end;
            }
            ATAPI_TestPassed(Result);
		}
    }

api_end:

    /* STATAPI_Close() */

    STTBX_Print(("%d.4 STATAPI_Close \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure we can close a connection\n"));
    {
        Error = STATAPI_Close(Drives[i].Handle);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to close a connection");
        }
        ATAPI_TestPassed(Result);

    }

   } /* end if (drive present)*/
  } /* end for */

    AtapiTermParams.ForceTerminate= FALSE;

    /* STATAPI_Term() */

    STTBX_Print(("%d.5 STATAPI_Term \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure we can terminate the driver\n"));
    {
        Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to terminate the driver");
        }else
        {
            ATAPI_TestPassed(Result);
        }
    }
    return Result;
}

/*****************************************************************
    ATAPI_TestAPINormal

  This test is aimed to show how almost all the functionalities of
  the driver work correctly. These are:
    - Init / Term
    - Open/ Close
    - Re-open
    - GetParams
    - GetCapability
    - GetPioMode
    - HardReset
    - SoftReset
    - CmdIn (READ_SECTOR and READ_MULTIPLE)
    - CmdOut (WRITE_SECTOR and WRITE_MULTIPLE)
    - CmdNoData (SET_MULIPLE_MODE)

  If This test is ran twice it test first the DEVICE 0 and then the DEVICE 1.

******************************************************************/
static ATAPI_TestResult_t ATAPI_TestAPINormal(ATAPI_TestParams_t * TestParams)
{

    ST_ErrorCode_t  Error;
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;
    U32 i;
    STATAPI_Params_t    DriverParams;
    STATAPI_Capability_t    Capability;
    STATAPI_PioMode_t Mode;
    STATAPI_PioTiming_t TimingParams;
    /* ATAPI driver parameters */
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;
#ifdef DMA_PRESENT
    STATAPI_DmaMode_t       DmaMode;
    STATAPI_DmaTiming_t     DmaTimingParams;
#endif

#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif

    AtapiInitParams.DriverPartition= TEST_PARTITION_1;

#if defined(ATAPI_FDMA)
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#endif
    AtapiInitParams.BaseAddress= (U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress= (U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p  = (STATAPI_RegisterMap_t *)PATARegMasks;
#endif    
    strcpy(AtapiInitParams.EVTDeviceName,EVTDevName);

    /* STATAPI_Init() */
    STTBX_Print(("%d.0 STATAPI_Init() \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure a ATAPI device can be initialized\n"));
    {
        Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to initialize ATAPI device");
            goto api2_end;
        }
        ATAPI_TestPassed(Result);

    }

 	for(i = 0;i < 2;i++)
    {
        if (Drives[i].Present)
        {
            /* STATAPI_Open() */
            if (i==0)  AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_0;
            else  AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_1;
            STTBX_Print(("%d.1 STATAPI_Open() \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure a ATAPI device connection can be opened\n"));
            {
                Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&Drives[i].Handle);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to open ATAPI device");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            /* STATAPI_SetPioMode */
            STTBX_Print(("%d.2 Set Pio mode 4 \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure we can set the Pio mode 4\n "));
            {
                Error = STATAPI_SetPioMode(Drives[i].Handle,STATAPI_PIO_MODE_4);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set Pio mode 4");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.2a Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTest(Drives[i].Handle,
                                         &AddressTable[TestParams->Ref],
                                         3);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                U8 repeat=0;
                STTBX_Print((" \n"));

         		do
        		{
        			if (TestUnitReady (Drives[i].Handle) == ST_NO_ERROR )
        			{
        			    RequestSense (Drives[i].Handle);
        				break;
        			}
        			task_delay ( 20 * 200000 );
        			++repeat;
        		}while ( repeat < 10 );

        		if ( repeat >= 10 )
        		{
            	        STTBX_Print(("%Unit Not Ready\n",TestParams->Ref));
            	        goto api2_end;
        		}
        		repeat = 0;

                STTBX_Print(("%d.2a Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));

                 	/*For testing disc read/write in pio mode */
            		if ( LastLBA > 0x100000 )
            		{
            			StartSector = 4;
            			NoOfSectors	= 4;
            			Error = SimpleRTestDVD(Drives[i].Handle,StartSector, NoOfSectors );

                        if (CheckCodeOk(Error))
                        {
                            ATAPI_TestFailed(Result,"Unable to Read(DVD) ATAPI device");
                            goto api2_end;
                        }
                        ATAPI_TestPassed(Result);

            		}
            		else
            		{
            			StartSector = 200;
            			NoOfSectors	= 4;
            			Error = SimpleRTestCD(Drives[i].Handle,StartSector, NoOfSectors );
                        if (CheckCodeOk(Error))
                        {
                            ATAPI_TestFailed(Result,"Unable to Read(CD) ATAPI device");
                            goto api2_end;
                        }
                        ATAPI_TestPassed(Result);
            	    }

                      STTBX_Print(("%d.2b  Read TOC Test \n",TestParams->Ref));
                      STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                     {

                        Error = ReadTOCTest(Drives[i].Handle);
                        if (CheckCodeOk(Error))
                        {
                            ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                            goto api2_end;
                        }

                        ATAPI_TestPassed(Result);
                      }
                  }

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.2c Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write 256-sectors of data and then read it back \n"));
                {
                    Error = SimpleRWTest(Drives[i].Handle,
                                         &AddressTable[TestParams->Ref],
                                         0);  /* 0 == 256 */
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);
                }
            }

#ifdef DMA_PRESENT
            /* Make sure the sectors are "empty". MWDMA write should then
             * fill the sectors, before reading them back to verify. UDMA
             * write currently doesn't work, so the UDMA read will read back
             * the data written by the MWDMA write and verify. When UDMA out
             * is fixed, this behaviour will need to be adjusted. Note that
             * this applies to virtually all of the MWDMA/UDMA tests
             * throughout the test harness.
             */

            ClearSectors(Drives[i].Handle, &AddressTable[TestParams->Ref], 1);
#if !defined(SATA_SUPPORTED)
            STTBX_Print(("%d.3 DMA mode tests \n",TestParams->Ref));
            STTBX_Print(("%d.3a Purpose: to ensure we can set MWDMA mode 2\n",TestParams->Ref));
            {
                Error = STATAPI_SetDmaMode(Drives[i].Handle,STATAPI_DMA_MWDMA_MODE_2);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set MWDMA mode 2");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            Error = SetTransferModeDma(Drives[i].Handle, STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.3a Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTestDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref],
                                            1);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                U8 repeat=0;
                STTBX_Print((" \n"));
                STTBX_Print(("%d.3a Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));

         		do
        		{
        			if ( TestUnitReady (Drives[i].Handle) == ST_NO_ERROR )
        			{
        			    RequestSense (Drives[i].Handle);
        				break;
        			}
        			task_delay ( 20 * 200000 );
        			repeat++;
        		}while ( repeat < 10 );

        		if ( repeat >= 10 )
        		{
            	        STTBX_Print(("%Unit Not Ready\n",TestParams->Ref));
            	        goto api2_end;
        		}
        		repeat = 0;
                     /*For testing disc read/write in Dma mode */
    		    if ( LastLBA > 0x100000 )
    		    {
        			StartSector = 10;
        			NoOfSectors	= 255;
        			Error = SimpleRTestDVD(Drives[i].Handle,StartSector, NoOfSectors );

                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read(DVD) ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);

    		    }
    		    else
    		    {
        			StartSector = 200;
        			NoOfSectors	= 255;
        			Error = SimpleRTestCD(Drives[i].Handle,StartSector, NoOfSectors );
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read(CD) ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);
    		    }
                STTBX_Print(("%d.3b Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {

                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                    }

                    ATAPI_TestPassed(Result);
                }
            }

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.3b Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write 256-sectors of data and then read it back \n"));
                {
                    Error = Rw256SectorsDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref]);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
#endif/*!SATA_SUPPORTED*/
            STTBX_Print(("%d.3c Purpose: to ensure we can set UDMA mode 4\n",
                        TestParams->Ref));
            {
                Error = STATAPI_SetDmaMode(Drives[i].Handle,
                                           STATAPI_DMA_UDMA_MODE_4);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set UDMA mode 4");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            Error = SetTransferModeDma(Drives[i].Handle,
                                       STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.3c Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTestDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref],
                                            10);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                U8 repeat=0;
                U32 Acum,Throughput=0;
                STTBX_Print((" \n"));
                STTBX_Print(("%d.3c Simple R/W in dma mode \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));

         		do
        		{
        			if (TestUnitReady (Drives[i].Handle) == ST_NO_ERROR )
        			{
        			    RequestSense (Drives[i].Handle);
        				break;
        			}
        			task_delay ( 20 * 200000 );
        			++repeat;
        		}while ( repeat < 10 );

        		if ( repeat >= 10 )
        		{
            	        STTBX_Print(("%Unit Not Ready\n",TestParams->Ref));
            	        goto api2_end;
        		}
        		repeat = 0;
                    /*For testing disc read/write in pio mode */
    		    if ( LastLBA > 0x100000 )
    		    {
        			StartSector = 10;
        			NoOfSectors	= 255;

        			Error = SimpleRTestDVD(Drives[i].Handle,StartSector, NoOfSectors );

                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read(DVD) ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);

    		    }
    		    else
    		    {
        			StartSector = 200;
        			NoOfSectors	= 255;
        			Error = SimpleRTestCD(Drives[i].Handle,StartSector, NoOfSectors );
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read(CD) ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);
    		    }
                STTBX_Print(("%d.3d Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {

                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                    }

                    ATAPI_TestPassed(Result);
                }

                STTBX_Print(("%d.3e Seek time in Dma Mode\n", TestParams->Ref));
                STTBX_Print(("Purpose: Calculate seek time taken by LU to track LBA\n"));
    	        Error = PacketSeekTime(Drives[i].Handle,ITERATION_NUMBER, &Acum);
    		    if (Error != ST_NO_ERROR)
                {
                    ATAPI_TestFailed(Result, "Something failed on the seek test ");
                    DisplayError(Error);
                }
                else
                {
                    STTBX_Print(("Seek time = %d us\n",(Acum * 1000000) /
                                       (ClocksPerSec * ITERATION_NUMBER)));
                    ATAPI_TestPassed(Result);
                }

                STTBX_Print(("%d.3f Read seek time in Dma mode\n", TestParams->Ref));
                STTBX_Print(("Purpose: Calculate the read seek time\n"));
                Error = PacketReadSeekTime(Drives[i].Handle, ITERATION_NUMBER, &Acum,&Throughput);
                STTBX_Print(("Throughput in Dma mode = %d Kb/s \n",Throughput));
                if (Error != ST_NO_ERROR)
                {
                   ATAPI_TestFailed(Result, "Something failed on the read seek test");
                }
               else
               {
                  STTBX_Print(("Read seek time = %d us\n",
                         (Acum)/(ITERATION_NUMBER)));
                  ATAPI_TestPassed(Result);
               }
            }

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.3g Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write 256-sectors of data and then read it back \n"));
                {
                    Error = Rw256SectorsDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref]);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }

#endif

            /* STATAPI_Close() */
            STTBX_Print(("%d.4 STATAPI_Close \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure we can close a connection\n"));
            {
                Error = STATAPI_Close(Drives[i].Handle);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to close a connection");
                }else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            /* STATAPI_Open() */
            AtapiOpenParams.DeviceAddress= i;
            STTBX_Print(("%d.5 STATAPI_Open() \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure we can re-open a closed connection\n"));
            {
                Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&Drives[i].Handle);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to re-open ATAPI device");

                }else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            /* STATAPI_SetPioMode */
            STTBX_Print(("%d.6 Set Pio mode 4 \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure we can set the Pio mode 4\n "));
            {
                Error = STATAPI_SetPioMode(Drives[i].Handle,STATAPI_PIO_MODE_4);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set Pio mode 4");

                }else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            Error = SetTransferModePio(Drives[i].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            /* Simple Read/Write Test */

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.7 Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTest(Drives[i].Handle,&AddressTable[TestParams->Ref],3);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                STTBX_Print(("%d.7  Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {

                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                        goto api2_end;
                    }

                    ATAPI_TestPassed(Result);
                }
            }

            if (Drives[i].Type == STATAPI_ATA_DRIVE)
            {
                /* Multiple Read/Write Test */

                STTBX_Print(("%d.8 Multiple Read Write Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: w/r 4 blocks in multiple mode 2\n"));
                {
                    Error = MultipleRWTest(Drives[i].Handle,
                                           &AddressTable[TestParams->Ref],
                                           4, 2);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Erase/Read on the ATAPI device");

                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else
            {
                STTBX_Print(("%d.8 Multiple Read Write Test overridden\n",TestParams->Ref));
                ATAPI_TestPassed(Result);
            }

#if defined(DMA_PRESENT)

            /* Make sure the sectors are "empty" */
            ClearSectors(Drives[i].Handle, &AddressTable[TestParams->Ref], 10);
#if !defined(SATA_SUPPORTED)
            STTBX_Print(("%d.8b Purpose: to ensure we can set MWDMA mode 2\n",TestParams->Ref));
            {
                Error = STATAPI_SetDmaMode(Drives[i].Handle,STATAPI_DMA_MWDMA_MODE_2);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set MWDMA mode 2");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            Error = SetTransferModeDma(Drives[i].Handle, STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            if (Drives[i].Type == STATAPI_ATA_DRIVE)
            {
                U8 NumSectors = 10;

                /* Multiple Read/Write Test */
                STTBX_Print(("%d.8b Multiple Sector Read Write Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: w/r %i sectors\n", NumSectors));
                {
                    Error = SimpleRWTestDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref],
                                            NumSectors);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Erase/Read on the ATAPI device");

                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else
            {
                STTBX_Print(("%d.8b Multiple Read Write Test overridden\n",TestParams->Ref));
                ATAPI_TestPassed(Result);
            }
#endif/*SATA_SUPPORTED*/
            STTBX_Print(("%d.8a Purpose: to ensure we can set UDMA mode 4\n",TestParams->Ref));
            {
                Error = STATAPI_SetDmaMode(Drives[i].Handle,
                                           STATAPI_DMA_UDMA_MODE_4);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set UDMA mode 4");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            Error = SetTransferModeDma(Drives[i].Handle, STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            if (Drives[i].Type == STATAPI_ATA_DRIVE)
            {
                U8 NumSectors;

                NumSectors = 10;

                /* Multiple Read/Write Test */
                /* DMA transfers don't use multiple mode; they just take a
                 * number of sectors, given here as the last param to the
                 * function
                 */
                STTBX_Print(("%d.8a Multiple Sector Read Write Test \n",
                            TestParams->Ref));
                STTBX_Print(("Purpose: w/r %i sectors at a time\n",
                            NumSectors));
                {
                    Error = SimpleRWTestDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref],
                                            NumSectors);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Erase/Read on the ATAPI device");

                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else
            {
                STTBX_Print(("%d.8a Multiple Read Write Test overridden\n",TestParams->Ref));
                ATAPI_TestPassed(Result);
            }

            Error = STATAPI_SetPioMode(Drives[i].Handle,STATAPI_PIO_MODE_4);
            Error = SetTransferModePio(Drives[i].Handle,STATAPI_PIO_MODE_4);
#endif /* DMA_PRESENT */

            /* STATAPI_GetParams() */
            STTBX_Print(("%d.9 STATAPI_GetParams() \n",TestParams->Ref));
            STTBX_Print(("Purpose: Retrieve Device & driver Information\n"));
            {
                Error = STATAPI_GetParams(Drives[i].Handle,&DriverParams);

                STTBX_Print(("--------- Device Info ----------\n"));
                STTBX_Print(("-----DeviceAddress: %d \n",DriverParams.DeviceAddress));
                STTBX_Print(("-----SupportedPioModes: %8x\n",DriverParams.SupportedPioModes));
                STTBX_Print(("-----SupportedDmaModes: %8x \n",DriverParams.SupportedDmaModes));
                STTBX_Print(("-----CurrentPioMode: %d\n",DriverParams.CurrentPioMode));
                STTBX_Print(("-----CurrentDmaMode: %d \n",DriverParams.CurrentDmaMode));

                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to retrieve parameters");

                }else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            STTBX_Print(("%d.10 STATAPI_GetCapability() \n",TestParams->Ref));
            STTBX_Print(("Purpose: Retrieve only Driver Information\n"));
            {
                Error = STATAPI_GetCapability(AtapiDevName,&Capability);


                STTBX_Print(("--------- Driver info ----------\n"));
                STTBX_Print(("-----DeviceType: %d \n",Capability.DeviceType));
                STTBX_Print(("-----SupportedPioModes: %8x\n",Capability.SupportedPioModes));
                STTBX_Print(("-----SupportedDmaModes: %8x \n",Capability.SupportedDmaModes));

                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to retrieve capabilities");

                }else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            STTBX_Print(("%d.11 STATAPI_GetPioMode() \n",TestParams->Ref));
            STTBX_Print(("Purpose: Check that we retrieve the correct Pio mode\n"));
            {
                Error = STATAPI_GetPioMode(Drives[i].Handle,&Mode,&TimingParams);

                STTBX_Print(("Pio mode = %d\n",Mode));


                if (CheckCodeOk(Error) || (Mode != STATAPI_PIO_MODE_4))
                {
                    ATAPI_TestFailed(Result,"Unable to retrieve capabilities");

                }else
                {
                    ATAPI_TestPassed(Result);
                }
            }

#ifdef DMA_PRESENT
            STTBX_Print(("%d.11a STATAPI_GetDmaMode()\n", TestParams->Ref));
            STTBX_Print(("Purpose: check that we retrieve the correct DMA mode\n"));
            {
                Error = STATAPI_GetDmaMode(Drives[i].Handle, &DmaMode,
                                            &DmaTimingParams);
                STTBX_Print(("DMA Mode = %d\n", DmaMode));
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result, "Unable to retrieve mode");
                }
                else
                {
                    if (DmaMode == STATAPI_DMA_UDMA_MODE_4)
                        ATAPI_TestPassed(Result);
                }
            }
#endif
            /*  STATAPI_HardReset  */
            STTBX_Print(("%d.12 STATAPI_HardReset() \n",TestParams->Ref));
            STTBX_Print(("Purpose: to recover from a HardReset\n"));
            {
                ST_ErrorCode_t errortmp;

                Error = STATAPI_HardReset(AtapiDevName);

#if defined(ST_5512) || defined(ST_5514) || defined(ST_5528) || defined(ST_8010) ||\
((defined(ST_7100) || defined(ST_7109) || defined(ST_7200)) && (defined (SATA_SUPPORTED)))
                errortmp = ST_NO_ERROR;
#else
                errortmp = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif
                if (CheckCode(Error,errortmp))
                {
                    ATAPI_TestFailed(Result,"Unable to recover from a HardReset");
                }
                else
                {
#if defined(ST_5512) || defined(ST_5514) || defined(ST_5528) || defined(ST_8010) ||\
((defined(ST_7100) || defined(ST_7109) || defined(ST_7200)) && (defined (SATA_SUPPORTED)))
                    STOS_SemaphoreWait(StepSemaphore_p);
#endif
                    ATAPI_TestPassed(Result);
                }
            }

            /* STATAPI_SetPioMode */
            STTBX_Print(("%d.13 Set Pio mode 4 \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure we can set the Pio mode 4\n "));
            {
                Error = STATAPI_SetPioMode(Drives[i].Handle,STATAPI_PIO_MODE_4);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set Pio mode 4");

                }
                else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            Error = SetTransferModePio(Drives[i].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            /* Simple Read/Write Test */
            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.14 Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTest(Drives[i].Handle,&AddressTable[TestParams->Ref],3);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                STTBX_Print(("%d.14  Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {

                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                        goto api2_end;
                    }

                    ATAPI_TestPassed(Result);
                }
            }

#ifdef DMA_PRESENT
            /* Make sure the sectors are "empty" */
            ClearSectors(Drives[i].Handle, &AddressTable[TestParams->Ref], 1);
#if !defined(SATA_SUPPORTED)
            STTBX_Print(("%d.14b Purpose: to ensure we can set MWDMA mode 2\n",TestParams->Ref));
            {
                Error = STATAPI_SetDmaMode(Drives[i].Handle,
                                           STATAPI_DMA_MWDMA_MODE_2);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set MWDMA mode 2");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            Error = SetTransferModeDma(Drives[i].Handle, STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.14b Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTestDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref],
                                            1);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                STTBX_Print(("%d.14b  Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {

                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                    }

                    ATAPI_TestPassed(Result);
                }
            }
#endif/*SATA_SUPPORTED*/
            STTBX_Print(("%d.14 DMA mode tests \n",TestParams->Ref));
            STTBX_Print(("%d.14a Purpose: to ensure we can set UDMA mode 4\n",TestParams->Ref));
            {
                Error = STATAPI_SetDmaMode(Drives[i].Handle,STATAPI_DMA_UDMA_MODE_4);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set UDMA mode 4");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            Error = SetTransferModeDma(Drives[i].Handle, STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.14a Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTestDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref],
                                            1);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                STTBX_Print(("%d.14a  Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {

                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                    }

                    ATAPI_TestPassed(Result);
                }
            }
#endif

            /*  STATAPI_SoftReset  */
            STTBX_Print(("%d.15 STATAPI_SoftReset() \n",TestParams->Ref));
            STTBX_Print(("Purpose: to recover from a SoftReset\n"));
            {
                Error = STATAPI_SoftReset(AtapiDevName);
                STOS_SemaphoreWait(StepSemaphore_p);

                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to recover from a SoftReset");

                }else
                {
                    ATAPI_TestPassed(Result);
                }
            }


            /* STATAPI_SetPioMode */

            STTBX_Print(("%d.16 Set Pio mode 4 \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure we can set the Pio mode 4\n "));
            {
                Error = STATAPI_SetPioMode(Drives[i].Handle,STATAPI_PIO_MODE_4);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set Pio mode 4");

                }else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            Error = SetTransferModePio(Drives[i].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            /* Simple Read/Write Test */

            if (Drives[i].Type == STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.17 Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTest(Drives[i].Handle,&AddressTable[TestParams->Ref],3);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                        goto api2_end;
                    }
                    ATAPI_TestPassed(Result);
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                STTBX_Print(("%d.17  Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {
                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                        goto api2_end;
                    }

                    ATAPI_TestPassed(Result);
                }
            }

            if (Drives[i].Type == STATAPI_ATA_DRIVE)
            {
                U8 Stat, AltStat, Err;

                /* Simple Read/Write Test */
                STTBX_Print(("%d.17 Calling STATAPI_GetATAStatus\n",
                            TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can call this function\n"));
                Error = STATAPI_GetATAStatus(Drives[i].Handle,
                                             &Stat, &AltStat, &Err);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result, "Unable to get registers");
                    goto api2_end;
                }
                STTBX_Print(("Current status value: 0x%02x\n", Stat));
                STTBX_Print(("Current alt. status value: 0x%02x\n", AltStat));
                STTBX_Print(("Current error value: 0x%02x\n", Err));
                ATAPI_TestPassed(Result);
            }

#ifdef DMA_PRESENT
            ClearSectors(Drives[i].Handle, &AddressTable[TestParams->Ref], 1);
#if !defined(SATA_SUPPORTED)
            STTBX_Print(("%d.18 DMA mode tests \n",TestParams->Ref));
            STTBX_Print(("%d.18a Purpose: to ensure we can set MWDMA mode 2\n",TestParams->Ref));
            {
                Error = STATAPI_SetDmaMode(Drives[i].Handle,STATAPI_DMA_MWDMA_MODE_2);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set MWDMA mode 2");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            Error = SetTransferModeDma(Drives[i].Handle, STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.18a Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTestDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref],
                                            1);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                STTBX_Print(("%d.18a  Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {

                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
#endif/*SATA_SUPPORTED*/
            STTBX_Print(("%d.18b Purpose: to ensure we can set UDMA mode 4\n",TestParams->Ref));
            {
                Error = STATAPI_SetDmaMode(Drives[i].Handle,STATAPI_DMA_UDMA_MODE_4);
                if (CheckCodeOk(Error))
                {
                    ATAPI_TestFailed(Result,"Unable to set UDMA mode 4");
                    goto api2_end;
                }
                ATAPI_TestPassed(Result);

            }

            Error = SetTransferModeDma(Drives[i].Handle, STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
                goto api2_end;

            if (Drives[i].Type== STATAPI_ATA_DRIVE)
            {
                /* Simple Read/Write Test */

                STTBX_Print(("%d.18b Simple R/W Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
                {
                    Error = SimpleRWTestDma(Drives[i].Handle,
                                            &AddressTable[TestParams->Ref],
                                            1);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }
            else if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
            {
                STTBX_Print(("%d.17b  Read TOC Test \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
                {

                    Error = ReadTOCTest(Drives[i].Handle);
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read TOC from ATAPI device");
                    }
                    else
                    {
                        ATAPI_TestPassed(Result);
                    }
                }
            }

#endif

#if !defined (SATA_SUPPORTED)
#if defined(DMA_PRESENT)
            /* Do set/get timing tests; to be safe, these should be the last
             * tests done on the handle before it's closed and re-opened. Test
             * uses UDMA 4, hence ifdef.
             */
            STTBX_Print(("%d.19 Set/Get timing parameters\n", TestParams->Ref));
            Error = DoSetGetTimingTests(Drives[i].Handle);
            if (Error == ST_NO_ERROR)
            {
                ATAPI_TestPassed(Result);
            }
            else
            {
                ATAPI_TestFailed(Result, "timing parameter tests failed");
            }
#endif
#endif

        } /* end if (device present) */
    } /* end for (i = 0,1) */

api2_end:

    /* STATAPI_Term() */
    AtapiTermParams.ForceTerminate= TRUE;
    STTBX_Print(("%d.20 STATAPI_Term \n",TestParams->Ref));
    STTBX_Print(("Purpose:To ensure we can terminate the driver with an open connection \n"));
    {
        Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to terminate the driver");
        }else
        {
            ATAPI_TestPassed(Result);
        }
    }

    return Result;

}

#if !defined(STATAPI_NO_PARAMETER_CHECK)
/*****************************************************************
    ATAPI_TestAPIErrant

  This test is aimed to show how the driver is armored against lousy
  users that call the functions with bad parameters.
    - Init / Term
    - Open/ Close
    - GetPioMode
    - SetPioMode
    - CmdIn (READ_SECTOR )
    - CmdOut (WRITE_SECTOR)
    - CmdNoData (SET_MULIPLE_MODE)

  If This test is ran twice it test first the DEVICE 0 and then the DEVICE 1.

******************************************************************/
static ATAPI_TestResult_t ATAPI_TestAPIErrant(ATAPI_TestParams_t * TestParams)
{

    ST_ErrorCode_t  Error;
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;

    STATAPI_Params_t    DriverParams;
    STATAPI_PioMode_t Mode;
    STATAPI_PioTiming_t TimingParams;
    STATAPI_DmaMode_t   DmaMode;
    STATAPI_DmaTiming_t DmaTimingParams;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 NumberRW,BuffSize,i;
    U8 *Data_p;
    U8 *Data2_p;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp1_p,*Temp2_p;
#endif

    /* ATAPI driver parameters */
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;
    STATAPI_Handle_t        AtapiHandle = NULL;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    Temp1_p= (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,16*BLOCK_SIZE + 31);
    Temp2_p= (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,16*BLOCK_SIZE + 31);
    Data_p =(U8 *)(((U32)(Temp1_p) + 31) & ~31);
    Data2_p = (U8 *)(((U32)(Temp2_p) + 31) & ~31);
#else
    Data_p= (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,16*BLOCK_SIZE);
    Data2_p= (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,16*BLOCK_SIZE);
#endif

#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif

    AtapiInitParams.DriverPartition= TEST_PARTITION_1;
#if defined(ATAPI_FDMA) 
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#endif
    AtapiInitParams.BaseAddress= (U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress= (U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p  = (STATAPI_RegisterMap_t *)PATARegMasks;    
#endif
    
    strcpy(AtapiInitParams.EVTDeviceName,EVTDevName);
    STTBX_Print(("%d.0 STATAPI_SetRegisters, STATAPI_Init()\n",
                TestParams->Ref));
    STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

    /* We need to check this first, since if it fails we can't run any
     * other tests safely
     */
    Error = STATAPI_SetRegisters(NULL);
    if (CheckCode(Error, ST_ERROR_BAD_PARAMETER))
    {
        ATAPI_TestFailed(Result, "STATAPI_SetRegisters has a flaw");
        goto errant_end;
    }
    else
    {
        ATAPI_TestPassed(Result);
    }

    Error = STATAPI_GetRegisters(NULL);
    if (CheckCode(Error, ST_ERROR_BAD_PARAMETER))
    {
        ATAPI_TestFailed(Result, "STATAPI_GetRegisters has a flaw");
        goto errant_end;
    }
    else
    {
        ATAPI_TestPassed(Result);
    }

     /* STATAPI_Init() */
    /* Calling Init with bad parameters */
     {
      Error = STATAPI_Init(NULL, &AtapiInitParams);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Init has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }

      Error = STATAPI_Init(AtapiDevName, NULL);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Init has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }

      AtapiInitParams.DeviceType= 5;
      Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Init has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);

#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif
      }
      AtapiInitParams.DriverPartition= NULL;
#if defined(STATAPI_SET_EMIREGISTER_MAP)      
      AtapiInitParams.RegisterMap_p  = (STATAPI_RegisterMap_t *)PATARegMasks;   
#endif      
      Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Init has a flaw");
          goto errant_end;
      }else
      {
        ATAPI_TestPassed(Result);
        AtapiInitParams.DriverPartition= TEST_PARTITION_1;
      }
      AtapiInitParams.BaseAddress= NULL;
      Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Init has a flaw");
          goto errant_end;
      }else
      {
        ATAPI_TestPassed(Result);
        AtapiInitParams.BaseAddress= (U32 *)ATA_BASE_ADDRESS;
      }

      AtapiInitParams.HW_ResetAddress= NULL;
      Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Init has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
       AtapiInitParams.HW_ResetAddress= (U16 *)ATA_HRD_RST;
      }

      strcpy(AtapiInitParams.EVTDeviceName,"JoeDoe/0");
      Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Init has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
       strcpy(AtapiInitParams.EVTDeviceName,EVTDevName);
      }
     
#if defined(STATAPI_SET_EMIREGISTER_MAP)      
      AtapiInitParams.RegisterMap_p  = (STATAPI_RegisterMap_t *)PATARegMasks;   
#endif      
      /*Finally we initiaize corerctly the Driver*/
      Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
      if (CheckCodeOk(Error))
      {
          ATAPI_TestFailed(Result,"Could not init the driver");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
      AtapiTermParams.ForceTerminate = FALSE;
      Error = STATAPI_Term(AtapiDevName, &AtapiTermParams);
      if (CheckCodeOk(Error))
      {
          ATAPI_TestFailed(Result, "Could not terminate the driver");
          goto errant_end;
      }
      /* Check that interface reset returns NO error, and that we can
       * initialise afterwards.
       */
      Error = STATAPI_ResetInterface();
      if (CheckCodeOk(Error))
      {
          ATAPI_TestFailed(Result, "Could not reset interface");
          goto errant_end;
      }
      else
      {
          ATAPI_TestPassed(Result);
      }
      /* Re-init driver */
      Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
      if (CheckCodeOk(Error))
      {
          ATAPI_TestFailed(Result,"Could not init the driver after interface reset");
          goto errant_end;
      }
      else
      {
          ATAPI_TestPassed(Result);
      }
#else
      Error = STATAPI_ResetInterface();
      if (CheckCode(Error, ST_ERROR_FEATURE_NOT_SUPPORTED))
      {
          ATAPI_TestFailed(Result,"STATAPI_ResetInterface has a flaw");
          goto errant_end;
      }
      else
      {
          ATAPI_TestPassed(Result);
      }
#endif

      /* Try to initialize twice */

      Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
      if (CheckCode(Error,ST_ERROR_ALREADY_INITIALIZED))
      {
          ATAPI_TestFailed(Result,"STATAPI_Init has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }

    }/*================= end of Init Faulty test ==========================*/
     STTBX_Print(("%d.1 Miscellaneous  \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure these functions needs a open handle to work \n"));

      {
          U8 Stat, AltStat, Err;

          STTBX_Print((" STATAPI_GetATAStatus \n"));
          Error = STATAPI_GetATAStatus(AtapiHandle, &Stat, &AltStat, &Err);
          if (CheckCode(Error, ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result, "STATAPI_GetATAStatus has a flaw");
              goto errant_end;
          }
          else
          {
              ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_GetPioMode \n"));
          Error = STATAPI_GetPioMode(AtapiHandle,&Mode,&TimingParams);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_GetDmaMode \n"));
          Error = STATAPI_GetDmaMode(AtapiHandle,&DmaMode,&DmaTimingParams);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetDmaMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

         /*After Open a connection and set the PioMode we can call this function.
            See you later.. */


         /*   Lets try to set the mode of a not opened handle  */
          STTBX_Print((" STATAPI_SetPioMode \n"));
          Mode= STATAPI_PIO_MODE_4;
          Error = STATAPI_SetPioMode(AtapiHandle,Mode);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_SetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_SetDmaMode \n"));
          Mode= STATAPI_DMA_UDMA_MODE_5;
          Error = STATAPI_SetDmaMode(AtapiHandle,Mode);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_SetDmaMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_Abort \n"));
          Error = STATAPI_Abort(AtapiHandle);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_Abort has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_Close \n"));
          Error = STATAPI_Close(AtapiHandle);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_Close has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_GetParams \n"));
          Error = STATAPI_GetParams(AtapiHandle,&DriverParams);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetParams has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_CmdNoData \n"));
          Error = STATAPI_CmdNoData(AtapiHandle,&Cmd,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdNoData has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_CmdIn \n"));
          Error = STATAPI_CmdIn(AtapiHandle,&Cmd,Data_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_CmdOut \n"));
          Error = STATAPI_CmdOut(AtapiHandle,&Cmd,Data_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          STTBX_Print((" Checking STATAPI_GetATAStatus rejects NULL params:\n"));
          STTBX_Print(("    Checking Stat...\n"));
          Error = STATAPI_GetATAStatus(AtapiHandle, NULL, &AltStat, &Err);
          if (CheckCode(Error, ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result, "STATAPI_GetATAStatus has a flaw");
              goto errant_end;
          }
          else
          {
              ATAPI_TestPassed(Result);
          }

          STTBX_Print(("    Checking AltStat...\n"));
          Error = STATAPI_GetATAStatus(AtapiHandle, &Stat, NULL, &Err);
          if (CheckCode(Error, ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result, "STATAPI_GetATAStatus has a flaw");
              goto errant_end;
          }
          else
          {
              ATAPI_TestPassed(Result);
          }

          STTBX_Print(("    Checking Err...\n"));
          Error = STATAPI_GetATAStatus(AtapiHandle, &Stat, &AltStat, NULL);
          if (CheckCode(Error, ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result, "STATAPI_GetATAStatus has a flaw");
              goto errant_end;
          }
          else
          {
              ATAPI_TestPassed(Result);
          }

      }
  

    /**************** Lets try now with STATAPI_Open ***********************/

    STTBX_Print(("%d.2 STATAPI_Open() \n",TestParams->Ref));
    STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

    {
      Error = STATAPI_Open("JoeDoe/0", &AtapiOpenParams,&AtapiHandle);
      if (CheckCode(Error,ST_ERROR_UNKNOWN_DEVICE))
      {
          ATAPI_TestFailed(Result,"STATAPI_Open has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }

      Error = STATAPI_Open(AtapiDevName, NULL,&AtapiHandle);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Open has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }

      Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,NULL);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Open has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }

      AtapiOpenParams.DeviceAddress= 3;
      Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&AtapiHandle);
      if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
      {
          ATAPI_TestFailed(Result,"STATAPI_Open has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
       AtapiOpenParams.DeviceAddress= TestParams->DevAddress;
      }
      Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&AtapiHandle);
      if (CheckCodeOk(Error))
      {
          ATAPI_TestFailed(Result,"Couldn't open a connection");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }

      /* Try to open twice */
      Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&AtapiHandle);
      if (CheckCode(Error,ST_ERROR_NO_FREE_HANDLES))
      {
          ATAPI_TestFailed(Result,"STATAPI_Open has a flaw");
          goto errant_end;
      }else
      {
       ATAPI_TestPassed(Result);
      }
     }/*================= end of Open Faulty test ==========================*/

    /*********** Lets try now with STATAPI_GetPioMode and Commands ***********/
     STTBX_Print(("%d.4 Commands and GetPioMode \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the Pio Mode must be set before commands\n"));

          STTBX_Print((" STATAPI_CmdNoData \n"));
          Error = STATAPI_CmdNoData(AtapiHandle,&Cmd,&CmdStatus);
          if (CheckCode(Error,STATAPI_ERROR_MODE_NOT_SET))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdNoData has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_CmdIn \n"));
          Error = STATAPI_CmdIn(AtapiHandle,&Cmd,Data_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,STATAPI_ERROR_MODE_NOT_SET))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          STTBX_Print((" STATAPI_CmdOut \n"));
          Error = STATAPI_CmdOut(AtapiHandle,&Cmd,Data_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,STATAPI_ERROR_MODE_NOT_SET))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

         Error = STATAPI_GetPioMode(AtapiHandle,&Mode,&TimingParams);
          if (CheckCode(Error,STATAPI_ERROR_MODE_NOT_SET))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          Error = STATAPI_GetDmaMode(AtapiHandle, &DmaMode, &DmaTimingParams);
          if (CheckCode(Error, STATAPI_ERROR_MODE_NOT_SET))
          {
            ATAPI_TestFailed(Result, "STATAPI_GetDmaMode has a flaw");
            goto errant_end;
          }
          else
          {
            ATAPI_TestPassed(Result);
          }

         /*After set the PioMode we can finally call this function and succeed.
            See you later.. */

 /**************** Lets try now with STATAPI_SetPioMode ***********************/


     STTBX_Print(("%d.5 STATAPI_SetPioMode() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {
          Mode=7;
          Error = STATAPI_SetPioMode(AtapiHandle,Mode);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_SetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Mode= STATAPI_PIO_MODE_4;
          Error = STATAPI_SetPioMode(NULL,Mode);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_SetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Mode= STATAPI_PIO_MODE_3;
          Error = STATAPI_SetPioMode(AtapiHandle,Mode);
          if (CheckCode(Error,ST_ERROR_FEATURE_NOT_SUPPORTED))
          {
              ATAPI_TestFailed(Result,"STATAPI_SetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Mode= STATAPI_PIO_MODE_4;
          Error = STATAPI_SetPioMode(AtapiHandle,Mode);
          if (CheckCodeOk(Error))
          {
              ATAPI_TestFailed(Result,"Couldn't set the Pio Mode ");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
      } /*================= end of SetPioMode Faulty test ==========================*/

    /**************** Lets try again with STATAPI_GetPioMode ***********************/


     STTBX_Print(("%d.6 STATAPI_GetPioMode()\n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {

          Error = STATAPI_GetPioMode(AtapiHandle,NULL,&TimingParams);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          Error = STATAPI_GetPioMode(AtapiHandle,&Mode,NULL);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          Error = STATAPI_GetPioMode(NULL,&Mode,&TimingParams);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          Error = STATAPI_GetPioMode(AtapiHandle,&Mode,&TimingParams);
          if (CheckCodeOk(Error))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetPioMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

      } /*================= end of GetPioMode Faulty test ==========================*/

      /**************** Lets try again with STATAPI_GetDmaMode ***********************/

     STTBX_Print(("%d.6 STATAPI_GetDmaMode()\n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));
      {

          Error = STATAPI_GetDmaMode(AtapiHandle,NULL,&DmaTimingParams);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetDmaMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          Error = STATAPI_GetDmaMode(AtapiHandle,(STATAPI_DmaMode_t*)&DmaMode,NULL);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetDmaMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          Error = STATAPI_GetDmaMode(NULL,(STATAPI_DmaMode_t*)&Mode,&DmaTimingParams);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetDmaMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_GetDmaMode(AtapiHandle,&DmaMode,&DmaTimingParams);
          if (CheckCodeOk(Error))
          {
              ATAPI_TestFailed(Result,"STATAPI_GetDmaMode has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
      }
      /*================= end of GetDmaMode Faulty test ==========================*/

     /**************** Lets try again with STATAPI_CmdNoData ***********************/


     STTBX_Print(("%d.7 STATAPI_CmdNoData() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {
          Cmd.CmdCode= STATAPI_CMD_SET_MULTIPLE_MODE;
          Cmd.SectorCount=2;

         Error = STATAPI_CmdNoData(AtapiHandle,NULL,&CmdStatus);
         if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdNoData has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
         Error = STATAPI_CmdNoData(AtapiHandle,&Cmd,NULL);
         if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdNoData has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          Error = STATAPI_CmdNoData(NULL,&Cmd,&CmdStatus);
         if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdNoData has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_CmdNoData(AtapiHandle,&Cmd,&CmdStatus);
         if (CheckCodeOk(Error))
          {
              ATAPI_TestFailed(Result,"Couldn't issue the command");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          STOS_SemaphoreWait(StepSemaphore_p);

      } /*================= end of CmdNoData Faulty test ==========================*/

       /**************** Lets try again with STATAPI_CmdOut ***********************/


     STTBX_Print(("%d.8 STATAPI_CmdOut() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {
          Cmd.CmdCode= STATAPI_CMD_WRITE_SECTORS;
          Cmd.SectorCount=2;
          Cmd.Features=0x00;
          Cmd.UseLBA= AddressTable[0].LBA;
          Cmd.Sector= AddressTable[0].Value.CHS_Value.Sector;
          Cmd.CylinderLow=  AddressTable[0].Value.CHS_Value.CylLow;
          Cmd.CylinderHigh= AddressTable[0].Value.CHS_Value.CylHigh;
          Cmd.Head= AddressTable[0].Value.CHS_Value.Head;
          BuffSize=1024;

          Error = STATAPI_CmdOut(AtapiHandle,&Cmd,Data_p,BuffSize,&NumberRW,NULL);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_CmdOut(AtapiHandle,NULL,Data_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

           Error = STATAPI_CmdOut(AtapiHandle,&Cmd,NULL,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          Error = STATAPI_CmdOut(AtapiHandle,&Cmd,Data_p,BuffSize,NULL,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Cmd.CmdCode = STATAPI_CMD_WRITE_DMA_QUEUED;
          Error = STATAPI_CmdOut(AtapiHandle, &Cmd, Data_p, BuffSize,
                                 &NumberRW, &CmdStatus);
          if (CheckCode(Error, STATAPI_ERROR_PROTOCOL_NOT_SUPPORTED))
          {
              ATAPI_TestFailed(Result, "STATAPI_CmdOut has a flaw");
              goto errant_end;
          }
          else
          {
              ATAPI_TestPassed(Result);
          }
          Cmd.CmdCode = STATAPI_CMD_WRITE_SECTORS;

          /* Coherence check */
          BuffSize=512; /* only one sector, and in the params we ask to write 2*/

          Error = STATAPI_CmdOut(AtapiHandle,&Cmd,Data_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          BuffSize=1024; /* restore correct value 2 blocks size */

#if defined(ST_5514) || defined(ST_5528)
          /* Current HAL implementation on 5514 has a four-byte
           * alignment requirement even for PIO transfers, so we'll test
           * that.
           */
          Error = STATAPI_CmdOut(AtapiHandle, &Cmd, (U8 *)((U32)Data_p + 1),
                                 BuffSize, &NumberRW, &CmdStatus);
          if (CheckCode(Error, ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result, "STATAPI_CmdOut doesn't enforce alignment");
              /* If that succeeded it will have overwritten some memory,
               * so we can't trust any further results.
               */
              goto errant_end;
          }
          else
          {
              ATAPI_TestPassed(Result);
          }
#endif

          for(i=0;i<2*BLOCK_SIZE;i++)
          {
              Data_p[i]=i;
              Data2_p[i]=0x00; /* this is for the next test */
          }

         Error = STATAPI_CmdOut(AtapiHandle,&Cmd,Data_p,BuffSize,&NumberRW,&CmdStatus);
         STOS_SemaphoreWait(StepSemaphore_p);
          if (CheckCodeOk(Error))
          {
              ATAPI_TestFailed(Result,"Couldn't write 2 sectors");
              goto errant_end;
          }else
          {
               if (Drives[0].Type== STATAPI_ATA_DRIVE)
               {
                   if (EvtError !=ST_NO_ERROR)
                   {
                      ATAPI_TestFailed(Result,"Couldn't read 2 sectors");
                      goto errant_end;
                   }else
                   {
                       ATAPI_TestPassed(Result);
                   }
               }else {
                    if (EvtError == STATAPI_ERROR_CMD_ERROR)
                     ATAPI_TestPassed(Result);
               }
          }



      } /*================= end of CmdOut Faulty test ==========================*/


       /**************** Lets try  with STATAPI_CmdIn ***********************/

     STTBX_Print(("%d.9 STATAPI_CmdIn() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {
          Cmd.CmdCode= STATAPI_CMD_READ_SECTORS;
          Cmd.SectorCount=2;
          Cmd.Features=0x00;
          Cmd.UseLBA= AddressTable[0].LBA;
          Cmd.Sector= AddressTable[0].Value.CHS_Value.Sector;
          Cmd.CylinderLow=  AddressTable[0].Value.CHS_Value.CylLow;
          Cmd.CylinderHigh= AddressTable[0].Value.CHS_Value.CylHigh;
          Cmd.Head= AddressTable[0].Value.CHS_Value.Head;


          Error = STATAPI_CmdIn(AtapiHandle,&Cmd,Data2_p,BuffSize,&NumberRW,NULL);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_CmdIn(AtapiHandle,NULL,Data2_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

           Error = STATAPI_CmdIn(AtapiHandle,&Cmd,NULL,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
           Error = STATAPI_CmdIn(AtapiHandle,&Cmd,Data2_p,BuffSize,NULL,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Cmd.CmdCode = STATAPI_CMD_READ_DMA_QUEUED;
          Error = STATAPI_CmdIn(AtapiHandle, &Cmd, Data_p, BuffSize,
                                 &NumberRW, &CmdStatus);
          if (CheckCode(Error, STATAPI_ERROR_PROTOCOL_NOT_SUPPORTED))
          {
              ATAPI_TestFailed(Result, "STATAPI_CmdIn has a flaw");
              goto errant_end;
          }
          else
          {
              ATAPI_TestPassed(Result);
          }
          Cmd.CmdCode = STATAPI_CMD_READ_SECTORS;

          /* Coherence check */
          BuffSize=512; /* only one sector, and in the params we ask to write 2*/

          Error = STATAPI_CmdIn(AtapiHandle,&Cmd,Data2_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_CmdIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }


         BuffSize=1024; /* restore correct value: 2 blocks size */

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
          /* Current HAL implementation on 5514 has a four-byte
           * alignment requirement even for PIO transfers, so we'll test
           * that.
           */
          Error = STATAPI_CmdIn(AtapiHandle, &Cmd, (U8 *)((U32)Data2_p + 1),
                                 BuffSize, &NumberRW, &CmdStatus);
          if (CheckCode(Error, ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result, "STATAPI_CmdOut doesn't enforce alignment");
              /* If that succeeded it will have overwritten some memory,
               * so we can't trust any further results.
               */
              goto errant_end;
          }
          else
          {
              ATAPI_TestPassed(Result);
          }
#endif
         NumberRW=0;
         Error = STATAPI_CmdIn(AtapiHandle,&Cmd,Data2_p,BuffSize,&NumberRW,&CmdStatus);
          if (CheckCodeOk(Error))
          {
              ATAPI_TestFailed(Result,"Couldn't read 2 sectors");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
          STOS_SemaphoreWait(StepSemaphore_p);

          if (Drives[0].Type== STATAPI_ATA_DRIVE)
          {
            if (EvtError !=ST_NO_ERROR)
            {
                 ATAPI_TestFailed(Result,"Couldn't read 2 sectors");
                 goto errant_end;
            }else
            {
              CompareData (Data_p,Data2_p,2*BLOCK_SIZE);
              ATAPI_TestPassed(Result);
            }

          }else
          {

            if (CheckCode(EvtError,STATAPI_ERROR_CMD_ERROR))
            {
                  ATAPI_TestFailed(Result,"STATAPI_CmdIn has a flaw");
                  goto errant_end;
              }else
              {
                   ATAPI_TestPassed(Result);
              }
            }


      } /*================= end of CmdIn Faulty test ==========================*/

        /**************** Lets try  with STATAPI_PacketIn ***********************/

    if (Drives[0].Type== STATAPI_ATAPI_DRIVE)
    {
     STTBX_Print(("%d.10 STATAPI_PacketIn() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {
          STATAPI_Packet_t Pkt;
          STATAPI_PktStatus_t PktStatus;

          BuffSize=8*K;
          memset(Pkt.Data,0,15);
          Pkt.OpCode= STATAPI_PKT_READ_TOC_PMA_ATIP;
          Pkt.Data[0]=0x02;
          Pkt.Data[6]=0x10;
          Pkt.Data[7]=0x00;
          Pkt.Data[8]=0x80;

          Error = STATAPI_PacketIn(AtapiHandle,&Pkt,Data_p,BuffSize,&NumberRW,NULL);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PktIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_PacketIn(AtapiHandle,NULL,Data_p,BuffSize,&NumberRW,&PktStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PktIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

           Error = STATAPI_PacketIn(AtapiHandle,&Pkt,NULL,BuffSize,&NumberRW,&PktStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PktIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
           Error = STATAPI_PacketIn(AtapiHandle,&Pkt,Data_p,BuffSize,NULL,&PktStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PktIn has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

#if defined(ATAPI_BMDMA)|| defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
          STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
          STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
          STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
          STOS_MemoryDeallocate(TEST_PARTITION_1,Data2_p);
#endif


      } /*================= end of PacketIn Faulty test ==========================*/
      STTBX_Print(("%d.11 STATAPI_PacketOut() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {
          STATAPI_Packet_t Pkt;
          STATAPI_PktStatus_t PktStatus;

          BuffSize=8*K;
          memset(Pkt.Data,0,15);
          Pkt.OpCode= STATAPI_PKT_WRITE_AND_VERIFY_10;


          Error = STATAPI_PacketOut(AtapiHandle,&Pkt,Data_p,BuffSize,&NumberRW,NULL);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PacketOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_PacketOut(AtapiHandle,NULL,Data_p,BuffSize,&NumberRW,&PktStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PacketOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

           Error = STATAPI_PacketOut(AtapiHandle,&Pkt,NULL,BuffSize,&NumberRW,&PktStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PacketOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
           Error = STATAPI_PacketOut(AtapiHandle,&Pkt,Data_p,BuffSize,NULL,&PktStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PacketOut has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

      } /*================= end of PacketOut Faulty test ==========================*/
      STTBX_Print(("%d.12 STATAPI_PacketNoData() \n",TestParams->Ref));
      STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {
          STATAPI_Packet_t Pkt;
          STATAPI_PktStatus_t PktStatus;

          BuffSize=8*K;
          memset(Pkt.Data,0,15);
          Pkt.OpCode= STATAPI_PKT_SET_READ_AHEAD;


          Error = STATAPI_PacketNoData(AtapiHandle,&Pkt,NULL);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PktNoData has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_PacketNoData(AtapiHandle,NULL,&PktStatus);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_PktNoData has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }



      } /*================= end of PacketNoData Faulty test ==========================*/
     }
      /**************** Lets try  with STATAPI_Term ***********************/

     STTBX_Print(("%d.13 STATAPI_Term() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure that if there is any open handle it return an error\n"));

     {
          AtapiTermParams.ForceTerminate=FALSE;
          Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
          if (CheckCode(Error,ST_ERROR_OPEN_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_Term has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }
         /* After closing the Handle then we can terminate the Driver.
            See you later.
         */

      } /*================= end of Term Faulty test ==========================*/


    /**************** Lets try  with STATAPI_Close ***********************/


     STTBX_Print(("%d.14 STATAPI_Close() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure the function return the expected error codes\n"));

      {
          Error = STATAPI_Close(NULL);
          if (CheckCode(Error,ST_ERROR_INVALID_HANDLE))
          {
              ATAPI_TestFailed(Result,"STATAPI_Close has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_Close(AtapiHandle);
          if (CheckCodeOk(Error))
          {
              ATAPI_TestFailed(Result,"Couldn't close the handle");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }


      } /*================= end of CmdIn Faulty test ==========================*/

      /**************** Lets try again with STATAPI_Term ***********************/


     STTBX_Print(("%d.14 STATAPI_Term() \n",TestParams->Ref));
     STTBX_Print(("Purpose: To ensure that if there is any open handle it return an error\n"));

     {

          Error = STATAPI_Term("JoeDoe",&AtapiTermParams);
          if (CheckCode(Error,ST_ERROR_UNKNOWN_DEVICE))
          {
              ATAPI_TestFailed(Result,"STATAPI_Term has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          Error = STATAPI_Term(AtapiDevName,NULL);
          if (CheckCode(Error,ST_ERROR_BAD_PARAMETER))
          {
              ATAPI_TestFailed(Result,"STATAPI_Term has a flaw");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

          AtapiTermParams.ForceTerminate=FALSE;
          Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
          if (CheckCodeOk(Error))
          {
              ATAPI_TestFailed(Result,"Couldn't terminate the driver ");
              goto errant_end;
          }else
          {
               ATAPI_TestPassed(Result);
          }

      } /*================= end of Term Faulty test ==========================*/
errant_end:

    return Result;

}
#endif

#if defined(TEST_TWO_DEVICE)
static void PATATask(ATAPI_TaskParams_t *Params_p)
{
    ST_ErrorCode_t  	Error  = ST_NO_ERROR;
    ATAPI_TestResult_t  Result = TEST_RESULT_ZERO;
    /* ATAPI driver parameters */
    STATAPI_InitParams_t    PATAAtapiInitParams;
    STATAPI_TermParams_t    PATAAtapiTermParams;
    STATAPI_OpenParams_t    PATAAtapiOpenParams;
    STATAPI_DriveType_t     Type;
    static ATAPI_DeviceInfo_t      PATAHardInfo;
    STATAPI_Handle_t        PATAHandle = NULL;
    U32 					NumberRW,BuffSize;
    U8					    *Data_p,i;
    STATAPI_Cmd_t      	    Cmd;
    STATAPI_CmdStatus_t 	CmdStatus;
    STATAPI_Packet_t        Pkt;
    STATAPI_PktStatus_t     PktStatus;
    U32 AtapiBlockSize;

    Data_p= (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BLOCK_SIZE);

    if (Data_p == NULL)
    {
        ATAPI_TestFailed(Result, "Not enough memory for transfer buffer");
    }
#if defined(DMA_PRESENT)
   PATAAtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
   PATAAtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif /* SATA_SUPPORTED */

    PATAAtapiInitParams.BaseAddress= (U32 *)0xA2800000;
    PATAAtapiInitParams.HW_ResetAddress= (U16 *)0xA2800000;
    PATAAtapiInitParams.InterruptNumber = OS21_INTERRUPT_IRL_ENC_7;
    PATAAtapiInitParams.InterruptLevel = 5;
    PATAAtapiInitParams.ClockFrequency = ClockFrequency;
    
    strcpy(PATAAtapiInitParams.EVTDeviceName,EVTDevName);

    PATAAtapiInitParams.DriverPartition= TEST_PARTITION_1;
#if defined(ATAPI_FDMA) 
    PATAAtapiInitParams.NCachePartition = TINY_PARTITION_1;
#endif
    STTBX_Print(("PATA STATAPI_Init() \n"));
    Error = STATAPI_Init("ATAPI0", &PATAAtapiInitParams);
	if (CheckCodeOk(Error))
	{
	    ATAPI_TestFailed(Result,"Unable to initialize ATAPI device");
	    STTBX_Print(("Connect PATA drive first\n"));
        goto test_end;
	}
	ATAPI_TestPassed(Result);
	
	
	 PATAAtapiOpenParams.DeviceAddress= STATAPI_DEVICE_0;
     STTBX_Print((" STATAPI_Open() \n"));
     STTBX_Print(("Purpose: to ensure a ATAPI device connection can be opened\n"));
     {
	        Error = STATAPI_Open("ATAPI0",&PATAAtapiOpenParams,&PATAHandle);
	        if (CheckCodeOk(Error))
	        {
	            ATAPI_TestFailed(Result,"Unable to open ATAPI device");
	            goto test_end;

	        }
	        ATAPI_TestPassed(Result);

     }

  	  /* STATAPI_SetPioMode */
   	  STTBX_Print((" Set Pio mode 4 \n"));
   	  STTBX_Print(("Purpose: to ensure we can set the Pio mode 4\n "));
      {
	        Error = STATAPI_SetPioMode(PATAHandle,STATAPI_PIO_MODE_4);
	        if (CheckCodeOk(Error))
	        {
	            ATAPI_TestFailed(Result,"Unable to set Pio mode 4");
				goto test_end;
	        }
	        ATAPI_TestPassed(Result);
       }
       
       Error = STATAPI_GetDriveType(PATAHandle,&Type);
	   
	   if (Type == STATAPI_ATAPI_DRIVE)
	   {
	      Cmd.CmdCode= STATAPI_CMD_IDENTIFY_PACKET_DEVICE;
	      STTBX_Print(("Device found at address 1 ......ATAPI\n"));
	   }
	   else
	   {
	      Cmd.CmdCode= STATAPI_CMD_IDENTIFY_DEVICE;
	      STTBX_Print(("Device found at address 1 ......ATA\n"));
	   }
     
	   Cmd.SectorCount=0;
	   Cmd.Features=0x00;
	   Cmd.UseLBA= FALSE;
	   Cmd.Sector= 0x00;
	   Cmd.CylinderLow= 0x00;
	   Cmd.CylinderHigh= 0x00;
	   Cmd.Head= 0x00;
	   BuffSize= BLOCK_SIZE;
	   
        Error = STATAPI_CmdIn(PATAHandle, &Cmd, Data_p,
                              BuffSize, &NumberRW, &CmdStatus);
	    STOS_SemaphoreWait(StepSemaphore_p);
	    if (CheckCodeOk(Error))
	    {
	         ATAPI_TestFailed(Result,"Couldn't receive the identify info ");
	         goto test_end;
	
	     }else
	     {
	          ATAPI_TestPassed(Result);
	     }

        STTBX_Print(("------------------------------------------------------\n"));
        ParseAndShow(&PATAHardInfo,Data_p);
        STTBX_Print(("PATA Simple R/W Test \n"));
        
        if (Type == STATAPI_ATA_DRIVE)               
        {
	        STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
	        {
	            Error = SimpleRWTest(PATAHandle,
	                                &AddressTable[0],
	                                 10);
	            if (CheckCodeOk(Error))
	            {
	                    ATAPI_TestFailed(Result,"Unable to Read/Write ATAPI device");
	                    goto test_end;
	            }
	            ATAPI_TestPassed(Result);
	        }
	    }
	    else        
        {
	        U8 repeat=0;
	        STTBX_Print((" \n"));
	        STTBX_Print(("Simple R/W Test \n"));
	        STTBX_Print(("Purpose: to ensure we can read data from the device \n"));
	 		do
        	{

	           memset( Pkt.Data, 0, 15 );
	           Pkt.OpCode= STATAPI_PKT_READ_CAPACITY;
	
	           Error = STATAPI_PacketIn(PATAHandle, &Pkt,Data_p,
	                                    4096,&NumberRW,&PktStatus);
	
	            if (Error != ST_NO_ERROR)
	            {
	                STTBX_Print((" Read Capacity failed \n"));
	            }	
	            STOS_SemaphoreWait(StepSemaphore_p);
	            if (EvtError ==ST_NO_ERROR)
	            {
	               STTBX_Print(("*** Number of retries: %d \n",repeat));
	               STTBX_Print(("### Bytes written: %d \n",NumberRW));
	               /*memcpy(&LastLBA,Data_p,4);*/
	               LastLBA= Data_p[0]<<24
	                       |Data_p[1]<<16
	                       |Data_p[2]<<8
	                       |Data_p[3];
	               STTBX_Print(("--- Last LBA addressable: 0x%x\n",LastLBA));
	               AtapiBlockSize=Data_p[4]<<24
	                       |Data_p[5]<<16
	                       |Data_p[6]<<8
	                       |Data_p[7];
	              STTBX_Print(("--- Block Length: %d\n",AtapiBlockSize));
	
	               STTBX_Print((" \n"));
	
	               ATAPI_TestPassed(Result);
	               break;
	            }
	            else
	            {
	
	               /* Request sense key */
	               memset( Pkt.Data, 0, 15 );
	               Pkt.OpCode= STATAPI_PKT_REQUEST_SENSE;
	               Pkt.Data[3]=0x20,
	
	               Error = STATAPI_PacketIn(PATAHandle, &Pkt,Data_p,
	                                    4096,&NumberRW,&PktStatus);
	                if (Error != ST_NO_ERROR)
	                {
	                    STTBX_Print((" Request sense key  Failed \n"));
	                    
	                }
	
	                STOS_SemaphoreWait(StepSemaphore_p);
	                if (Data_p[2]&0x0F)   repeat++;
	                if (repeat>30)
	                {
	                    STTBX_Print((" Request Sense failed, too many tries: %d \n",repeat));
	                    ATAPI_TestFailed(Result,"Request Sense Failed ");
	                    break;
	                }
	            }
				task_delay ( 200000 );
         	 }while(TRUE);
	
	       	/*For testing disc read/write in pio mode */
			if ( LastLBA > 0x100000 )
			{
				StartSector = 16;
				NoOfSectors	= 4;
				Error = SimpleRTestDVD(PATAHandle,StartSector, NoOfSectors );
	
	            if (CheckCodeOk(Error))
	            {
	                ATAPI_TestFailed(Result,"Unable to Read(DVD) ATAPI device");
	                
	            }
	            ATAPI_TestPassed(Result);
	
			 }
			 else
			 {
				StartSector = 160;
				NoOfSectors	= 4;
				Error = SimpleRTestCD(PATAHandle,StartSector, NoOfSectors );
	            if (CheckCodeOk(Error))
	            {
	                ATAPI_TestFailed(Result,"Unable to Read(CD) ATAPI device");
	                
	            }
	            ATAPI_TestPassed(Result);
			 }
	    }
test_end:
        /* Get final statistics */
        Params_p->Error = Error;
	    Error = STATAPI_Close(PATAHandle);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Close drive %d Failed \n"));
        }
        
        PATAAtapiTermParams.ForceTerminate= FALSE;
        Error = STATAPI_Term("ATAPI0",&PATAAtapiTermParams);
	    if (Error != ST_NO_ERROR)
	    {
	        STTBX_Print(("Term Failed \n"));
	        ATAPI_TestFailed(Result, "Identify Device Test Failed ");
	    }
	    else
	    {
	        ATAPI_TestPassed(Result);
	    }
}  

static void SATATask(ATAPI_TaskParams_t *Params_p)
{
    ST_ErrorCode_t  	Error = ST_NO_ERROR;
    U32 				NumberRead,BuffSize2,j,NumberWrite,i,BlockNo;
    ATAPI_TestResult_t  Result = TEST_RESULT_ZERO;
    U16 				SectorCount;
    ATAPI_Address_t 	*Addr;
    U8 					*Data2_p;
    U8 					*DataTest1_p = NULL, *store1;
    U8 					*DataTest2_p = NULL, *store2;
    static    		     ATAPI_DeviceInfo_t      SATAHardInfo;
    /* ATAPI driver parameters */
    STATAPI_InitParams_t    SATAAtapiInitParams;
    STATAPI_TermParams_t    SATAAtapiTermParams;
    STATAPI_OpenParams_t    SATAAtapiOpenParams;
    STATAPI_Handle_t   	    SATAHandle = NULL;
    STATAPI_Cmd_t       	Cmd;
    STATAPI_CmdStatus_t 	CmdStatus;
 
    Data2_p= (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BLOCK_SIZE);

    SATAAtapiInitParams.DeviceType		=  STATAPI_SATA;
    SATAAtapiInitParams.BaseAddress		= (U32 *)0xB9209000;
    SATAAtapiInitParams.HW_ResetAddress	= (U16 *)0xB9209000;
    SATAAtapiInitParams.InterruptNumber = OS21_INTERRUPT_SATA;
    SATAAtapiInitParams.InterruptLevel 	= ATA_INTERRUPT_LEVEL;
    SATAAtapiInitParams.ClockFrequency  = ClockFrequency;
    strcpy(SATAAtapiInitParams.EVTDeviceName,EVTDevName);
    
    SATAAtapiInitParams.DriverPartition= TEST_PARTITION_1;

    Error = STATAPI_Init("SATA", &SATAAtapiInitParams);
	if (CheckCodeOk(Error))
	{
		STTBX_Print(("Need to Connect SATA drive \n"));
        goto test2_end;
	}
    ATAPI_TestPassed(Result);
	
	 SATAAtapiOpenParams.DeviceAddress= STATAPI_DEVICE_0;
     STTBX_Print(("STATAPI_Open() \n"));
     STTBX_Print(("Purpose: to ensure a SATA device connection can be opened\n"));
     {
	        Error = STATAPI_Open("SATA",&SATAAtapiOpenParams,&SATAHandle);
	        if (CheckCodeOk(Error))
	        {
	            ATAPI_TestFailed(Result,"Unable to open ATAPI device");
	            goto test2_end;

	        }
	        ATAPI_TestPassed(Result);
      }
  	  /* STATAPI_SetPioMode */
   	  STTBX_Print((" Set Pio mode 4 \n"));
   	  STTBX_Print(("Purpose: to ensure we can set the Pio mode 4\n "));
      {
	        Error = STATAPI_SetPioMode(SATAHandle,STATAPI_PIO_MODE_4);
	        if (CheckCodeOk(Error))
	        {
	            ATAPI_TestFailed(Result,"Unable to set Pio mode 4");
				goto test2_end;
	        }
	        ATAPI_TestPassed(Result);
       }
       Cmd.CmdCode= STATAPI_CMD_IDENTIFY_DEVICE;
	      
	   Cmd.SectorCount=0;
	   Cmd.Features=0x00;
	   Cmd.UseLBA= FALSE;
	   Cmd.Sector= 0x00;
	   Cmd.CylinderLow= 0x00;
	   Cmd.CylinderHigh= 0x00;
	   Cmd.Head= 0x00;
	   BuffSize2= BLOCK_SIZE;
	   
        Error = STATAPI_CmdIn(SATAHandle, &Cmd, Data2_p,
                              BuffSize2, &NumberRead, &CmdStatus);
        STOS_SemaphoreWait(StepSemaphore2_p);
        if (CheckCodeOk(Error))
        {
             ATAPI_TestFailed(Result,"Couldn't receive the identify info ");
             goto test2_end;

         }else
         {
              ATAPI_TestPassed(Result);
         }

        STTBX_Print(("------------------------------------------------------\n"));
        ParseAndShow(&SATAHardInfo,Data2_p);
       
       /* SectorCount is more readable. And can be changed if we want to
     	* do stuff with BlockNo == 0 (== 256 sectors)
     	*/
    	SectorCount = 36;

    	/* Allocate from ncache region */
    	store1 = STOS_MemoryAllocate(TINY_PARTITION_1,
        	                     (SectorCount * BLOCK_SIZE) + 255);
	    if (store1 == NULL)
	    {
	        STTBX_Print(("Failed to allocate store1\n"));
	    }
	    store2 = STOS_MemoryAllocate(TINY_PARTITION_1,
	                             (SectorCount * BLOCK_SIZE) + 255);
	    if (store2 == NULL)
	    {
	        STTBX_Print(("Failed to allocate store2\n"));
	        STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
	    }

	    /* Align to 256byte boundaries; only needs to be 128, really */
	    DataTest1_p = (U8 *)(((U32)store1 + 255) & 0xffffff00);
	    DataTest2_p = (U8 *)(((U32)store2 + 255) & 0xffffff00);
	
	    for (i = 0; i < SectorCount * BLOCK_SIZE; i++)
	    {
	        DataTest1_p[i] = i;
	        DataTest2_p[i] = 0x00;
	    }
	  	  /* STATAPI_SetPioMode */
   	 	STTBX_Print((" Set UDMA mode 4 \n"));
   	 	STTBX_Print(("Purpose: to ensure we can set the UDMA mode 4\n "));
      	{
	        Error = STATAPI_SetDmaMode(SATAHandle,STATAPI_DMA_UDMA_MODE_4);
	        if (CheckCodeOk(Error))
	        {
	            ATAPI_TestFailed(Result,"Unable to set UDMA mode 4");
	            goto test2_end;

	        }
	        ATAPI_TestPassed(Result);
       	}
	    Cmd.Features = 0x00;
	    Addr=&AddressTable[4];
	    Cmd.UseLBA = TRUE;
	    if (Cmd.UseLBA)
	    {
	        Cmd.LBA = (U32)Addr->Value.LBA_Value;
	    }
	    else
	    {
	        Cmd.Sector = Addr->Value.CHS_Value.Sector;
	        Cmd.CylinderLow =  Addr->Value.CHS_Value.CylLow;
	        Cmd.CylinderHigh = Addr->Value.CHS_Value.CylHigh;
	        Cmd.Head = Addr->Value.CHS_Value.Head;
	    }
	    Cmd.SectorCount = 36;
	    Cmd.CmdCode = STATAPI_CMD_WRITE_DMA;

    /* UDMA writes don't work on this cut, so we skip them */
#ifdef RW_INFO
        STTBX_Print(("Writing Data  : \n"));
#endif
        Error = STATAPI_CmdOut(SATAHandle, &Cmd, DataTest1_p,
                               SectorCount * BLOCK_SIZE,
                               &NumberWrite, &CmdStatus);
        if (Error != ST_NO_ERROR)
        {
            STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
            STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
        }

        STOS_SemaphoreWait(StepSemaphore2_p);

        if ((NumberWrite != (SectorCount * BLOCK_SIZE)) ||
            (EvtError != ST_NO_ERROR))
        {
            U8 ExtError;

            STTBX_Print(("Didn't write all bytes; wrote %i, wanted %i (error %u)\n",
                        NumberWrite, (SectorCount * BLOCK_SIZE), EvtError));
            STATAPI_GetExtendedError(SATAHandle, &ExtError);
            STTBX_Print(("Extended error code: 0x%02x\n", ExtError));
            STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
            STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
        }

#ifdef RW_INFO
    ShowStatus(&CmdStatus);
    STTBX_Print(("Reading Data  :  \n"));
#endif

    Cmd.SectorCount = 36;
    Cmd.CmdCode = STATAPI_CMD_READ_DMA;
    NumberWrite = 0;

    Error = STATAPI_CmdIn(SATAHandle, &Cmd, DataTest2_p, SectorCount * BLOCK_SIZE,
                          &NumberWrite, &CmdStatus);
    if (Error != ST_NO_ERROR)
    {
        STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
        STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
    }

     STOS_SemaphoreWait(StepSemaphore2_p);

    if ((NumberWrite != (SectorCount * BLOCK_SIZE)) ||
        (EvtError != ST_NO_ERROR))
    {
        STTBX_Print(("Didn't read back all bytes; read %i, wanted %i (error %u)\n",
                    NumberWrite, (SectorCount * BLOCK_SIZE), EvtError));
        STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
        STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
    }

#ifdef RW_INFO
    ShowStatus(&CmdStatus);

    STTBX_Print(("Comparing Data  :   \n"));
#endif
    if (CompareData(DataTest1_p, DataTest2_p, SectorCount * BLOCK_SIZE))
    {
        Error = ST_NO_ERROR;
        STTBX_Print(("SATA task successful\n"));
        
    }
    else
    {
        STTBX_Print(("Data doesn't match\n"));
        Error = STATAPI_ERROR_CMD_ERROR;
    }

    STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
    STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
test2_end:

    Params_p->Error = Error;
    Error = STATAPI_Close(SATAHandle);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Close drive %d Failed \n"));

    }
   	SATAAtapiTermParams.ForceTerminate= FALSE;
    Error = STATAPI_Term("SATA",&SATAAtapiTermParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Term Failed \n"));
    }

}

static ATAPI_TestResult_t ATAPI_TestMultiInstance(ATAPI_TestParams_t * TestParams)
{
   	ST_ErrorCode_t  	Error = ST_NO_ERROR;
   	ATAPI_TaskParams_t  PataParams, SataParams;
    ATAPI_TestResult_t  Result = TEST_RESULT_ZERO;	
    PataParams.Error = Error;
    SataParams.Error = Error;
      
    STTBX_Print(("Creating SATA task...\n"));
    Error = STOS_TaskCreate((void(*)(void *))SATATask,
		                                &SataParams,
		                                TEST_PARTITION_1,
		                                STACK_SIZE,
		                                NULL,
		                                TEST_PARTITION_1,
		                                &SataParams.Task_p,
		                                NULL,
		                                MIN_USER_PRIORITY,
		                                "SATATask",
		                                (task_flags_t)0);
    if  (Error != ST_NO_ERROR)
    {
        ATAPI_TestFailed(Result, "Unable to create SATA task");
        return Result;
    }		                                
		                                
    STTBX_Print(("Creating PATA task...\n"));
    Error = STOS_TaskCreate((void(*)(void *))PATATask,
		                                &PataParams,
		                                TEST_PARTITION_1,
		                                STACK_SIZE,
		                                NULL,
		                                TEST_PARTITION_1,
		                                &PataParams.Task_p,
		                                NULL,
		                                MIN_USER_PRIORITY,
		                                "PATA Task",
		                                (task_flags_t)0);
    if  (Error != ST_NO_ERROR)
    {
        ATAPI_TestFailed(Result, "Unable to create PATA task");
        return Result;
    }		                                
    
    STTBX_Print(("Waiting for both tasks to complete...\n"));

    /* Wait for each task before deleting it */
    if (STOS_TaskWait(&SataParams.Task_p,TIMEOUT_INFINITY) == 0)
    {
     	 STOS_TaskDelete(SataParams.Task_p,
    				     TEST_PARTITION_1,
    			    	 NULL,
    			   		 TEST_PARTITION_1);
 		 ATAPI_TestPassed(Result);    			   		 
     }
     
    if (STOS_TaskWait(&PataParams.Task_p,TIMEOUT_INFINITY) == 0)
    {
         STOS_TaskDelete(PataParams.Task_p,
    					 TEST_PARTITION_1,
    			   	     NULL,
    			         TEST_PARTITION_1);
      	 ATAPI_TestPassed(Result);    			   		 
     }
    if (PataParams.Error != ST_NO_ERROR ||
        SataParams.Error != ST_NO_ERROR)
    {
        ATAPI_TestFailed(Result, "Error during processing");
    }
    else
    {
        ATAPI_TestPassed(Result);
    }
    return Result;
} 
#endif/*TEST_TWO_DEVICE*/

/*****************************************************************
    ATAPI_TestIdentify

  This test is aimed to collect information about the device connected
  to the bus in order to perform later tests. Basically it issues a
  STATAPI_CMD_IDENTIFY_DEVICE command

******************************************************************/

static ATAPI_TestResult_t ATAPI_TestIdentify(ATAPI_TestParams_t * TestParams)
{

    ST_ErrorCode_t  Error;
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;

    /* ATAPI driver parameters */
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;
    STATAPI_Handle_t        AtapiHandle0 = NULL;
    STATAPI_Handle_t        AtapiHandle1 = NULL;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    STATAPI_DriveType_t     Type;
    U32 NumberRW,BuffSize,i;
    U8 *Data_p;
#if defined(ATAPI_FDMA)
    U8 *Temp_p;
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#elif defined(STATAPI_CRYPT)
    U8 *Temp_p;
#endif
#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType      = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType      = STATAPI_SATA;
#endif
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif

    AtapiInitParams.DriverPartition = TEST_PARTITION_1;

    AtapiInitParams.BaseAddress= (U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress= (U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p = (STATAPI_RegisterMap_t *)PATARegMasks;
#endif    
    strcpy(AtapiInitParams.EVTDeviceName,EVTDevName);

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    Temp_p= (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BLOCK_SIZE + 31);
    Data_p =(U8 *)(((U32)(Temp_p) + 31) & ~31);
#else
    Data_p= (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BLOCK_SIZE);
#endif
    if (Data_p == NULL)
    {
        ATAPI_TestFailed(Result, "Not enough memory for transfer buffer");
        return Result;
    }

    Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
    if (Error != ST_NO_ERROR)
    {
          STTBX_Print(("Init Failed Error: "));
          DisplayErrorNew(Error);
          goto identify_end;
    }

   STTBX_Print(("Try to open Handle on device 0 \n"));

    AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_0;
    Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&AtapiHandle0);

    if (Error == STATAPI_ERROR_DEVICE_NOT_PRESENT)
    {
         STTBX_Print(("No device found in Address 0\n"));
         Drives[0].Present=FALSE;

    }else if (Error ==ST_NO_ERROR)
    {
        Error = STATAPI_GetDriveType(AtapiHandle0,&Type);
        Drives[0].Present=TRUE;
        Drives[0].Type=Type;
        Drives[0].Handle=AtapiHandle0;
        if (Type== STATAPI_ATAPI_DRIVE)
        {

#if defined(SATA_SUPPORTED)
          STTBX_Print(("Device found at address 0 ......SATAPI\n"));
#else
          STTBX_Print(("Device found at address 0 ......ATAPI\n"));
#endif
        }
        else
#if defined(SATA_SUPPORTED)
          STTBX_Print(("Device found at address 0 ......SATA\n"));
#else
          STTBX_Print(("Device found at address 0 ......ATA\n"));
#endif

    }else
    {
        STTBX_Print(("Error when opening handle 0!!!  Error: "));
        DisplayErrorNew(Error);
        goto identify_end;
    }

   STTBX_Print(("-------------------------------\n"));
   STTBX_Print(("Try to open Handle on device 1 \n"));

    AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_1;
    Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&AtapiHandle1);
#if defined(SATA_SUPPORTED)    
	if (CheckCode(Error,ST_ERROR_FEATURE_NOT_SUPPORTED) == 0)
	{
		STTBX_Print(("No Master slave possible in SATA....\n"));
	}
#else  
    if (Error == STATAPI_ERROR_DEVICE_NOT_PRESENT)
    {
         STTBX_Print(("No device found in Address 1\n"));
         Drives[1].Present=FALSE;

    }else if (Error ==ST_NO_ERROR)
    {
        Error = STATAPI_GetDriveType(AtapiHandle1,&Type);
        Drives[1].Present=TRUE;
        Drives[1].Type=Type;
        Drives[1].Handle=AtapiHandle1;
        if (Type== STATAPI_ATAPI_DRIVE)
        {
          STTBX_Print(("Device found at address 1 ......ATAPI\n"));
        }else
        {
          STTBX_Print(("Device found at address 1 ......ATA\n"));
        }

    }else
    {
        STTBX_Print(("Error when opening handle 1!!!  Error: %x\n",Error));
        goto identify_end;
    }
#endif    
    STTBX_Print(("-------------------------------\n"));
    STTBX_Print(("%d.0 Issue Identify Device Cmd \n",TestParams->Ref));
    STTBX_Print(("Purpose: retrieve information about the device connected to the bus\n"));

    {
       for(i=0;i<2;i++)
       {
       if (Drives[i].Present)
       {
         STTBX_Print(("Device : %d\n",i));
         STTBX_Print(("================\n"));

        Error = STATAPI_SetPioMode(Drives[i].Handle,STATAPI_PIO_MODE_4);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("SetPioMode device %d  Failed \n",i));
            break;
        }


        if (Drives[i].Type== STATAPI_ATAPI_DRIVE)
        {
          Cmd.CmdCode= STATAPI_CMD_IDENTIFY_PACKET_DEVICE;
        }else
        {
          Cmd.CmdCode= STATAPI_CMD_IDENTIFY_DEVICE;
        }

          Cmd.SectorCount=0;
          Cmd.Features=0x00;
          Cmd.UseLBA= FALSE;
          Cmd.Sector= 0x00;
          Cmd.CylinderLow= 0x00;
          Cmd.CylinderHigh= 0x00;
          Cmd.Head= 0x00;
          BuffSize= BLOCK_SIZE;

        Error = STATAPI_CmdIn(Drives[i].Handle, &Cmd, Data_p,
                              BuffSize, &NumberRW, &CmdStatus);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result, "Couldn't issue identify device command");
            break;
        }
        STOS_SemaphoreWait(StepSemaphore_p);
        if (CheckCodeOk(Error))
        {
             ATAPI_TestFailed(Result,"Couldn't receive the identify info ");
             break;
         }else
         {
              ATAPI_TestPassed(Result);
         }

        STTBX_Print(("------------------------------------------------------\n"));
        ParseAndShow(&HardInfo,Data_p);

        Error = STATAPI_Close(Drives[i].Handle);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Close drive %d Failed \n",i));
            break;
        }

        }  /* end if present */
        }   /* end for */

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
#endif

        AtapiTermParams.ForceTerminate= FALSE;
        Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Term Failed \n"));
            goto identify_end;

        }
    }

    return Result;

identify_end:

    AtapiTermParams.ForceTerminate= TRUE;
    Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Term Failed \n"));
    }
    ATAPI_TestFailed(Result, "Identify Device Test Failed ");
    return Result;
}

/*****************************************************************
    ATAPI_TestPacket

  This test is aimed to issue some packet commands and show the results

******************************************************************/

static ATAPI_TestResult_t ATAPI_TestPacket(ATAPI_TestParams_t * TestParams)
{
    ST_ErrorCode_t  Error;
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;

    /* ATAPI driver parameters */
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;
    STATAPI_Packet_t        Pkt;
    STATAPI_PktStatus_t     PktStatus;
    U32 NumberRW,j;
    U32 AtapiBlockSize;
    U32 Acum,Throughput=0;
    U8 *Data_p;
    U8 *SenseData_p;
    
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *SenseTemp_p;
    U8 *Temp_p;

    Temp_p      = (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,8*K + 31);
    Data_p      = (U8*)(((U32)(Temp_p) + 31) & ~31);
    SenseTemp_p = (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,8*BLOCK_SIZE + 31);
    SenseData_p = (U8*)(((U32)(SenseTemp_p) + 31) & ~31);

#else/* !ATAPI_FDMA*/
    Data_p      = (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,8*K);
    SenseData_p = (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,8*BLOCK_SIZE);
#endif
    memset( Data_p, 0, 8*K );
    memset( SenseData_p, 0, 8*BLOCK_SIZE );
    
#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif

    AtapiInitParams.DriverPartition= TEST_PARTITION_1;
#if defined(ATAPI_FDMA)
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#endif
    AtapiInitParams.BaseAddress= (U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress= (U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p = (STATAPI_RegisterMap_t *)PATARegMasks;    
#endif    
    strcpy(AtapiInitParams.EVTDeviceName,EVTDevName);

    Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Init Failed \n"));
        goto packet_end;
    }

    for(j=0;j<1;j++)
    {
      if (Drives[j].Present && (Drives[j].Type== STATAPI_ATAPI_DRIVE))
      {
        if (j==0)
            AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_0;
        else
            AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_1;

        Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&Drives[j].Handle);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Open Failed \n"));
            goto packet_end;
        }

        Error = STATAPI_SetPioMode(Drives[j].Handle,STATAPI_PIO_MODE_4);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("SetPioMode Failed \n"));
            goto packet_end;
        }

        STTBX_Print(("%d.0 Read Capacity \n",TestParams->Ref));
        STTBX_Print(("Purpose: Retrieve the capacity of this LU :\n"));
        {
            U8 repeat=0;
            Quiet=TRUE;

			do
			{
				if ( TestUnitReady (Drives[j].Handle) == ST_NO_ERROR )
				{
					break;
				}
				task_delay ( 20 * 200000 );
				++repeat;
			}while ( repeat < 10 );

			if ( repeat >= 10 )
			{
	        STTBX_Print(("%Unit Not Ready\n",TestParams->Ref));
	        goto packet_end;
			}
			repeat = 0;

        do
        {

           memset( Pkt.Data, 0, 15 );
           Pkt.OpCode= STATAPI_PKT_READ_CAPACITY;

           Error = STATAPI_PacketIn(Drives[j].Handle, &Pkt,Data_p,
                                    4096,&NumberRW,&PktStatus);

            if (Error != ST_NO_ERROR)
            {
                STTBX_Print((" Read Capacity failed \n"));
                goto packet_end;
            }

            STOS_SemaphoreWait(StepSemaphore_p);
            if (EvtError ==ST_NO_ERROR)
            {
               STTBX_Print(("*** Number of retries: %d \n",repeat));
               STTBX_Print(("### Bytes written: %d \n",NumberRW));
               /*memcpy(&LastLBA,Data_p,4);*/
               LastLBA= Data_p[0]<<24
                       |Data_p[1]<<16
                       |Data_p[2]<<8
                       |Data_p[3];
               STTBX_Print(("--- Last LBA addressable: 0x%x\n",LastLBA));
               AtapiBlockSize=Data_p[4]<<24
                       |Data_p[5]<<16
                       |Data_p[6]<<8
                       |Data_p[7];
              STTBX_Print(("--- Block Length: %d\n",AtapiBlockSize));

               STTBX_Print((" \n"));

               ATAPI_TestPassed(Result);
               break;
            }
            else
            {

               /* Request sense key */
               memset( Pkt.Data, 0, 15 );
               Pkt.OpCode= STATAPI_PKT_REQUEST_SENSE;
               Pkt.Data[3]=0x20,

               Error = STATAPI_PacketIn(Drives[j].Handle, &Pkt,SenseData_p,
                                    4096,&NumberRW,&PktStatus);
                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Request sense key  Failed \n"));
                    goto packet_end;
                }

                STOS_SemaphoreWait(StepSemaphore_p);
                if (SenseData_p[2]&0x0F)   repeat++;
                if (repeat>30)
                {
                    STTBX_Print((" Request Sense failed, too many tries: %d \n",repeat));
                    ATAPI_TestFailed(Result,"Request Sense Failed ");
                    break;
                }
            }
            task_delay(5000);
          }while(TRUE);
         }

              memset( Data_p, 0, 8*K );
  			 {
			    STTBX_Print(("%d.1 ATAPI Simple R/W Test in pio mode \n",TestParams->Ref));
                STTBX_Print(("Purpose: to ensure we can read data from ATAPI device \n"));

				/*For testing disc read/write in pio mode */
				if ( LastLBA > 0x100000 )
				{
					StartSector = 16;
					NoOfSectors	= 1;
			
					Error = SimpleRTestDVD(Drives[j].Handle,StartSector, NoOfSectors );

                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read(DVD) ATAPI device");
                        goto packet_end;
                    }
                    ATAPI_TestPassed(Result);

                  	NoOfSectors	= 4;
					Error = SimpleRTestDVD(Drives[j].Handle,StartSector, NoOfSectors );

					if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read(DVD) ATAPI device");
                        goto packet_end;
                    }
                    ATAPI_TestPassed(Result);
             
                 }

                else
				{
					StartSector = 170;
					NoOfSectors	= 1;
					Error = SimpleRTestCD(Drives[j].Handle,StartSector, NoOfSectors );
                    if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read(CD) ATAPI device");
                        goto packet_end;
                    }
                    ATAPI_TestPassed(Result);

					NoOfSectors	= 4;
					Error = SimpleRTestCD(Drives[j].Handle,StartSector, NoOfSectors );

					if (CheckCodeOk(Error))
                    {
                        ATAPI_TestFailed(Result,"Unable to Read(CD) ATAPI device");
                        goto packet_end;
                    }
                    ATAPI_TestPassed(Result);

			     }

    	          STTBX_Print(("%d.2 Seek time\n", TestParams->Ref));
                  STTBX_Print(("Purpose: Calculate seek time taken by LU to track LBA\n"));
    		      Error = PacketSeekTime(Drives[j].Handle,ITERATION_NUMBER, &Acum);
      			  if (Error != ST_NO_ERROR)
                  {
                        ATAPI_TestFailed(Result, "Something failed on the seek test ");
                        DisplayError(Error);
                  }
                  else
                  {
                        STTBX_Print(("Seek time = %d us\n",(Acum * 1000000) /
                                           (ClocksPerSec * ITERATION_NUMBER)));
                        ATAPI_TestPassed(Result);
                  }

                  STTBX_Print(("%d.3 Read seek time\n", TestParams->Ref));
                  STTBX_Print(("Purpose: Calculate the read seek time\n"));
                  Error = PacketReadSeekTime(Drives[j].Handle, ITERATION_NUMBER, &Acum,&Throughput);
                  STTBX_Print(("Throughput = %d Kb/s \n",Throughput));
                  if (Error != ST_NO_ERROR)
                  {
                      ATAPI_TestFailed(Result, "Something failed on the read seek test");
                  }
                  else
                  {
                      STTBX_Print(("Read seek time = %d us\n",
                             (Acum)/(ITERATION_NUMBER)));
                      ATAPI_TestPassed(Result);
                  }
             }

#ifdef   SOAK_TEST
        memset( Data_p, 0, 8*K );

        STTBX_Print(("%d.2  Soak Test \n",TestParams->Ref));
        STTBX_Print(("Purpose: Read all the addressable blocks in groups of 3 \n"));
        {
            U32 LBA=0;
            U32 k;


           memset( Pkt.Data, 0, 15 );
           Pkt.OpCode= STATAPI_PKT_READ_CD;
           Pkt.Data[0]=0x00;
           Pkt.Data[6]=0x00;
           Pkt.Data[7]=0x03; /* Length= 0x0003 */
           Pkt.Data[8]=0x10;  /* Retrieve user data*/

           for (k=0;k<(LastLBA/3);k++)
           {

               Pkt.Data[1]=(LBA & 0xFF000000) >>24;
               Pkt.Data[2]=(LBA & 0x00FF0000) >>16;
               Pkt.Data[3]=(LBA & 0x0000FF00) >>8;
               Pkt.Data[4]=(LBA & 0x000000FF);

               Error = STATAPI_PacketIn(Drives[j].Handle, &Pkt,Data_p,
                                    8*K,&NumberRW,&PktStatus);

                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Read  CD  failed \n"));
                    goto packet_end;
                }

                STOS_SemaphoreWait(StepSemaphore_p);
                if (EvtError ==ST_NO_ERROR)
                {

                   STTBX_Print(("### Percentage read: %d% \r",100*k/(LastLBA/3)));
                   LBA+= 3;
                }else{
                   STTBX_Print(("\n Error when k= %d",k));
                }
            }
            STTBX_Print(("\n"));
            ATAPI_TestPassed(Result);
         }

#endif /* #ifdef   SOAK_TEST */
            Error = STATAPI_Close(Drives[j].Handle);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Close Failed \n"));
                goto packet_end;
            }

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            if (Temp_p != NULL)
                STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
            if (SenseTemp_p != NULL)
                STOS_MemoryDeallocate(TINY_PARTITION_1,SenseTemp_p);
#else
            if (Data_p != NULL)
                STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
           
            if (SenseData_p != NULL)
                STOS_MemoryDeallocate(TEST_PARTITION_1,SenseData_p);
#endif
        }
    }
    
    AtapiTermParams.ForceTerminate= FALSE;
    Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Term Failed \n"));
    }
    return Result;
packet_end:
    AtapiTermParams.ForceTerminate= TRUE;
    Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Term Failed \n"));
    }

    ATAPI_TestFailed(Result,"Identify Device Test Failed ");
    return Result;


}

/*****************************************************************
    ATAPI_TestPerformance

    This test is aimed to measure time parameter and other performance
    paramters in order to know about the overhead introduced by the driver.
******************************************************************/
static ATAPI_TestResult_t ATAPI_TestPerformance(ATAPI_TestParams_t *TestParams)
{
    ST_ErrorCode_t  Error;
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;

    /* ATAPI driver parameters */
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;
    STATAPI_Packet_t     Pkt;
    STATAPI_PktStatus_t  PktStatus;

    U8 k, StartI;
    U32 Acum, i, j;
    U32 SpeedArray[BUFFER_SIZES];
    U8 *Data_p;
    U32 repeat=0;
    U32 NumberRW=0;
    U32 Length=0,DataRate,DataRate2,StartLBA,EndLBA;
#if defined(ATAPI_FDMA) 
    U8 *Temp_p;
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#elif defined(STATAPI_CRYPT)
    U8 *Temp_p;
#endif
    AtapiInitParams.DriverPartition= TEST_PARTITION_1;
#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif
    AtapiInitParams.BaseAddress = (U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress = (U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p = (STATAPI_RegisterMap_t *)PATARegMasks;    
#endif    
    strcpy(AtapiInitParams.EVTDeviceName, EVTDevName);

    Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Init Failed \n"));
        DisplayErrorNew(Error);
        goto perform_end;
    }

    for (j = 0; j < 1; j++)
    {
        if (Drives[j].Present && (Drives[j].Type== STATAPI_ATA_DRIVE))
        {
            AtapiOpenParams.DeviceAddress = STATAPI_DEVICE_0;
            Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,
                                 &Drives[j].Handle);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Open Failed \n"));
                goto perform_end;
            }

            Error = STATAPI_SetPioMode(Drives[j].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("SetPioMode Failed \n"));
                goto perform_end;
            }

            Error = SetTransferModePio(Drives[j].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("SetTransferModePio failed\n"));
                goto perform_end;
            }

            STTBX_Print(("%d.0 Track to track seek time\n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the track to track seek time\n"));

            Error = SeekTime(Drives[j].Handle, ITERATION_NUMBER, &Acum);
            if (Error != ST_NO_ERROR)
            {
                ATAPI_TestFailed(Result, "Something failed on the seek test ");
                DisplayError(Error);
            }
            else
            {
                STTBX_Print(("Track to track seek time = %d us\n",
                           (Acum * 1000000) /
                                   (ClocksPerSec * ITERATION_NUMBER)));
                ATAPI_TestPassed(Result);
            }


            STTBX_Print(("%d.1 Write seek time\n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the write seek time\n"));

            Error = WriteSeekTime(Drives[j].Handle, ITERATION_NUMBER, &Acum);
            if (Error != ST_NO_ERROR)
            {
               ATAPI_TestFailed(Result, "Something failed on the write seek test ");
            }
            else
            {
                 STTBX_Print(("Write seek time = %d us\n",
                             (Acum * 1000000) /
                                     (ClocksPerSec * ITERATION_NUMBER)));
                 ATAPI_TestPassed(Result);
            }

            STTBX_Print(("%d.2 Read seek time\n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the read seek time\n"));

            Error = ReadSeekTime(Drives[j].Handle, ITERATION_NUMBER, &Acum);
            if (Error != ST_NO_ERROR)
            {
               ATAPI_TestFailed(Result, "Something failed on the write seek test");
            }
            else
            {
                 STTBX_Print(("Read seek time = %d us\n",
                             (Acum * 1000000) /
                                     (ClocksPerSec * ITERATION_NUMBER)));
                 ATAPI_TestPassed(Result);
            }

            STTBX_Print(("%d.3 Throughput Write \n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when writing using several buffer sizes \n"));
            STTBX_Print(("       We aren't working in multiple mode i.e one sector txfer per interrupt\n"));

            Error = ThrPutWriteSimple(Drives[j].Handle,
                                      ITERATION_NUMBER,
                                      SpeedArray);
            if (Error != ST_NO_ERROR)
            {
               ATAPI_TestFailed(Result,
                                "Something failed on the write throughput test");
            }
            else
            {
                STTBX_Print(("Buffer Size (KB)     Speed (Kbytes/sec)\n"));

                for (i = 0; i < BUFFER_SIZES; i++)
                {
                    STTBX_Print(("    %d                 %d\n",
                                BuffSizes[i] / K, SpeedArray[i]));
                }
                ATAPI_TestPassed(Result);
            }

            STTBX_Print(("%d.4 Throughput Read\n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when reading using several buffer sizes \n"));
            STTBX_Print(("       We aren't working in multiple mode i.e one sector txfer per interrupt\n"));

            Error = ThrPutReadSimple(Drives[j].Handle,
                                     ITERATION_NUMBER, SpeedArray);
            if (Error != ST_NO_ERROR)
            {
                ATAPI_TestFailed(Result, "Something failed on the read throughput");
            }
            else
            {
                STTBX_Print(("Buffer Size (KB)     Speed (Kbytes/sec)\n"));

                for (i = 0; i < BUFFER_SIZES; i++)
                {
                    STTBX_Print(("    %d                 %d\n",
                                BuffSizes[i] / K, SpeedArray[i]));
                }
                ATAPI_TestPassed(Result);
            }

#if defined(DMA_PRESENT)
#if !defined(ST_5528) && !defined(SATA_SUPPORTED)

            STTBX_Print(("%d.5a Throughput Read (MWDMA)\n",TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when reading using several buffer sizes \n"));

            Error = STATAPI_SetDmaMode(Drives[j].Handle,
                                       STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting MWDMA mode 2: "));
                DisplayErrorNew(Error);
            }
            Error = SetTransferModeDma(Drives[j].Handle,
                                       STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting transfer mode: "));
                DisplayErrorNew(Error);
            }

            Error = ThrPutReadSimpleDma(Drives[j].Handle,
                                       ITERATION_NUMBER, SpeedArray);
            if (Error != ST_NO_ERROR)
            {
               ATAPI_TestFailed(Result, "Something failed on the read throughput");
            }
            else
            {
                STTBX_Print(("Buffer Size (KB)     Speed (Kbytes/sec)\n"));

                for (i = 0; i < BUFFER_SIZES; i++)
                {
                    STTBX_Print(("    %d                 %d\n",
                                BuffSizes[i] / K, SpeedArray[i]));
                }
                ATAPI_TestPassed(Result);
            }

#endif

#if !defined(HDDI_5514_CUT_3_0)
            STTBX_Print(("%d.5b Throughput Read (UDMA)\n",TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when reading using several buffer sizes \n"));

            Error = STATAPI_SetDmaMode(Drives[j].Handle,
                                       STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting UDMA mode 4: "));
                DisplayErrorNew(Error);
            }
            Error = SetTransferModeDma(Drives[j].Handle,
                                       STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting transfer mode: "));
                DisplayErrorNew(Error);
            }

            Error = ThrPutReadSimpleDma(Drives[j].Handle,
                                       ITERATION_NUMBER, SpeedArray);
            if (Error != ST_NO_ERROR)
            {
               ATAPI_TestFailed(Result, "Something failed on the read throughput");
            }
            else
            {
                STTBX_Print(("Buffer Size (KB)     Speed (Kbytes/sec)\n"));

                for (i = 0; i < BUFFER_SIZES; i++)
                {
                    STTBX_Print(("    %d                 %d\n",
                                BuffSizes[i] / K, SpeedArray[i]));
                }
                ATAPI_TestPassed(Result);
            }

#endif
#endif/*DMA_PRESENT*/

            Error = STATAPI_SetPioMode(Drives[j].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("SetPioMode Failed \n"));
                goto perform_end;
            }

            Error = SetTransferModePio(Drives[j].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting transfer mode: "));
                DisplayErrorNew(Error);
            }


            STTBX_Print(("%d.6 Throughput Write Multiple\n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when Writing using several buffer sizes\n"));
            STTBX_Print(("       NOW we are in multiple mode\n"));

            StartI = 0;
            for (k = 2; k <= 16; k = 2 * k)
            {
                Error = ThrPutWriteMultiple(Drives[j].Handle, k,
                                            SpeedArray);
                if (Error != ST_NO_ERROR)
                {
                    ATAPI_TestFailed(Result,"Something failed on the  Throughput Multiple test");
                }
                else
                {
                    STTBX_Print(("Buffer Size (KB)   Speed (KB/sec)  Sectors/transfer %d \n",k));
                    for(i = StartI; i < BUFFER_SIZES; i++)
                    {
                       STTBX_Print(("    %d                %d   \n",
                                   BuffSizes[i] / K, SpeedArray[i]));
                    }
                    ATAPI_TestPassed(Result);
                }
                StartI++;
            }

            STTBX_Print(("%d.7 Throughput Read Multiple\n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when reading using several buffer sizes\n"));
            STTBX_Print(("       NOW we are in multiple mode\n"));

            StartI = 0;
            for (k = 2; k <= 16; k = 2 * k)
            {
                Error = ThrPutReadMultiple(Drives[j].Handle, k, SpeedArray);
                if (Error != ST_NO_ERROR)
                {
                   ATAPI_TestFailed(Result,"Something failed on the  Throughput Multiple test");
                }
                else
                {
                    STTBX_Print(("Buffer Size (KB)   Speed (KB/sec)  Sectors/transfer %d \n",k));
                    for (i = StartI; i < BUFFER_SIZES; i++)
                    {
                       STTBX_Print(("    %d                %d   \n",
                                   BuffSizes[i] / K, SpeedArray[i]));
                    }

                    ATAPI_TestPassed(Result);
                }
                StartI++;
            }

#if defined(DMA_PRESENT)
#if !defined(ST_5528) && !defined(SATA_SUPPORTED)

            STTBX_Print(("%d.8 Throughput Read Multiple (MWDMA)\n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when reading using several buffer sizes \n"));
            Error = STATAPI_SetDmaMode(Drives[j].Handle,
                                       STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting UDMA mode 4: "));
                DisplayErrorNew(Error);
            }
            Error = SetTransferModeDma(Drives[j].Handle,
                                       STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting transfer mode: "));
                DisplayErrorNew(Error);
            }

            StartI = 0;
            for (k = 2; k <= 16; k = 2 * k)
            {
                Error = ThrPutReadMultipleDma(Drives[j].Handle, k, SpeedArray);
                if (Error != ST_NO_ERROR)
                {
                    ATAPI_TestFailed(Result,
                                    "Something failed on the Throughput Multiple test ");
                }
                else
                {
                    STTBX_Print(("Buffer Size (KB)   Speed (KB/sec)\n"));
                    for (i = StartI; i < BUFFER_SIZES; i++)
                    {
                        STTBX_Print(("    %d                %d   \n",
                                    BuffSizes[i] / K, SpeedArray[i]));
                    }
                    ATAPI_TestPassed(Result);
                }
                StartI++;
            }
#endif/*5528*/

#if !defined(HDDI_5514_CUT_3_0)

            STTBX_Print(("%d.9 Throughput Read Multiple (UDMA)\n",TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when reading using several buffer sizes \n"));

            Error = STATAPI_SetDmaMode(Drives[j].Handle,
                                       STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting DMA mode: "));
                DisplayErrorNew(Error);
            }
            Error = SetTransferModeDma(Drives[j].Handle,
                                       STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting transfer mode: "));
                DisplayErrorNew(Error);
            }

            StartI = 0;
            for (k = 2; k <= 16; k = 2 * k)
            {
                Error = ThrPutReadMultipleDma(Drives[j].Handle,k,SpeedArray);
                if (Error != ST_NO_ERROR)
                {
                   ATAPI_TestFailed(Result,"Something failed on the  Throughput Multiple test ");
                }
                else
                {
                    STTBX_Print(("Buffer Size (KB)   Speed (KB/sec)\n"));
                    for (i = StartI; i < BUFFER_SIZES; i++)
                    {
                        STTBX_Print(("    %d                %d   \n",
                                    BuffSizes[i] / K, SpeedArray[i]));
                    }

                    ATAPI_TestPassed(Result);
                }
                StartI++;
           }

#endif /* If not cut 3.0 */

#if !defined(ST_5528) && !defined(SATA_SUPPORTED)
            STTBX_Print(("%d.9 Throughput write (MWDMA)\n", TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when writing using several buffer sizes \n"));

            Error = STATAPI_SetDmaMode(Drives[j].Handle,
                                       STATAPI_DMA_MWDMA_MODE_2);
            Error = SetTransferModeDma(Drives[j].Handle,
                                       STATAPI_DMA_MWDMA_MODE_2);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting transfer mode: "));
                DisplayErrorNew(Error);
            }
            StartI = 0;
            for (k = 2; k <= 16; k = 2 * k)
            {
                Error = ThrPutWriteMultipleDma(Drives[j].Handle, k, SpeedArray);
                if (Error != ST_NO_ERROR)
                {
                   ATAPI_TestFailed(Result, "Something failed on the  Throughput Multiple test ");
                }
                else
                {
                    STTBX_Print(("Buffer Size (KB)   Speed (KB/sec)\n"));
                    for (i = StartI; i < BUFFER_SIZES; i++)
                    {
                       STTBX_Print(("    %d                %d   \n",
                                   BuffSizes[i] / K,SpeedArray[i]));
                    }

                    ATAPI_TestPassed(Result);
                }
                StartI++;
            }
#endif/*5528*/

#if !defined(HDDI_5514_CUT_3_0)

            STTBX_Print(("%d.10 Throughput write (UDMA)\n",TestParams->Ref));
            STTBX_Print(("Purpose: Calculate the throughput when writing using several buffer sizes \n"));

            Error = STATAPI_SetDmaMode(Drives[j].Handle,
                                       STATAPI_DMA_UDMA_MODE_4);
            Error = SetTransferModeDma(Drives[j].Handle,
                                       STATAPI_DMA_UDMA_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Error setting transfer mode: "));
                DisplayErrorNew(Error);
            }
            StartI = 0;
            for (k = 2; k <= 16; k = 2 * k)
            {
                Error = ThrPutWriteMultipleDma(Drives[j].Handle, k, SpeedArray);
                if (Error != ST_NO_ERROR)
                {
                    ATAPI_TestFailed(Result, "Something failed on the  Throughput Multiple test ");
                }
                else
                {
                    STTBX_Print(("Buffer Size (KB)   Speed (KB/sec)\n"));
                    for (i = StartI; i < BUFFER_SIZES; i++)
                    {
                        STTBX_Print(("    %d                %d   \n",
                                    BuffSizes[i] / K, SpeedArray[i]));
                    }
                    ATAPI_TestPassed(Result);
                }
                StartI++;
            }

#endif /* Not cut 3.0 */
#endif /* DMA_PRESENT */

            Error = STATAPI_Close(Drives[j].Handle);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Close Failed \n"));
                goto perform_end;
            }
            AtapiOpenParams.DeviceAddress = STATAPI_DEVICE_1;
        } /* end of if present*/
        else if((Drives[j].Present && (Drives[j].Type== STATAPI_ATAPI_DRIVE)))
        {

            AtapiOpenParams.DeviceAddress = STATAPI_DEVICE_0;
            Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,
                                 &Drives[j].Handle);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Open Failed \n"));
                goto perform_end;
            }

            Error = STATAPI_SetPioMode(Drives[j].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("SetPioMode Failed \n"));
                goto perform_end;
            }

            Error = SetTransferModePio(Drives[j].Handle, STATAPI_PIO_MODE_4);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("SetTransferModePio failed\n"));
                goto perform_end;
            }
         STTBX_Print(("%d.0 Get Performance Command:\n",TestParams->Ref));
         STTBX_Print(("Purpose: Calculate the throughput of LU\n"));

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        Temp_p = (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,1*K + 31);
        Data_p = (U8 *)(((U32)(Temp_p) + 31) & ~31);
#else
        Data_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,1*K);
#endif
 		    do
		    {
			   if ( TestUnitReady (Drives[j].Handle) == ST_NO_ERROR )
			   {
			       RequestSense (Drives[j].Handle);
				   break;
			   }
			   task_delay ( 20 * 200000 );
			   ++repeat;
		    }while ( repeat < 10 );

    		   if ( repeat >= 10 )
    		   {
        	        STTBX_Print(("%Unit Not Ready\n",TestParams->Ref));
        	        goto perform_end;
    		   }
    		   repeat = 0;

            do
             {
               memset( Pkt.Data, 0, 15 );
               Pkt.OpCode= STATAPI_PKT_GET_PERFORMANCE;
               Pkt.Data[0]=0x10;
               Pkt.Data[7]=0x00;
               Pkt.Data[8]=0x02;

               Error = STATAPI_PacketIn(Drives[j].Handle, &Pkt,Data_p,
                                        4096,&NumberRW,&PktStatus);

                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print((" Read  TOC failed \n"));
                    break;
                }

                STOS_SemaphoreWait(StepSemaphore_p);
                if (EvtError ==ST_NO_ERROR)
                {
                   STTBX_Print(("*** Number of retries: %d \n",repeat));
                   Length = ((Data_p[0]<< 24) | (Data_p[1]<< 16) | (Data_p[2]<< 8) | (Data_p[3]));
                   STTBX_Print(( "performnace data Length=%02X\n",
                                 Length));

                   StartLBA = ((Data_p[8]<< 24) | (Data_p[9]<< 16) | (Data_p[10]<< 8) | (Data_p[11]));
                   DataRate = ((Data_p[12]<< 24) | (Data_p[13]<< 16) | (Data_p[14]<< 8) | (Data_p[15]));
                   StartLBA = ((Data_p[16]<< 24) | (Data_p[17]<< 16) | (Data_p[18]<< 8) | (Data_p[19]));
                   DataRate2 = ((Data_p[20]<< 24) | (Data_p[21]<< 16) | (Data_p[22]<< 8) | (Data_p[23]));
                   STTBX_Print(("--- start LBA addressable: 0x%x\n",StartLBA));
                   STTBX_Print(("Throughput at start LBA= %d Kb/s \n",DataRate));
                   STTBX_Print(("--- End LBA addressable: 0x%x\n",EndLBA));
                   STTBX_Print(("Throughput at end LBA = %d Kb/s \n",DataRate2));
                   Error =ST_NO_ERROR;
                   break;
                }

            }while(TRUE);
            ATAPI_TestPassed(Result);

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
#endif

            Error = STATAPI_Close(Drives[j].Handle);
            if (Error != ST_NO_ERROR)
            {
                STTBX_Print(("Close Failed \n"));
                goto perform_end;
            }
            AtapiOpenParams.DeviceAddress = STATAPI_DEVICE_1;
           }
      } /* end of for*/
    AtapiTermParams.ForceTerminate = FALSE;
    Error = STATAPI_Term(AtapiDevName, &AtapiTermParams);

    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Term Failed \n"));
    }
    return Result;

perform_end:

    AtapiTermParams.ForceTerminate= TRUE;
    Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Term Failed \n"));
    }
    return Result;
}

/*****************************************************************
    ATAPI_SoakTest

    This test is aimed to check all the sectors of the HDD attached to
    the bus.

******************************************************************/
#ifdef SOAK_TEST

static ATAPI_TestResult_t ATAPI_SoakTest(ATAPI_TestParams_t * TestParams)
{

    ST_ErrorCode_t  Error;
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;

    /* ATAPI driver parameters */
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;
    U32 k;

#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif

    AtapiInitParams.DriverPartition= TEST_PARTITION_1;
#if defined(ATAPI_FDMA)
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#endif
    AtapiInitParams.BaseAddress= (volatile U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress= (volatile U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p = (STATAPI_RegisterMap_t *)PATARegMasks;    
#endif    
    strcpy(AtapiInitParams.EVTDeviceName,EVTDevName);

    #define  NUMBER_OF_SECTORS   64
    #define  BUFF_SIZE            (NUMBER_OF_SECTORS/2)*K

     Error = STATAPI_Init(AtapiDevName, &AtapiInitParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Init Failed \n"));
        goto soak_end;
    }
    for(k=0;k<1;k++)
    {
     if (Drives[k].Present && (Drives[k].Type== STATAPI_ATA_DRIVE))
     {

      AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_0;
      Error = STATAPI_Open(AtapiDevName, &AtapiOpenParams,&Drives[k].Handle);
      if (Error != ST_NO_ERROR)
      {
          STTBX_Print(("Open Failed \n"));
          goto soak_end;
      }

      Error = STATAPI_SetPioMode(Drives[k].Handle,STATAPI_PIO_MODE_4);
      if (Error != ST_NO_ERROR)
      {
          STTBX_Print(("SetPioMode Failed \n"));
          goto soak_end;
      }

    STTBX_Print(("%d.0 Simple R/W Test \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
    STTBX_Print(("         in all the sectors \n"));
    {

        U8 *DataTest1_p;
        U8 *DataTest2_p;
        U32 i,j,LBA_Address,Iter,Remain;
        STATAPI_Cmd_t Cmd;
        U32 NumberWrite=0;

        STATAPI_CmdStatus_t CmdStatus;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        U8 *Temp1_p;
        U8 *Temp2_p;

        Temp1_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BUFF_SIZE + 31);
        Temp2_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BUFF_SIZE + 31);
        DataTest1_p = (U8 *)(((U32)(Temp1_p) + 31) & ~31);
        DataTest2_p = (U8 *)(((U32)(Temp2_p) + 31) & ~31);
#else/*!ATAPI_FDMA*/
        DataTest1_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BUFF_SIZE);
        DataTest2_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BUFF_SIZE);
#endif

       for(i=0;i<BUFF_SIZE;i++)
       {
        DataTest1_p[i]=i;
        DataTest2_p[i]=0x00;
       }

      LBA_Address=0x00000000;
      Iter= HardInfo.AddressableSectors/NUMBER_OF_SECTORS;
      Remain= HardInfo.AddressableSectors%NUMBER_OF_SECTORS;
      Cmd.Features=0x00;
      Cmd.UseLBA= TRUE;
      STTBX_Print((" Iter= %d \n",Iter));


     for (j=0;j<Iter;j++)
     {

        Cmd.LBA=LBA_Address;

        Cmd.SectorCount= NUMBER_OF_SECTORS;
        Cmd.CmdCode= STATAPI_CMD_WRITE_SECTORS;

        Error = STATAPI_CmdOut(Drives[k].Handle,&Cmd,DataTest1_p,BUFF_SIZE,&NumberWrite,&CmdStatus);
        if (Error != ST_NO_ERROR)
        {
            ATAPI_TestFailed(Result,"Error when writing simple mode");
            STTBX_Print(("Error = "));
            DisplayError(Error);
            STTBX_Print(("\n"));
            goto end_simple_soak;
        }

        STOS_SemaphoreWait(StepSemaphore_p);
        if ((CmdStatus.Error) || (EvtError != ST_NO_ERROR))
        {
            ATAPI_TestFailed(Result, "SoakTest failed when writing multiple mode");
            STTBX_Print(("CmdStatus.Error: %i\n", CmdStatus.Error));
            STTBX_Print(("       EvtError: %i\n", EvtError));
            STTBX_Print((" Number written: %i (expected %i)\n",
                        NumberWrite, BUFF_SIZE));
        }

        if (NumberWrite != (BUFF_SIZE))
        {
            ATAPI_TestFailed(Result,"SoakTest failed when writing multiple mode");
            STTBX_Print(("Wrote %i bytes, instead of %i\n",
                         NumberWrite, BUFF_SIZE));
            goto end_simple_soak;
        }

        Cmd.SectorCount=NUMBER_OF_SECTORS;
        Cmd.CmdCode= STATAPI_CMD_READ_SECTORS;

        Error = STATAPI_CmdIn(Drives[k].Handle, &Cmd, DataTest2_p,
                              BUFF_SIZE, &NumberWrite, &CmdStatus);
        if (Error != ST_NO_ERROR)
        {
            ATAPI_TestFailed(Result,"SoakTest failed when reading simple mode");
            STTBX_Print(("Error = "));
            DisplayError(Error);
            STTBX_Print(("\n"));
            STTBX_Print(("Address %d \n",LBA_Address));
            goto end_multiple_soak;

        }

        STOS_SemaphoreWait(StepSemaphore_p);
        if ((CmdStatus.Error) || (EvtError != ST_NO_ERROR))
        {
            ATAPI_TestFailed(Result, "SoakTest failed when reading multiple mode");
            STTBX_Print(("CmdStatus.Error: %i\n", CmdStatus.Error));
            STTBX_Print(("       EvtError: %i\n", EvtError));
            STTBX_Print(("    Number read: %i (expected %i)\n",
                        NumberWrite, BUFF_SIZE));
        }

        if (NumberWrite != (BUFF_SIZE))
        {
             ATAPI_TestFailed(Result,"SoakTest failed when reading");
             STTBX_Print(("Read %i bytes, instead of %i\n",
                         NumberWrite, BUFF_SIZE));
             STTBX_Print(("Address %d\n", LBA_Address));
             goto end_simple_soak;
        }

        if (VerifyBuffer(DataTest1_p, DataTest2_p, BUFF_SIZE) != TRUE)
        {
            ATAPI_TestFailed(Result, "Data mismatch");
        }

        /* Move to next block, clear read buffer */
        LBA_Address += NUMBER_OF_SECTORS;
        memset(DataTest2_p, 0x00, BUFF_SIZE);

        if ((j%1000)==0)
        {
            STTBX_Print(("\t\t\tPercentage tested: %02d%% , %d\r",(j*100)/Iter,j));
        }
        STTBX_Print((" j= %d \r",j));
    }
    if (Remain!=0)
    {
        STTBX_Print(("Almost all sectors checked only %d left\n",Remain));
    }
    ATAPI_TestPassed(Result);

end_simple_soak:

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
   }

    STTBX_Print(("%d.1 Multiple R/W Test \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
    STTBX_Print(("         in all the sectors \n"));
    {

        U8 *DataTest1_p;
        U8 *DataTest2_p;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        U8 *Temp1_p,*Temp2_p;
#endif
        U32 i,j,LBA_Address,Iter,Remain;
        STATAPI_Cmd_t Cmd;
        U32 NumberWrite=0;
        STATAPI_CmdStatus_t CmdStatus;


       Cmd.SectorCount=HardInfo.DRQBlockSize;
       Cmd.CmdCode= STATAPI_CMD_SET_MULTIPLE_MODE;
       STTBX_Print(("Setting multiple mode  \n "));

       Error = STATAPI_CmdNoData(Drives[k].Handle,&Cmd,&CmdStatus);
       if (Error !=ST_NO_ERROR)
       {
           ATAPI_TestFailed(Result,"SoakTest failed when setting multiple mode");
           goto end_multiple_soak;
       }

       STOS_SemaphoreWait(StepSemaphore_p);

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
       Temp1_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BUFF_SIZE + 31);
       Temp2_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BUFF_SIZE + 31);
       DataTest1_p=(U8 *)(((U32)(Temp1_p) + 31) & ~31);
       DataTest2_p= (U8 *)(((U32)(Temp1_p) + 31) & ~31);
#else/*!ATAPI FDMA*/
       DataTest1_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BUFF_SIZE);
       DataTest2_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BUFF_SIZE);
#endif

       for(i=0;i<BUFF_SIZE;i++)
       {
         DataTest1_p[i]=i;
         DataTest2_p[i]=0x00;
       }

      LBA_Address=0x00000000;
      Iter= HardInfo.AddressableSectors/NUMBER_OF_SECTORS;
      Remain=HardInfo.AddressableSectors%NUMBER_OF_SECTORS;
      STTBX_Print((" Iter= %d \n",Iter));
      Cmd.Features=0x00;
      Cmd.UseLBA= TRUE;

     for (j=0;j<Iter;j++)
     {

        Cmd.LBA=LBA_Address;

        Cmd.SectorCount= NUMBER_OF_SECTORS;
        Cmd.CmdCode= STATAPI_CMD_WRITE_MULTIPLE;

        Error = STATAPI_CmdOut(Drives[k].Handle,&Cmd,DataTest1_p,BUFF_SIZE,&NumberWrite,&CmdStatus);

        STOS_SemaphoreWait(StepSemaphore_p);

        if (Error != ST_NO_ERROR)
        {
            ATAPI_TestFailed(Result,"Error when writing multiple mode");
            STTBX_Print(("Error = "));
            DisplayError(Error);
            STTBX_Print(("\n"));
            goto end_multiple_soak;
        }

        if (NumberWrite != (BUFF_SIZE))
        {
            ATAPI_TestFailed(Result,"SoakTest failed when writing multiple mode");
            STTBX_Print(("Number written: %d\n",NumberWrite));
            goto end_multiple_soak;
        }

        Cmd.SectorCount=NUMBER_OF_SECTORS;
        Cmd.CmdCode= STATAPI_CMD_READ_MULTIPLE;

        Error = STATAPI_CmdIn(Drives[k].Handle,&Cmd,DataTest2_p,BUFF_SIZE,&NumberWrite,&CmdStatus);
        STOS_SemaphoreWait(StepSemaphore_p);

        if (Error != ST_NO_ERROR)
        {
            ATAPI_TestFailed(Result,"SoakTest failed when reading multiple mode");
            STTBX_Print(("Address %d \n",LBA_Address));
            goto end_multiple_soak;

        }

        if (NumberWrite != (BUFF_SIZE))
        {
             ATAPI_TestFailed(Result,"SoakTest failed when reading ");
             STTBX_Print(("Address %d \n",LBA_Address));
             goto end_multiple_soak;
        }


        if (memcmp(DataTest1_p,DataTest2_p,BUFF_SIZE))
        {
              STTBX_Print(("Data read doesn't match written :Address %d\n",LBA_Address));
        }
        LBA_Address+=NUMBER_OF_SECTORS;
        memset(DataTest2_p,0x00,BUFF_SIZE);

        if ((j%1000)==0)
        {
            STTBX_Print(("\nPercentage tested: %d \r",(j*100)/Iter ));
        }
        STTBX_Print((" j= %d \r",j));

        }
        if (Remain!=0)
        {
            STTBX_Print(("Almost all sectors checked only %d left\n",Remain));
        }
        ATAPI_TestPassed(Result);

end_multiple_soak:

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
    }

#if defined(DMA_PRESENT)
    STTBX_Print(("%d.2 DMA R/W Test \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure we can write data and then read it back \n"));
    STTBX_Print(("         in all the sectors using DMA\n"));

    {
        U8 *DataTest1_p, *DataTest2_p;
        U8 *store1, *store2;
        U32 i, j, LBA_Address, Iter, Remain;
        U32 NumberWrite = 0;
        U32 PercentArray[101];
        U32 PercentLevel = 0;
        STATAPI_Cmd_t Cmd;
        STATAPI_CmdStatus_t CmdStatus;

#undef   NUMBER_OF_SECTORS
#define  NUMBER_OF_SECTORS      254
#undef   BUFF_SIZE
#define  BUFF_SIZE              (NUMBER_OF_SECTORS / 2) * K

        store1 = (U8 *)STOS_MemoryAllocate(TINY_PARTITION_1, BUFF_SIZE);
        if (store1 == NULL)
        {
            STTBX_Print(("Couldn't allocate buffer1 for test\n"));
            goto end_dma_soak;
        }
        store2 = (U8 *)STOS_MemoryAllocate(TINY_PARTITION_1, BUFF_SIZE);
        if (store2 == NULL)
        {
            STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
            STTBX_Print(("Couldn't allocate buffer2 for test\n"));
            goto end_dma_soak;
        }

        /* Align to 256byte boundaries; only needs to be 128, really */
        DataTest1_p = (U8 *)(((U32)store1 + 255) & 0xffffff00);
        DataTest2_p = (U8 *)(((U32)store2 + 255) & 0xffffff00);

        STTBX_Print(("Telling STATAPI to use UDMA mode 4\n"));
        Error = STATAPI_SetDmaMode(Drives[k].Handle,
                                   STATAPI_DMA_UDMA_MODE_4);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result,"Unable to set UDMA mode 4");
            goto end_dma_soak;
        }

        STTBX_Print(("Programming drive to use UDMA mode 4\n"));
        Error = SetTransferModeDma(Drives[k].Handle,
                                   STATAPI_DMA_UDMA_MODE_4);
        if (CheckCodeOk(Error))
        {
            ATAPI_TestFailed(Result, "Unable to set UDMA mode 4 (2)");
            goto end_dma_soak;
        }

        for(i = 0; i < BUFF_SIZE; i++)
        {
            DataTest1_p[i] = i;
            DataTest2_p[i] = 0x00;
        }

        LBA_Address = 0x00000000;
        Iter = HardInfo.AddressableSectors / NUMBER_OF_SECTORS;
        Remain = HardInfo.AddressableSectors % NUMBER_OF_SECTORS;
        STTBX_Print(("Total number of sectors: %d\n",
                    HardInfo.AddressableSectors));
        STTBX_Print(("Number of reads/writes for %i sector chunks: %i\n",
                    NUMBER_OF_SECTORS, Iter));
        Cmd.Features = 0x00;
        Cmd.UseLBA = TRUE;

        for (i = 0; i < 101; i++)
        {
            PercentArray[i] = (HardInfo.AddressableSectors / 100) * i;
        }

        for (j = 0; j < Iter; j++)
        {

            Cmd.LBA = LBA_Address;

            Cmd.SectorCount = NUMBER_OF_SECTORS;
            Cmd.CmdCode = STATAPI_CMD_WRITE_DMA;

            Error = STATAPI_CmdOut(Drives[k].Handle, &Cmd, DataTest1_p,
                                   BUFF_SIZE, &NumberWrite, &CmdStatus);
            if (Error != ST_NO_ERROR)
            {
                ATAPI_TestFailed(Result,"Error when writing DMA command");
                STTBX_Print(("Error = "));
                DisplayError(Error);
                STTBX_Print(("\n"));
                goto end_dma_soak;
            }

            STOS_SemaphoreWait(StepSemaphore_p);

            if (Error != ST_NO_ERROR)
            {
                ATAPI_TestFailed(Result,"1. Error when writing DMA mode");
                STTBX_Print(("Error = "));
                DisplayError(Error);
                STTBX_Print(("\n"));
                goto end_dma_soak;
            }

            if (NumberWrite != (BUFF_SIZE))
            {
                ATAPI_TestFailed(Result,"2. SoakTest failed when writing DMA mode");
                STTBX_Print(("Number written: %d\n",NumberWrite));
                goto end_dma_soak;
            }

            Cmd.SectorCount = NUMBER_OF_SECTORS;
            Cmd.CmdCode = STATAPI_CMD_READ_DMA;

            Error = STATAPI_CmdIn(Drives[k].Handle, &Cmd, DataTest2_p,
                                  BUFF_SIZE, &NumberWrite, &CmdStatus);
            STOS_SemaphoreWait(StepSemaphore_p);

            if (Error != ST_NO_ERROR)
            {
                ATAPI_TestFailed(Result,
                                 "3. SoakTest failed when reading DMA mode");
                STTBX_Print(("Address %d \n", LBA_Address));
                goto end_dma_soak;

            }

            if (NumberWrite != (BUFF_SIZE))
            {
                 ATAPI_TestFailed(Result,"4. SoakTest failed when reading ");
                 STTBX_Print(("Address %d \n", LBA_Address));
                 goto end_dma_soak;
            }

            if (memcmp(DataTest1_p,DataTest2_p,BUFF_SIZE))
            {
                  STTBX_Print(("Data read doesn't match written :Address %d\n",
                              LBA_Address));
            }
            LBA_Address += NUMBER_OF_SECTORS;
            memset(DataTest2_p, 0x00, BUFF_SIZE);

            if (LBA_Address >= PercentArray[PercentLevel])
            {
                STTBX_Print(("%i%%\n", PercentLevel));
                PercentLevel++;
            }
        }
        if (Remain != 0)
        {
            STTBX_Print(("Almost all sectors checked only %d left\n",
                        Remain));
        }
        ATAPI_TestPassed(Result);

end_dma_soak:

        if (store1)
            STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
        if (store2)
            STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
    }
#endif

        Error = STATAPI_Close(Drives[k].Handle);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Close Failed \n"));
            goto soak_end;
        }

    } /*end if present and ATA*/
      AtapiOpenParams.DeviceAddress= STATAPI_DEVICE_1;

    }/* end for*/

soak_end:

    AtapiTermParams.ForceTerminate= TRUE;
    Error = STATAPI_Term(AtapiDevName,&AtapiTermParams);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Term Failed \n"));
    }

    return Result;
}

#endif
 /*=======================================================================
                PERFORMANCE TEST HELPER FUNCTIONS
        This functions perform a single step of the performance test
  =======================================================================*/



 /*************************************************************************
    SeekTime:

    This function returns the acummulated seek time in all the iterations
    The seek time here is defined as the time from the call to the STATAPI_CmdNoData
    function to the time when the event is received.

***************************************************************************/
static ST_ErrorCode_t SeekTime(STATAPI_Handle_t Handle, U16 Iter, U32 *Acum)
{

    ST_ErrorCode_t      Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U16                 i;

    clock_t time1, time2;
    clock_t *TimePool;

    Cmd.CmdCode = STATAPI_CMD_SEEK;
    Cmd.SectorCount = 0;
    Cmd.Features = 0x00;
    Cmd.UseLBA = FALSE;

    Cmd.Sector= AddressTable[0].Value.CHS_Value.Sector;
    Cmd.CylinderLow=  AddressTable[0].Value.CHS_Value.CylLow;
    Cmd.CylinderHigh= AddressTable[0].Value.CHS_Value.CylHigh;
    Cmd.Head= AddressTable[0].Value.CHS_Value.Head;

    TimePool = (clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1, (Iter * sizeof(clock_t)));
    if (TimePool == NULL)
    {
        STTBX_Print(("Not enough memory for seek test\n"));
        return ST_ERROR_NO_MEMORY;
    }

    Error = STATAPI_CmdNoData(Handle,&Cmd,&CmdStatus);
    STOS_SemaphoreWait(StepSemaphore_p);
    if ((Error !=ST_NO_ERROR) || (CmdStatus.Error !=0))
    {
        STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);
        return ST_ERROR_BAD_PARAMETER;
    }

    for(i = 0; i < Iter; i++)
    {
        Cmd.Sector = AddressTable[i].Value.CHS_Value.Sector;
        Cmd.CylinderLow =  AddressTable[i].Value.CHS_Value.CylLow;
        Cmd.CylinderHigh = AddressTable[i].Value.CHS_Value.CylHigh;
        Cmd.Head = AddressTable[i].Value.CHS_Value.Head;

        time1=time_now();

        Error = STATAPI_CmdNoData(Handle, &Cmd, &CmdStatus);
        STOS_SemaphoreWait(StepSemaphore_p);
        time2 = time_now();
        TimePool[i] = time_minus(time2, time1);

        if ((Error != ST_NO_ERROR) || (CmdStatus.Error != 0))
        {
            STTBX_Print(("Got error on iteration %i\nReturned error: ", i));
            DisplayError(Error);
            STTBX_Print(("; command status error: %i\n", CmdStatus.Error));
            DumpCmdStatus(CmdStatus);
            break;
        }
    }

    *Acum = 0;
    if (i == Iter)
    {
        /* Calculate average */
        for(i = 0; i < Iter; i++)
        {
            *Acum += (U32)TimePool[i];
        }

    }
    else
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }

    STOS_MemoryDeallocate(TEST_PARTITION_1, TimePool);

    return Error;
}

static ST_ErrorCode_t PacketSeekTime(STATAPI_Handle_t hATAPIDevHandle, U16 Iter, U32 *Acum)
{
    ST_ErrorCode_t      Error;
    STATAPI_Packet_t    Pkt;
    STATAPI_PktStatus_t PktStatus;
    U16 i;
    clock_t time1,time2;
    clock_t *TimePool = NULL;
    U8 StartSector= 180;

	memset(Pkt.Data,0,15);
	TimePool = (clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1, (Iter * sizeof(clock_t)));
    if (TimePool == NULL)
    {
        STTBX_Print(("Not enough memory for seek test\n"));
        return ST_ERROR_NO_MEMORY;
    }

  	Pkt.OpCode = STATAPI_PKT_SEEK;

	Pkt.Data [LSN_ADDRESS_OFFSET] 		= (U8)(StartSector >> 24 ) & 0xFF;
	Pkt.Data [LSN_ADDRESS_OFFSET + 1]   = (U8)(StartSector >> 16 ) & 0xFF;
	Pkt.Data [LSN_ADDRESS_OFFSET + 2]   = (U8)(StartSector >> 8 ) & 0xFF;
	Pkt.Data [LSN_ADDRESS_OFFSET + 3]   = (U8)(StartSector & 0xFF);


    Error = STATAPI_PacketNoData ( hATAPIDevHandle, &Pkt, &PktStatus );
    STOS_SemaphoreWait(StepSemaphore_p);

    if ((Error !=ST_NO_ERROR))
    {
        STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);
        return ST_ERROR_BAD_PARAMETER;
    }

    for(i = 0; i < Iter; i++)
    {
        time1=time_now();

        Error = STATAPI_PacketNoData ( hATAPIDevHandle, &Pkt, &PktStatus );
        STOS_SemaphoreWait(StepSemaphore_p);
        time2 = time_now();
        TimePool[i] = time_minus(time2, time1);

        if ((Error != ST_NO_ERROR) || (EvtError != ST_NO_ERROR))
        {
            STTBX_Print(("Got error on iteration %i\nReturned error: ", i));
            break;
        }
    }

    *Acum = 0;
    if (i == Iter)
    {
        /* Calculate average */
        for(i = 0; i < Iter; i++)
        {
            *Acum += (U32)TimePool[i];
        }
    }
    else
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }

    STOS_MemoryDeallocate(TEST_PARTITION_1, TimePool);
    return Error;
}

void DumpCmdStatus(STATAPI_CmdStatus_t CmdStatus)
{
    STTBX_Print(("Error: %i\n", CmdStatus.Error));
    STTBX_Print(("Status: %i\n", CmdStatus.Status));
    STTBX_Print(("LBA: %i\n", CmdStatus.LBA));
    STTBX_Print(("Sector: %i\n", CmdStatus.Sector));
    STTBX_Print(("CylinderLow: %i\n", CmdStatus.CylinderLow));
    STTBX_Print(("CylinderHigh: %i\n", CmdStatus.CylinderHigh));
    STTBX_Print(("Head: %i\n", CmdStatus.Head));
    STTBX_Print(("SectorCount: %i\n", CmdStatus.SectorCount));
}

/*************************************************************************
    WriteSeekTime:

    This function returns the acummulated seek time in all the iterations
    The seek time here is defined as the time from the call to the STATAPI_CmdOut
    function to the time when the event is received.
    This function writes one sector

***************************************************************************/
static ST_ErrorCode_t WriteSeekTime(STATAPI_Handle_t Handle,U16 Iter,U32 *Acum)
{

    ST_ErrorCode_t  Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U16 i;
    U8 *Data_p = NULL;
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp_p;    
#endif
    U32 BytesWritten = 0;

    clock_t time1;
    clock_t *TimePool = NULL;

    Cmd.CmdCode= STATAPI_CMD_WRITE_SECTORS;
    Cmd.SectorCount=1;
    Cmd.Features=0x00;
    Cmd.UseLBA= FALSE;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    Temp_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BLOCK_SIZE + 31);
    Data_p =(U8 *)(((U32)(Temp_p) + 31) & ~31);
#else/*!ATAPI_FDMA*/
    Data_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BLOCK_SIZE);
#endif
    if (NULL == Data_p)
    {
        STTBX_Print(("Failed to allocate memory for 'Data_p'"));
        return (ST_ERROR_NO_MEMORY);
    }

    TimePool=(clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1,(Iter*sizeof(clock_t)));
    if (NULL == TimePool)
    {   /* Error: clean-up & exit */
        STTBX_Print(("Failed to allocate memory for 'TimePool'"));
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Data_p);
    #else
        STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
    #endif
        return (ST_ERROR_NO_MEMORY);
    }

    for(i=0;i<BLOCK_SIZE;i++)
    {
        Data_p[i]=i;
    }

    for(i=0;i<Iter;i++)
    {
      Cmd.Sector= AddressTable[i].Value.CHS_Value.Sector;
      Cmd.CylinderLow=  AddressTable[i].Value.CHS_Value.CylLow;
      Cmd.CylinderHigh= AddressTable[i].Value.CHS_Value.CylHigh;
      Cmd.Head= AddressTable[i].Value.CHS_Value.Head;
      time1=time_now();

      Error =  STATAPI_CmdOut(Handle,&Cmd,Data_p,BLOCK_SIZE,&BytesWritten,&CmdStatus);
      STOS_SemaphoreWait(StepSemaphore_p);
      TimePool[i]= time_minus(time_now(),time1);

        if ((Error != ST_NO_ERROR) || (CmdStatus.Error != 0))
        {
            STTBX_Print(("Got error on iteration %i\nReturned error: ", i));
            DisplayError(Error);
            STTBX_Print(("; command status error: %i\n", CmdStatus.Error));
            DumpCmdStatus(CmdStatus);
            break;
        }
    }

    *Acum=0;
    if (i == Iter)
    {
        /* Calculate average */
        for (i = 0; i < Iter; i++)
        {
            *Acum += (U32 )TimePool[i];
        }
    }
    else
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }

    STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
#endif

   return Error;
}

static ST_ErrorCode_t ReadSeekTime(STATAPI_Handle_t Handle, U16 Iter, U32 *Acum)
{

    ST_ErrorCode_t  Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U16 i;
    U8 *Data1_p = NULL;
    U8 *Data2_p = NULL;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp1_p = NULL;
    U8 *Temp2_p = NULL;
#endif
    U32 BytesWritten;

    clock_t time1;
    clock_t *TimePool = NULL;

    Cmd.CmdCode= STATAPI_CMD_READ_SECTORS;
    Cmd.SectorCount=1;
    Cmd.Features=0x00;
    Cmd.UseLBA= FALSE;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    Temp1_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BLOCK_SIZE + 31);
    Temp2_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BLOCK_SIZE + 31);
    Data1_p =(U8 *)(((U32)(Temp1_p) + 31) & ~31);
    Data2_p =(U8 *)(((U32)(Temp2_p) + 31) & ~31);
#else
    Data1_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BLOCK_SIZE);
    Data2_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BLOCK_SIZE);
#endif

    TimePool=(clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1,(Iter*sizeof(clock_t)));

    for(i=0;i<BLOCK_SIZE;i++)
    {
        Data1_p[i]=i;
        Data2_p[i]=0x00;
    }

    for(i=0;i<Iter;i++)
    {

      Cmd.Sector= AddressTable[i].Value.CHS_Value.Sector;
      Cmd.CylinderLow=  AddressTable[i].Value.CHS_Value.CylLow;
      Cmd.CylinderHigh= AddressTable[i].Value.CHS_Value.CylHigh;
      Cmd.Head= AddressTable[i].Value.CHS_Value.Head;
      time1=time_now();

      Error =  STATAPI_CmdIn(Handle,&Cmd,Data2_p,BLOCK_SIZE,&BytesWritten,&CmdStatus);
      STOS_SemaphoreWait(StepSemaphore_p);
      TimePool[i]= time_minus(time_now(),time1);

        if ((Error != ST_NO_ERROR) || (CmdStatus.Error != 0))
        {
            STTBX_Print(("Got error on iteration %i\nReturned error: ", i));
            DisplayError(Error);
            STTBX_Print(("; command status error: %i\n", CmdStatus.Error));
            DumpCmdStatus(CmdStatus);
            break;
        }
      if (BytesWritten!=BLOCK_SIZE)  break;
      if (memcmp (Data1_p, Data2_p, BLOCK_SIZE)) break;

    }

    *Acum=0;

    if (i==Iter)
    {
        /* Calculate average */

        for(i=0;i<Iter;i++)
        {
            *Acum+=(U32)TimePool[i];
        }

    }else { Error =ST_ERROR_BAD_PARAMETER;}

    STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,Data1_p);
    STOS_MemoryDeallocate(TEST_PARTITION_1,Data2_p);
#endif

   return Error;
}

static ST_ErrorCode_t PacketReadSeekTime(STATAPI_Handle_t hATAPIDevHandle, U16 Iter, U32 *Acum ,U32 *throughput)
{
    ST_ErrorCode_t      Error;
    STATAPI_Packet_t    Pkt;
    STATAPI_PktStatus_t PktStatus;
    U16 i;
    U8 *Data1_p = NULL;
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp_p = NULL;
#endif        
    U32 NumberRW=0,Blocksize;
    clock_t time1;
    clock_t *TimePool = NULL;
    U8 StartSector=160;
    U8 NoOfSectors=3;

    memset( Pkt.Data, 0, 15 );
    if ( LastLBA > 0x100000 )
    {
       Blocksize = DVD_SECTOR_SIZE;
       Pkt.OpCode= STATAPI_PKT_READ_10;
       Pkt.Data[ATAPI_XFER_LENGTH_OFFSET + 3]=(U8)((NoOfSectors) & 0xFF);
    }
    else
    {
       Blocksize=CD_SECTOR_SIZE;
       Pkt.OpCode= STATAPI_PKT_READ_CD;
       Pkt.Data [ATAPI_XFER_LENGTH_OFFSET ]	    = (U8)((NoOfSectors >> 16)  & 0xFF);
	   Pkt.Data [ATAPI_XFER_LENGTH_OFFSET + 1]	= (U8)((NoOfSectors >> 8 )  & 0xFF);
	   Pkt.Data [ATAPI_XFER_LENGTH_OFFSET + 2]	= (U8)((NoOfSectors) & 0xFF);
    }

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    Temp_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,NoOfSectors*Blocksize+ 31);
    Data1_p =(U8 *)(((U32)(Temp_p) + 31) & ~31);
#else
    Data1_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,NoOfSectors*Blocksize);
#endif
    memset( Data1_p, 0, NoOfSectors*Blocksize);
    TimePool = (clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1, (Iter * sizeof(clock_t)));

    for(i=0;i<Iter;i++)
    {

             Pkt.Data [LSN_ADDRESS_OFFSET] 		=  (U8)(StartSector >> 24 ) & 0xFF;
        	 Pkt.Data [LSN_ADDRESS_OFFSET + 1]   = (U8)(StartSector >> 16 ) & 0xFF;
        	 Pkt.Data [LSN_ADDRESS_OFFSET + 2]   = (U8)(StartSector >> 8 ) & 0xFF;
        	 Pkt.Data [LSN_ADDRESS_OFFSET + 3]   = (U8)(StartSector & 0xFF);
             time1=time_now();

             Error = STATAPI_PacketIn(hATAPIDevHandle,&Pkt,Data1_p,
                                                    NoOfSectors*Blocksize,&NumberRW,&PktStatus);
             STOS_SemaphoreWait(StepSemaphore_p);
             TimePool[i]= time_minus(time_now(),time1);


             if (Error != ST_NO_ERROR)
             {
                    STTBX_Print((" Read  CD  failed \n"));
                    break;
             }


             if ((EvtError !=ST_NO_ERROR) && (NumberRW!= NoOfSectors*Blocksize))
             {
                   STTBX_Print(("\n Error when i=%d",i));
                   STTBX_Print(("\n Event error = %x, NumberRW=%d",EvtError,NumberRW));
                   break;
             }
             StartSector+=NoOfSectors;
             TimePool[i]=(TimePool[i]*1000000)/(ClocksPerSec);/*time in microsecs*/
      }

    *Acum=0;
    if (i==Iter)
    {
        /* Calculate average */

        for(i=0;i<Iter;i++)
        {
            *Acum+=(U32)TimePool[i];
        }


}else { Error =ST_ERROR_BAD_PARAMETER;}

   *throughput= (NoOfSectors*Blocksize*1000)/(*Acum/Iter);

    STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,Data1_p);
#endif
   return Error;
}

static ST_ErrorCode_t ThrPutReadSimple(STATAPI_Handle_t Handle,U16 Iter,U32 *Speed)
{
    ST_ErrorCode_t  Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 i,j,Temp;
    U8 *Data1_p = NULL;
    U32 BytesWritten,BytesToRW;
    clock_t time1;
    clock_t *TimePool = NULL;


#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp1_p = NULL;
    Temp1_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BuffSizes[BUFFER_SIZES-1] + 31);
    Data1_p =(U8 *)(((U32)(Temp1_p) + 31) & ~31);
#else/*!ATAPi_FDMA*/
    Data1_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BuffSizes[BUFFER_SIZES-1]);
#endif
    if (NULL == Data1_p)
    {
        STTBX_Print(("Failed to allocate memory for 'Data1_p'"));
        return (ST_ERROR_NO_MEMORY);
    }

    TimePool=(clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1,(Iter*sizeof(clock_t)));
    if (NULL == TimePool)
    {   /* Error: clean-up & exit */
        STTBX_Print(("Failed to allocate memory for 'TimePool'"));
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,Data1_p);
#endif
        return (ST_ERROR_NO_MEMORY);
    }

    Cmd.CmdCode= STATAPI_CMD_READ_SECTORS;

    Cmd.Features=0x00;
    Cmd.UseLBA= FALSE;


    for(i=0;i<BuffSizes[BUFFER_SIZES-1];i++)
    {
        Data1_p[i]=i;
    }


    for (j=0;j<BUFFER_SIZES;j++)
    {
        BytesToRW=BuffSizes[j];
        Cmd.SectorCount=BuffSizes[j]/BLOCK_SIZE;
        for(i=0;i<Iter;i++)
        {
          Cmd.Sector= AddressTable[i].Value.CHS_Value.Sector;
          Cmd.CylinderLow=  AddressTable[i].Value.CHS_Value.CylLow;
          Cmd.CylinderHigh= AddressTable[i].Value.CHS_Value.CylHigh;
          Cmd.Head= AddressTable[i].Value.CHS_Value.Head;
          time1=time_now();

          Error =  STATAPI_CmdIn(Handle,&Cmd,Data1_p,BytesToRW,&BytesWritten,&CmdStatus);
          STOS_SemaphoreWait(StepSemaphore_p);
          TimePool[i]= time_minus(time_now(),time1);

          if ((Error !=ST_NO_ERROR) ||
              (CmdStatus.Error !=0) ||
              (BytesWritten != BuffSizes[j]))
          {
              U8 Code;
              STTBX_Print(("%i, %i\n", j, i));
              STTBX_Print(("Buffer size: %i; sector count: %i (%i)\n",
                      BytesToRW,
                      Cmd.SectorCount, BuffSizes[j] / BLOCK_SIZE));
              STATAPI_GetExtendedError(Handle, &Code);
              STTBX_Print(("ExtError: 0x%02x\n", Code));
              STTBX_Print(("Command: 0x%02x\n", Cmd.CmdCode));
              DumpCmdStatus(CmdStatus);
              DisplayErrorNew(Error);
              break;
          }
          if (BytesWritten!=BuffSizes[j])  break;

        }

        Speed[j]=0;

        if (i==Iter)
        {
            float tempfloat;

            /* Add the time taken for all iterations together */
            for (i = 0; i < Iter; i++)
            {
                Speed[j] += (U32)TimePool[i];
            }

            Temp = BuffSizes[j] * Iter;    /* Total bytes */
            /* tempfloat is now time taken in seconds */
            tempfloat = (float)Speed[j] / (float)ClocksPerSec;
            tempfloat = (Temp / tempfloat);   /* Adjusted bytes for one second */
            tempfloat /= 1024;                /* Kbytes */
            Speed[j] = (U32)(floor(tempfloat));
        }
        else
        {
            Error =ST_ERROR_BAD_PARAMETER;
            break;
        }

    }
    STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,Data1_p);
#endif

   return Error;
}

#if defined(DMA_PRESENT)
static ST_ErrorCode_t ThrPutReadSimpleDma(STATAPI_Handle_t Handle,U16 Iter,U32 *Speed)
{

    ST_ErrorCode_t  Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 i,j,Temp;
    U8 *Data1_p = NULL, *tmp_p = NULL;
    U32 BytesWritten,BytesToRW;
    clock_t time1;
    clock_t *TimePool = NULL;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    tmp_p = (U8 *)STOS_MemoryAllocate(TINY_PARTITION_1,
                                  BuffSizes[BUFFER_SIZES - 1] + 256);
#else
    tmp_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BuffSizes[BUFFER_SIZES-1] + 256);
#endif
    Data1_p = (U8 *)(((U32)tmp_p + 255) & 0xffffff00);
    TimePool=(clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1,(Iter*sizeof(clock_t)));

    Cmd.CmdCode= STATAPI_CMD_READ_DMA;

    Cmd.Features=0x00;
    Cmd.UseLBA= FALSE;

    for(i=0;i<BuffSizes[BUFFER_SIZES-1];i++)
    {
        Data1_p[i]=i;
    }

    for (j=0;j<BUFFER_SIZES;j++)
    {
        BytesToRW=BuffSizes[j];
        Cmd.SectorCount=BuffSizes[j]/BLOCK_SIZE;
        for(i=0;i<Iter;i++)
        {
          Cmd.Sector= AddressTable[i].Value.CHS_Value.Sector;
          Cmd.CylinderLow=  AddressTable[i].Value.CHS_Value.CylLow;
          Cmd.CylinderHigh= AddressTable[i].Value.CHS_Value.CylHigh;
          Cmd.Head= AddressTable[i].Value.CHS_Value.Head;
          time1=time_now();

          Error =  STATAPI_CmdIn(Handle,&Cmd,Data1_p,BytesToRW,&BytesWritten,&CmdStatus);
          if (Error == ST_NO_ERROR)
              STOS_SemaphoreWait(StepSemaphore_p);

          TimePool[i]= time_minus(time_now(),time1);

          if ((Error !=ST_NO_ERROR) ||
              (CmdStatus.Error !=0) ||
              (BytesWritten != BuffSizes[j]))
          {
              U8 Code;
              STTBX_Print(("%i, %i\n", j, i));
              STTBX_Print(("Buffer size: %i; sector count: %i (%i)\n",
                      BytesToRW,
                      Cmd.SectorCount, BuffSizes[j] / BLOCK_SIZE));
              STATAPI_GetExtendedError(Handle, &Code);
              STTBX_Print(("ExtError: 0x%02x\n", Code));
              STTBX_Print(("Command: 0x%02x\n", Cmd.CmdCode));
              DumpCmdStatus(CmdStatus);
              DisplayErrorNew(Error);
              break;
          }
          if (BytesWritten!=BuffSizes[j])  break;

        }

        Speed[j]=0;

        if (i==Iter)
        {
            float tempfloat;

            /* Add the time taken for all iterations together */
            for (i = 0; i < Iter; i++)
            {
                Speed[j] += TimePool[i];
            }

            Temp = BuffSizes[j] * Iter;    /* Total bytes */
            /* tempfloat is now time taken in seconds */
            tempfloat = (float)Speed[j] / (float)ClocksPerSec;
            tempfloat = (Temp / tempfloat);   /* Adjusted bytes for one second */
            tempfloat /= 1024;                /* Kbytes */
            Speed[j] = (U32)(floor(tempfloat));
        }
        else
        {
            Error =ST_ERROR_BAD_PARAMETER;
            break;
        }

    }
    STOS_MemoryDeallocate(TEST_PARTITION_1, TimePool);
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1, tmp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1, tmp_p);
#endif

   return Error;
}
#endif

static ST_ErrorCode_t ThrPutWriteSimple(STATAPI_Handle_t Handle,U16 Iter,U32 *Speed)
{

    ST_ErrorCode_t  Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 i,j,Temp;
    U8 *Data_p = NULL;
    U32 BytesWritten,BytesToRW;
    clock_t time1;
    clock_t *TimePool;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp_p = NULL;
    Temp_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BuffSizes[BUFFER_SIZES-1] + 31);
    Data_p =(U8 *)(((U32)(Temp_p) + 31) & ~31);
#else
    Data_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BuffSizes[BUFFER_SIZES-1]);
#endif
    if (NULL == Data_p)
    {
        STTBX_Print(("Failed to allocate memory for 'Data_p'"));
        return (ST_ERROR_NO_MEMORY);
    }

    TimePool=(clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1,(Iter*sizeof(clock_t)));
    if (NULL == TimePool)
    {   /* Error: clean-up & exit */
        STTBX_Print(("Failed to allocate memory for 'TimePool'"));
    #if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
    #else
        STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
    #endif
        return (ST_ERROR_NO_MEMORY);
    }


    Cmd.CmdCode= STATAPI_CMD_WRITE_SECTORS;

    Cmd.Features=0x00;
    Cmd.UseLBA= FALSE;


    for (j=0;j<BUFFER_SIZES;j++)
    {
        BytesToRW=BuffSizes[j];
        Cmd.SectorCount=BuffSizes[j]/BLOCK_SIZE;
        for (i = 0; i < Iter; i++)
        {
            U32 loop;

            memset(Data_p, 0, BuffSizes[j]);

          Cmd.Sector= AddressTable[i].Value.CHS_Value.Sector;
          Cmd.CylinderLow=  AddressTable[i].Value.CHS_Value.CylLow;
          Cmd.CylinderHigh= AddressTable[i].Value.CHS_Value.CylHigh;
          Cmd.Head= AddressTable[i].Value.CHS_Value.Head;

          Error =  STATAPI_CmdOut(Handle,&Cmd,Data_p,BytesToRW,&BytesWritten,&CmdStatus);
          if (Error == ST_NO_ERROR)
              STOS_SemaphoreWait(StepSemaphore_p);
          else
          {
              STTBX_Print(("Failed to write: "));
              DisplayErrorNew(Error);
          }
          if ((CmdStatus.Error) || (EvtError != ST_NO_ERROR))
          {
              STTBX_Print(("Clear failed.\n"));
              goto jump;
          }
#if defined(ATAPI_DEBUG)
          if (ATAPI_Trace)
          {
              STTBX_Print(("Clear failed.\n"));
              goto jump;
          }
#endif

          for(loop=0;loop<BuffSizes[BUFFER_SIZES-1];loop++)
           Data_p[loop]=loop;

          time1=time_now();

          Error =  STATAPI_CmdOut(Handle,&Cmd,Data_p,BytesToRW,&BytesWritten,&CmdStatus);
          STOS_SemaphoreWait(StepSemaphore_p);
          TimePool[i]= time_minus(time_now(),time1);

jump:
#if defined(ATAPI_DEBUG)
            if (ATAPI_Trace)
            {
                    if (BytesWritten != 2048)
                    {
                        STTBX_Print(("%i %i\nCmdStatus:\n", j, i));
                        DumpCmdStatus(CmdStatus);
                        Cmd.CmdCode = STATAPI_CMD_READ_SECTORS;
                        Error = STATAPI_CmdIn(Handle, &Cmd, Data_p, BytesToRW,
                                              &BytesWritten, &CmdStatus);
                        STOS_SemaphoreWait(StepSemaphore_p);
                        Cmd.CmdCode = STATAPI_CMD_WRITE_SECTORS;
                        STTBX_Print(("CmdStatus:\n"));
                        DumpCmdStatus(CmdStatus);
                        STTBX_Print(("Read %i bytes", BytesWritten));
                        if (BytesWritten <= 2048)
                        {
                            U32 i;
                            for (i = 0; i < BytesToRW; i++)
                            {
                                if (i % 16 == 0)
                                {
                                    STTBX_Print(("\n0x%04x  ", i));
                                }
                                STTBX_Print(("%02x ", Data_p[i]));
                            }
                        }
                    }
            }
#endif

          if ((Error !=ST_NO_ERROR) || (CmdStatus.Error !=0))
          {
              STTBX_Print(("Error somewhere\n"));
              break;
          }
          if (BytesWritten!=BuffSizes[j])
          {
              STTBX_Print(("Byte count mismatch\n"));
              break;
          }
        }

        Speed[j]=0;

        /* If we completed all the writes... */
        if (i == Iter)
        {
            float tempfloat;

            /* Add the time taken for all iterations together */
            for (i = 0; i < Iter; i++)
            {
                Speed[j] += TimePool[i];
            }

            /* tempfloat is now time taken in seconds */
            tempfloat = (float)Speed[j] / (float)ClocksPerSec;
            Temp = BuffSizes[j] * Iter;    /* Total bytes */
            tempfloat = (Temp / tempfloat);          /* Adjusted bytes for one second */
            tempfloat /= 1024;                   /* Kbytes */
            Speed[j] = (U32)(floor(tempfloat));
        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER;
            break;
        }
    }
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
#endif

    STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);


   return Error;
}

static ST_ErrorCode_t ThrPutWriteMultiple(STATAPI_Handle_t Handle,U8 Mode,U32 *Speed)
{

    ST_ErrorCode_t  Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 i,j,StartJ,Temp;
    U8 *Data_p;
    U32 BytesWritten,BytesToRW;
    clock_t time1;
    clock_t *TimePool;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp_p = NULL;
    Temp_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,
                                BuffSizes[BUFFER_SIZES-1] + 256);
    Data_p = (U8 *)(((U32)Temp_p + 255) & 0xffffff00);
#else
    Data_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BuffSizes[BUFFER_SIZES-1]);
#endif

    TimePool=(clock_t*)STOS_MemoryAllocate(TEST_PARTITION_1,(ITERATION_NUMBER*sizeof(clock_t)));
    switch(Mode)
    {
        case 2:StartJ=0; break;
        case 4:StartJ=1; break;
        case 8:StartJ=2; break;
        case 16:StartJ=3; break;
    }

    for(i=0;i<BuffSizes[BUFFER_SIZES-1];i++)
    {
        Data_p[i]=i;
    }

        Cmd.SectorCount=Mode;
        Cmd.CmdCode= STATAPI_CMD_SET_MULTIPLE_MODE;

        Error = STATAPI_CmdNoData(Handle,&Cmd,&CmdStatus);
        STOS_SemaphoreWait(StepSemaphore_p);
        if (Error !=ST_NO_ERROR)
        {
           STTBX_Print(("Error Setting multiple mode \n"));
           return Error;
        }

        Cmd.CmdCode= STATAPI_CMD_WRITE_MULTIPLE;
        Cmd.Features=0x00;
        Cmd.UseLBA= FALSE;

        for (j=StartJ;j<BUFFER_SIZES;j++)
        {
            BytesToRW=BuffSizes[j];
            Cmd.SectorCount=BuffSizes[j]/BLOCK_SIZE;
            for(i=0;i<ITERATION_NUMBER;i++)
            {
              Cmd.Sector= AddressTable[i].Value.CHS_Value.Sector;
              Cmd.CylinderLow=  AddressTable[i].Value.CHS_Value.CylLow;
              Cmd.CylinderHigh= AddressTable[i].Value.CHS_Value.CylHigh;
              Cmd.Head= AddressTable[i].Value.CHS_Value.Head;
              time1=time_now();

              Error =  STATAPI_CmdOut(Handle,&Cmd,Data_p,BytesToRW,&BytesWritten,&CmdStatus);
              STOS_SemaphoreWait(StepSemaphore_p);
              TimePool[i]= time_minus(time_now(),time1);

              if ((Error !=ST_NO_ERROR) || (CmdStatus.Error !=0)) break;
              if (BytesWritten!=BuffSizes[j])  break;
            }

            Speed[j]=0;

            if (i==ITERATION_NUMBER)
            {
                float tempfloat;

                /* Add the time taken for all iterations together */
                for (i = 0; i < ITERATION_NUMBER; i++)
                {
                    Speed[j] += TimePool[i];
                }

                /* tempfloat is now time taken in seconds */
                tempfloat = (float)Speed[j] / (float)ClocksPerSec;
                Temp = BuffSizes[j] * ITERATION_NUMBER;    /* Total bytes */
                tempfloat = (Temp / tempfloat);          /* Adjusted bytes for one second */
                tempfloat /= 1024;                   /* Kbytes */
                Speed[j] = (U32)(floor(tempfloat));
            }else { Error =ST_ERROR_BAD_PARAMETER;  break;}

        }  /* end of buffer sizes iteration */


    STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
#endif

   return Error;
}

static ST_ErrorCode_t ThrPutReadMultiple(STATAPI_Handle_t Handle,
                                         U8 Mode,
                                         U32 *Speed)
{

    ST_ErrorCode_t      Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 i, j, StartJ, Temp;
    U8 *Data_p;
    U32 BytesWritten, BytesToRW;
    clock_t time1;
    clock_t *TimePool;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp_p = NULL;
    Temp_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,
                                BuffSizes[BUFFER_SIZES-1] + 256);
    Data_p = (U8 *)(((U32)Temp_p + 255) & 0xffffff00);
#else
    Data_p = (U8 *)STOS_MemoryAllocate(TEST_PARTITION_1,
                                   BuffSizes[BUFFER_SIZES - 1]);
#endif

    TimePool = (clock_t *)STOS_MemoryAllocate(TEST_PARTITION_1,\
                        (ITERATION_NUMBER * sizeof(clock_t)));
    if (TimePool == NULL)
    {
        STTBX_Print(("Couldn't allocate memory for timing data\n"));
        return ST_ERROR_NO_MEMORY;
    }
    switch(Mode)
    {
        case  2:    StartJ=0; break;
        case  4:    StartJ=1; break;
        case  8:    StartJ=2; break;
        case 16:    StartJ=3; break;
    }

    Cmd.SectorCount = Mode;
    Cmd.CmdCode = STATAPI_CMD_SET_MULTIPLE_MODE;

    Error = STATAPI_CmdNoData(Handle, &Cmd, &CmdStatus);
    STOS_SemaphoreWait(StepSemaphore_p);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Error Setting multiple mode \n"));
        return Error;
    }

    Cmd.CmdCode = STATAPI_CMD_READ_MULTIPLE;
    Cmd.Features = 0x00;
    Cmd.UseLBA = FALSE;

    for (j = StartJ; j < BUFFER_SIZES; j++)
    {
        BytesToRW = BuffSizes[j];
        Cmd.SectorCount = BuffSizes[j] / BLOCK_SIZE;

        for(i = 0; i < ITERATION_NUMBER; i++)
        {
            Cmd.Sector = AddressTable[i].Value.CHS_Value.Sector;
            Cmd.CylinderLow =  AddressTable[i].Value.CHS_Value.CylLow;
            Cmd.CylinderHigh = AddressTable[i].Value.CHS_Value.CylHigh;
            Cmd.Head = AddressTable[i].Value.CHS_Value.Head;
            time1 = time_now();

            Error = STATAPI_CmdIn(Handle, &Cmd, Data_p, BytesToRW,&BytesWritten, &CmdStatus);
            STOS_SemaphoreWait(StepSemaphore_p);
            TimePool[i] = time_minus(time_now(), time1);

            if ((Error != ST_NO_ERROR) || (CmdStatus.Error != 0))
                break;
            if (BytesWritten != BuffSizes[j])
                break;
        }

        Speed[j] = 0;

        if (i == ITERATION_NUMBER)
        {
            float tempfloat;

            /* Add the time taken for all iterations together */
            for (i = 0; i < ITERATION_NUMBER; i++)
                Speed[j] += TimePool[i];

            /* convert tempfloat to time taken in seconds */
            tempfloat = (float)Speed[j] / (float)ClocksPerSec;
            /* Total bytes */
            Temp = BuffSizes[j] * ITERATION_NUMBER;
            /* Adjust bytes for one second */
            tempfloat = (Temp / tempfloat);
            /* Convert to Kbytes */
            tempfloat /= 1024;
            Speed[j] = (U32)(floor(tempfloat));
        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER;
            break;
        }

    }  /* end of buffer sizes iteration */

    STOS_MemoryDeallocate(TEST_PARTITION_1, TimePool);
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1, Temp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1, Data_p);
#endif

   return Error;
}

#if defined(DMA_PRESENT)
static ST_ErrorCode_t ThrPutReadMultipleDma(STATAPI_Handle_t Handle,
                                            U8  Mode,
                                            U32 *Speed)
{

    ST_ErrorCode_t      Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 i, j, StartJ, Temp;
    U8 *Data_p, *tmp_p;
    U32 BytesWritten, BytesToRW;
    clock_t time1;
    clock_t *TimePool;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    tmp_p = (U8 *)STOS_MemoryAllocate(TINY_PARTITION_1,
                                   BuffSizes[BUFFER_SIZES - 1] + 256);
#else
    tmp_p = (U8 *)STOS_MemoryAllocate(TEST_PARTITION_1,
                                   BuffSizes[BUFFER_SIZES - 1] + 256);
#endif
    Data_p = (U8 *)(((U32)tmp_p + 255) & 0xffffff00);

    TimePool = (clock_t *)STOS_MemoryAllocate(TEST_PARTITION_1,
                        (ITERATION_NUMBER * sizeof(clock_t)));
    if (TimePool == NULL)
    {
        STTBX_Print(("Couldn't allocate memory for timing data\n"));
        return ST_ERROR_NO_MEMORY;
    }
    switch(Mode)
    {
        case  2:    StartJ=0; break;
        case  4:    StartJ=1; break;
        case  8:    StartJ=2; break;
        case 16:    StartJ=3; break;
    }

    Cmd.CmdCode = STATAPI_CMD_READ_DMA;
    Cmd.Features = 0x00;
    Cmd.UseLBA = FALSE;

    for (j = StartJ; j < BUFFER_SIZES; j++)
    {
        BytesToRW = BuffSizes[j];
        Cmd.SectorCount = BuffSizes[j] / BLOCK_SIZE;

        for(i = 0; i < ITERATION_NUMBER; i++)
        {
            Cmd.Sector = AddressTable[i].Value.CHS_Value.Sector;
            Cmd.CylinderLow =  AddressTable[i].Value.CHS_Value.CylLow;
            Cmd.CylinderHigh = AddressTable[i].Value.CHS_Value.CylHigh;
            Cmd.Head = AddressTable[i].Value.CHS_Value.Head;
            time1 = time_now();

            Error = STATAPI_CmdIn(Handle, &Cmd, Data_p, BytesToRW,
                                  &BytesWritten, &CmdStatus);
            if (Error == ST_NO_ERROR)
                STOS_SemaphoreWait(StepSemaphore_p);
            TimePool[i] = time_minus(time_now(), time1);

            if ((Error != ST_NO_ERROR) || (CmdStatus.Error != 0))
            {
                U8 ExtError;
                STTBX_Print(("Error on iteration %i\n", i));
                STTBX_Print(("Error: 0x%08x; CmdStatus.Error: 0x%08x\n",
                            Error, CmdStatus.Error));
                STATAPI_GetExtendedError(Handle, &ExtError);
                STTBX_Print(("Extended error code: 0x%02x\n", ExtError));
                break;
            }
            if (BytesWritten != BuffSizes[j])
                break;
        }

        Speed[j] = 0;

        if (i == ITERATION_NUMBER)
        {
            float tempfloat;

            /* Add the time taken for all iterations together */
            for (i = 0; i < ITERATION_NUMBER; i++)
                Speed[j] += TimePool[i];

            /* convert tempfloat to time taken in seconds */
            tempfloat = (float)Speed[j] / (float)ClocksPerSec;
            /* Total bytes */
            Temp = BuffSizes[j] * ITERATION_NUMBER;
            /* Adjust bytes for one second */
            tempfloat = (Temp / tempfloat);
            /* Convert to Kbytes */
            tempfloat /= 1024;
            Speed[j] = (U32)(floor(tempfloat));
        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER;
            break;
        }

    }  /* end of buffer sizes iteration */

    STOS_MemoryDeallocate(TEST_PARTITION_1, TimePool);

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1, tmp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1, tmp_p);
#endif
   return Error;
}

static ST_ErrorCode_t ThrPutWriteMultipleDma(STATAPI_Handle_t Handle,
                                             U8 Mode, U32 *Speed)
{
    ST_ErrorCode_t  Error;
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 i,j,StartJ,Temp;
    U8 *Data_p = NULL, *source,*tmp_p = NULL;
    U32 BytesWritten,BytesToRW;
    clock_t time1;
    clock_t *TimePool = NULL;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    tmp_p = (U8 *)STOS_MemoryAllocate(TINY_PARTITION_1,
                                   BuffSizes[BUFFER_SIZES - 1] + 256);
#else
    tmp_p = (U8 *)STOS_MemoryAllocate(TEST_PARTITION_1,
                                   BuffSizes[BUFFER_SIZES - 1] + 256);
#endif
    Data_p = (U8 *)(((U32)tmp_p + 255) & 0xffffff00);

    if (NULL == Data_p)
    {
        STTBX_Print(("Failed to allocate memory for 'Data_p'"));
        return (ST_ERROR_NO_MEMORY);
    }
    source = Data_p;
    TimePool = (clock_t *)STOS_MemoryAllocate(TEST_PARTITION_1,
                            (ITERATION_NUMBER * sizeof(clock_t)));
    if (NULL == TimePool)
    {   /* Error: clean-up & exit */
        STTBX_Print(("Failed to allocate memory for 'TimePool'"));
        STOS_MemoryDeallocate(TINY_PARTITION_1, source);
        return (ST_ERROR_NO_MEMORY);
    }

    switch(Mode)
    {
        case 2:StartJ=0; break;
        case 4:StartJ=1; break;
        case 8:StartJ=2; break;
        case 16:StartJ=3; break;
    }

    for(i=0;i<BuffSizes[BUFFER_SIZES-1];i++)
    {
        Data_p[i]=i;
    }

        Cmd.CmdCode= STATAPI_CMD_WRITE_DMA;
        Cmd.Features=0x00;
        Cmd.UseLBA= FALSE;

        for (j=StartJ;j<BUFFER_SIZES;j++)
        {
            BytesToRW=BuffSizes[j];
            Cmd.SectorCount=BuffSizes[j]/BLOCK_SIZE;
            for(i=0;i<ITERATION_NUMBER;i++)
            {
              Cmd.Sector= AddressTable[i].Value.CHS_Value.Sector;
              Cmd.CylinderLow=  AddressTable[i].Value.CHS_Value.CylLow;
              Cmd.CylinderHigh= AddressTable[i].Value.CHS_Value.CylHigh;
              Cmd.Head= AddressTable[i].Value.CHS_Value.Head;
              time1=time_now();

              Error =  STATAPI_CmdOut(Handle,&Cmd,Data_p,BytesToRW,&BytesWritten,&CmdStatus);
              STOS_SemaphoreWait(StepSemaphore_p);
              TimePool[i]= time_minus(time_now(),time1);

              if ((Error !=ST_NO_ERROR) || (CmdStatus.Error !=0)) break;
              if (BytesWritten!=BuffSizes[j])  break;
            }

            Speed[j]=0;

            if (i==ITERATION_NUMBER)
            {
                float tempfloat;

                /* Add the time taken for all iterations together */
                for (i = 0; i < ITERATION_NUMBER; i++)
                {
                    Speed[j] += (U32)TimePool[i];
                }

                /* tempfloat is now time taken in seconds */
                tempfloat = (float)Speed[j] / (float)ClocksPerSec;
                Temp = BuffSizes[j] * ITERATION_NUMBER;    /* Total bytes */
                tempfloat = (Temp / tempfloat);          /* Adjusted bytes for one second */
                tempfloat /= 1024;                   /* Kbytes */
                Speed[j] = (U32)(floor(tempfloat));
            }
            else
            {
                Error =ST_ERROR_BAD_PARAMETER;
                break;
            }

        }  /* end of buffer sizes iteration */
    STOS_MemoryDeallocate(TEST_PARTITION_1,TimePool);

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,tmp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,tmp_p);
#endif

   return Error;
}
#endif /* DMA_PRESENT */

static void  ParseAndShow(ATAPI_DeviceInfo_t *Out,U8 *Data)
{
    U32 W = 0;
    MaxLBA48_1 = 0;
    MaxLBA48_2 = 0;
    LBA48_Capable = FALSE;
	MaxLBA     = 0;
	SMART_Capable = FALSE;
    SMART_Enabled = FALSE;

    /* Lets grab Device specific info */
    STTBX_Print(("Removable: %d \n",Out->Removable=Data[0]& 0x80));
    memcpy(&Out->Heads,&Data[6],sizeof(U16));
    STTBX_Print(("Logical Heads: %d \n",Out->Heads));
    memcpy(&Out->SectorPerTrack,&Data[12],sizeof(U16));
    STTBX_Print(("Logical sectors per logical track: %d \n",Out->SectorPerTrack=Data[12]));
    memcpy(&Out->Cylinders,&Data[2],sizeof(U16));
    STTBX_Print(("Logical Cylinders: %d \n",Out->Cylinders));
    STTBX_Print(("DRQ Block Size: %d \n",Out->DRQBlockSize=Data[94]&0x00FF));
#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
    /* Comes out of drive byte-swapped */
    SwapString((U16 *)&Data[20], (U8 *)Out->SerialNumber, 10);
    SwapString((U16 *)&Data[54], (U8 *)Out->ModelNumber, 20);
    STTBX_Print(("Serial: %s\n", Out->SerialNumber));
    STTBX_Print(("Model: %s\n", Out->ModelNumber));
#else
    STTBX_Print(("Serial: %s \n",memcpy(Out->SerialNumber,&Data[20],20)));
    STTBX_Print(("Model: %s \n",memcpy(Out->ModelNumber,&Data[54],40)));
#endif
    memcpy(&Out->CurrentCylinders,&Data[108],sizeof(U16));
    STTBX_Print(("Current Logical Cyls: %d \n",Out->CurrentCylinders));
    memcpy(&Out->CurrentHeads,&Data[110],sizeof(U16));
    STTBX_Print(("Current Logical Heads: %d \n",Out->CurrentHeads));
    memcpy(&Out->CurrentSectorPerTrack,&Data[112],sizeof(U16));
    STTBX_Print(("Curent SecPerTrack: %d \n",Out->CurrentSectorPerTrack));
    memcpy(&Out->AddressableSectors,&Data[120],sizeof(U32));
    STTBX_Print(("Addressable Sectors (LBA): %d \n",Out->AddressableSectors));
    memcpy(&Out->Capacity,&Data[114],sizeof(U32));
    STTBX_Print(("Current capacity in sectors : %d \n",Out->Capacity));
    memcpy(&Out->QueueDepth,&Data[150],sizeof(U16));
    STTBX_Print(("Queue depth: %d \n",Out->QueueDepth));
    memcpy(&Out->MWDMA,&Data[126],sizeof(U16));
    STTBX_Print(("MWDMA: 0x%x \n",Out->MWDMA=Data[126]));
    memcpy(&Out->PIO,&Data[128],sizeof(U16));
    STTBX_Print(("PIO: 0x%x \n",Out->PIO=Data[128]));
    memcpy(&Out->UDMA,&Data[176],sizeof(U16));
    STTBX_Print(("UDMA: 0x%x \n",Out->UDMA=Data[176]));

    /* Word 60, but data is in bytes. Same for others. */
    memcpy(&MaxLBA, &Data[60 * 2], sizeof(U32));

    /* Not using self-test or error logging, so we only check whether
     * the disk has basic SMART.
     */
     
    memcpy(&W, &Data[82 * 2], sizeof(U32));
    SMART_Capable = ((W & 0x0001) != 0);
    STTBX_Print(("SMART capable: %s; ",
                (SMART_Capable == TRUE?"yes":"no")));
              
    memcpy(&W, &Data[85 * 2], sizeof(U32));
    SMART_Enabled = ((W & 0x0001) != 0);
    STTBX_Print(("SMART enabled: %s\n",
                (SMART_Enabled == TRUE?"yes":"no")));
    /* These are set for later use by the ATAPI_LBA48Test function */
    memcpy(&W, &Data[83 * 2], sizeof(U16));
    LBA48_Capable = ((W & 0x0400) != 0);
    /* Words 100, 101. Note - supporting this word is optional. We hope
     * the drive does...
     */
    memcpy(&MaxLBA48_2, &Data[100 * 2], sizeof(U32));
    /* Words 102, 103 */
    memcpy(&MaxLBA48_1, &Data[102 * 2], sizeof(U32));
    STTBX_Print(("LBA-48 capable: %s\n",
                (LBA48_Capable == TRUE?"yes":"no")));
             
    if (LBA48_Capable == TRUE)
    {
        STTBX_Print(("Maximum LBA-48 sector: %u%u", MaxLBA48_1,
                    MaxLBA48_2));
        if ((MaxLBA48_1 == 0) && (MaxLBA48_2 == MaxLBA))
        {
            STTBX_Print((" - \"fake\" LBA-48, same size\n"));
        }
        else
        {
            STTBX_Print(("\n"));
        }
    }
    else
    {
        STTBX_Print(("Maximum LBA sector: %u\n", MaxLBA));
    }
  

    STTBX_Print(("------------------------------------------------------\n"));
 }

/*************************************************************
    This function performs Write and Read Test in  a specified address
    using the multiple mode set to 2 blocks per IRQ

*************************************************************/
ST_ErrorCode_t  MultipleRWTest(STATAPI_Handle_t Handle,
                               ATAPI_Address_t *Addr,
                               U8 BlockNo,U8 Mode)
{

    ST_ErrorCode_t Error;
    U8 *DataTest1_p;
    U8 *DataTest2_p;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp1_p,*Temp2_p;
#endif
    U32 i;
    STATAPI_Cmd_t Cmd;
    U32 NumberWrite=0;
    STATAPI_CmdStatus_t CmdStatus;

    Cmd.SectorCount=Mode;
    Cmd.CmdCode= STATAPI_CMD_SET_MULTIPLE_MODE;
    STTBX_Print(("Setting multiple mode  : "));
    Error = STATAPI_CmdNoData(Handle,&Cmd,&CmdStatus);
    CheckCodeOk(Error);
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }
    STOS_SemaphoreWait(StepSemaphore_p);

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    Temp1_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BlockNo*BLOCK_SIZE + 31);
    Temp2_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,BlockNo*BLOCK_SIZE + 31);
    DataTest1_p=(U8 *)(((U32)(Temp1_p) + 31) & ~31);
    DataTest2_p= (U8 *)(((U32)(Temp2_p) + 31) & ~31);
#else
    DataTest1_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BlockNo*BLOCK_SIZE);
    DataTest2_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,BlockNo*BLOCK_SIZE);
#endif

    for(i=0;i<BlockNo*BLOCK_SIZE;i++)
    {
        DataTest1_p[i]=i;
        DataTest2_p[i]=0x00;
    }
    Cmd.Features=0x00;
    Cmd.UseLBA= Addr->LBA;
    if (Addr->LBA)
    {
     Cmd.LBA=Addr->Value.LBA_Value;
    }else
    {
        Cmd.Sector= Addr->Value.CHS_Value.Sector;
        Cmd.CylinderLow=  Addr->Value.CHS_Value.CylLow;
        Cmd.CylinderHigh= Addr->Value.CHS_Value.CylHigh;
        Cmd.Head= Addr->Value.CHS_Value.Head;
    }

    Cmd.SectorCount= BlockNo;
    Cmd.CmdCode= STATAPI_CMD_WRITE_MULTIPLE;

    STTBX_Print(("Writing Data  : "));


    Error = STATAPI_CmdOut(Handle,&Cmd,DataTest1_p,BlockNo*BLOCK_SIZE,&NumberWrite,&CmdStatus);

    STOS_SemaphoreWait(StepSemaphore_p);

    if (Error != ST_NO_ERROR)
    {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        return Error;
    }


    if (NumberWrite != (BlockNo*BLOCK_SIZE))
    {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        return CmdStatus.Error;
    }
    #ifdef RW_INFO
    ShowStatus(&CmdStatus);
    #endif

    Cmd.SectorCount=BlockNo;
    Cmd.CmdCode= STATAPI_CMD_READ_MULTIPLE;


    STTBX_Print(("Reading Data  :  "));


    Error = STATAPI_CmdIn(Handle,&Cmd,DataTest2_p,BlockNo*BLOCK_SIZE,&NumberWrite,&CmdStatus);
    STOS_SemaphoreWait(StepSemaphore_p);

    if (Error != ST_NO_ERROR)
    {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        return Error;
    }


    if (NumberWrite != (BlockNo*BLOCK_SIZE))
    {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        return CmdStatus.Error;
    }

    #ifdef RW_INFO
    ShowStatus(&CmdStatus);
    #endif


    if (CompareData (DataTest1_p,DataTest2_p,BlockNo*BLOCK_SIZE))
    {
        Error = ST_NO_ERROR;
    }else
    {
       Error = STATAPI_ERROR_CMD_ERROR;
    }


#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
    STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
    return Error;

}

#if defined(DMA_PRESENT)
/* Zeroes the given sectors */
static ST_ErrorCode_t ClearSectors(STATAPI_Handle_t Handle,
                                   ATAPI_Address_t *Addr,
                                   U8 BlockNo)
{
    ST_ErrorCode_t Error;
    STATAPI_Cmd_t Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 NumberWrite = 0;
    U8 *DataTest1_p;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp1_p;
    Temp1_p = (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,
                                       BlockNo * BLOCK_SIZE + 256);
    DataTest1_p = (U8 *)(((U32)Temp1_p + 255) & 0xffffff00);
#else
    DataTest1_p = (U8*)STOS_MemoryAllocate(TEST_PARTITION_1, BlockNo * BLOCK_SIZE);
#endif

    memset(DataTest1_p, 0, BlockNo * BLOCK_SIZE);

    Cmd.Features = 0x00;
    Cmd.UseLBA = Addr->LBA;
    if (Cmd.UseLBA)
    {
        Cmd.LBA = (U32)Addr->Value.LBA_Value;
    }
    else
    {
        Cmd.Sector = Addr->Value.CHS_Value.Sector;
        Cmd.CylinderLow =  Addr->Value.CHS_Value.CylLow;
        Cmd.CylinderHigh = Addr->Value.CHS_Value.CylHigh;
        Cmd.Head = Addr->Value.CHS_Value.Head;
    }
    Cmd.SectorCount = BlockNo;

    Cmd.CmdCode = STATAPI_CMD_WRITE_SECTORS;

#ifdef RW_INFO
    STTBX_Print(("Clearing sector(s): "));
#endif
    Error = STATAPI_CmdOut(Handle, &Cmd, DataTest1_p, BlockNo * BLOCK_SIZE,
                           &NumberWrite, &CmdStatus);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("Error sending command: "));
        DisplayErrorNew(Error);
    }
    else
    {
        STOS_SemaphoreWait(StepSemaphore_p);

        if (NumberWrite != (BlockNo * BLOCK_SIZE))
        {
            Error = CmdStatus.Error;
            STTBX_Print(("Error occurred: "));
            DisplayErrorNew(Error);
        }

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
#endif

#ifdef RW_INFO
        ShowStatus(&CmdStatus);
#endif
    }

    return Error;
}
#endif

/************************************************************
*    SimpleRWTest
************************************************************/
ST_ErrorCode_t  SimpleRWTest(STATAPI_Handle_t Handle,
                             ATAPI_Address_t *Addr,
                             U8 BlockNo)
{
    ST_ErrorCode_t Error;
    U8 *DataTest1_p;
    U8 *DataTest2_p;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Temp1_p,*Temp2_p;
#endif
    U32 SectorCount;
    U32 i;

    STATAPI_Cmd_t Cmd;
    U32 NumberWrite=0;
    STATAPI_CmdStatus_t CmdStatus;

    if (BlockNo == 0)
        SectorCount = 256;
    else
        SectorCount = BlockNo;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)

    Temp1_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,SectorCount*BLOCK_SIZE + 256);
    Temp2_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,SectorCount*BLOCK_SIZE + 256);
    DataTest1_p = (U8 *)(((U32)Temp1_p + 255) &  0xffffff00);
    DataTest2_p = (U8 *)(((U32)Temp2_p + 255) &  0xffffff00);

#else
    if (BlockNo > 0)
    {
        DataTest1_p = (U8 *)STOS_MemoryAllocate(TEST_PARTITION_1,
                                            BlockNo * BLOCK_SIZE);
        if (DataTest1_p != NULL)
        {
            DataTest2_p = (U8 *)STOS_MemoryAllocate(TEST_PARTITION_1,
                                                BlockNo * BLOCK_SIZE);
            if (DataTest2_p == NULL)
            {
                STOS_MemoryDeallocate(TEST_PARTITION_1, DataTest1_p);
                DataTest1_p = NULL;
            }
        }
    }
    else
    {
        DataTest1_p = BigBuffer1;
        DataTest2_p = BigBuffer2;
    }
#endif

    if ((DataTest1_p == NULL) || (DataTest2_p == NULL))
    {
        STTBX_Print(("Unable to allocate memory for test\n"));
        return ST_ERROR_NO_MEMORY;
    }

    for(i=0;i<SectorCount*BLOCK_SIZE;i++)
    {
        DataTest1_p[i]=i;
        DataTest2_p[i]=0x00;
    }

    Cmd.Features=0x00;
    Cmd.UseLBA= Addr->LBA;
    if (Cmd.UseLBA)
    {
        Cmd.LBA =(U32) Addr->Value.LBA_Value;
    }else
    {
        Cmd.Sector= Addr->Value.CHS_Value.Sector;
        Cmd.CylinderLow=  Addr->Value.CHS_Value.CylLow;
        Cmd.CylinderHigh= Addr->Value.CHS_Value.CylHigh;
        Cmd.Head= Addr->Value.CHS_Value.Head;
    }
    Cmd.SectorCount= BlockNo;

    Cmd.CmdCode= STATAPI_CMD_WRITE_SECTORS;

#if defined(RW_INFO)
    STTBX_Print(("Clearing sector: "));
#endif
    Error = STATAPI_CmdOut(Handle, &Cmd, DataTest2_p, SectorCount * BLOCK_SIZE,
                           &NumberWrite, &CmdStatus);
    if (Error != ST_NO_ERROR)
    {
        if (BlockNo != 0)
        {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        }
        return Error;
    }

    STOS_SemaphoreWait(StepSemaphore_p);

    if (NumberWrite != (SectorCount*BLOCK_SIZE))
    {
        if (BlockNo != 0)
        {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        }
        return CmdStatus.Error;
    }

#if defined(RW_INFO)
    ShowStatus(&CmdStatus);
    STTBX_Print(("Writing Data  : "));
#endif

    Error = STATAPI_CmdOut(Handle, &Cmd, DataTest1_p, SectorCount * BLOCK_SIZE,
                           &NumberWrite, &CmdStatus);
    if (Error != ST_NO_ERROR)
    {
        if (BlockNo != 0)
        {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        }
        return Error;
    }

    STOS_SemaphoreWait(StepSemaphore_p);

    if (NumberWrite != (SectorCount*BLOCK_SIZE))
    {
        if (BlockNo != 0)
        {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        }
        return CmdStatus.Error;
    }
#if defined(RW_INFO)
    ShowStatus(&CmdStatus);
    STTBX_Print(("Reading Data  :  "));
#endif

    Cmd.SectorCount=BlockNo;
    Cmd.CmdCode= STATAPI_CMD_READ_SECTORS;
    NumberWrite=0;

    /* DataTest2_p still zeroed from above */
    Error = STATAPI_CmdIn(Handle, &Cmd, DataTest2_p, SectorCount * BLOCK_SIZE,
                        &NumberWrite, &CmdStatus);
    if (Error != ST_NO_ERROR)
    {
        if (BlockNo != 0)
        {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        }
        return Error;
    }
    STOS_SemaphoreWait(StepSemaphore_p);

    if (NumberWrite != (SectorCount*BLOCK_SIZE))
    {
        if (BlockNo != 0)
        {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        }
        return CmdStatus.Error;
    }

#ifdef RW_INFO
    ShowStatus(&CmdStatus);
    STTBX_Print(("Comparing Data  :   "));
#endif
    if (CompareData (DataTest1_p,DataTest2_p,BlockNo*BLOCK_SIZE))
    {
        Error = ST_NO_ERROR;
    }else
    {
       Error = STATAPI_ERROR_CMD_ERROR;
    }

#ifdef RW_INFO
    STTBX_Print(("Clearing sector: "));
#endif
    memset(DataTest2_p, 0, BlockNo * BLOCK_SIZE);
    Cmd.CmdCode= STATAPI_CMD_WRITE_SECTORS;
    Error = STATAPI_CmdOut(Handle, &Cmd, DataTest2_p, SectorCount * BLOCK_SIZE,
                           &NumberWrite, &CmdStatus);
    if (Error != ST_NO_ERROR)
    {
        if (BlockNo != 0)
        {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        }
        return Error;
    }

    STOS_SemaphoreWait(StepSemaphore_p);

    if (NumberWrite != (SectorCount*BLOCK_SIZE))
    {
        if (BlockNo != 0)
        {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
            STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
            STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
        }
        return CmdStatus.Error;
    }
#ifdef RW_INFO
    ShowStatus(&CmdStatus);
#endif

    if (BlockNo != 0)
    {
#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp1_p);
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp2_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest1_p);
        STOS_MemoryDeallocate(TEST_PARTITION_1,DataTest2_p);
#endif
    }

    return Error;
}

#if defined(DMA_PRESENT)
/************************************************************
Name: SimpleRWTestDma

Description:
    Performs a simple write/read/verify cycle, using DMA mode
    transfers. Won't work if BlockNo == 0 - in that case,
    Rw256SectorsDma is the one to call.

Returns:
    ST_NO_ERROR on success
    Anything else is failure.

Notes:
*************************************************************/
ST_ErrorCode_t SimpleRWTestDma(STATAPI_Handle_t Handle,
                               ATAPI_Address_t *Addr,
                               U8 BlockNo)
{
    ST_ErrorCode_t Error;
    STATAPI_Cmd_t Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 NumberWrite = 0;
    U32 i;
    U16 SectorCount;
    U8 *DataTest1_p = NULL, *store1;
    U8 *DataTest2_p = NULL, *store2;

    /* SectorCount is more readable. And can be changed if we want to
     * do stuff with BlockNo == 0 (== 256 sectors)
     */
    SectorCount = BlockNo;

    /* Allocate from ncache region */
    store1 = STOS_MemoryAllocate(TINY_PARTITION_1,
                             (SectorCount * BLOCK_SIZE) + 255);
    if (store1 == NULL)
    {
        STTBX_Print(("Failed to allocate store1\n"));
        return ST_ERROR_NO_MEMORY;
    }
    store2 = STOS_MemoryAllocate(TINY_PARTITION_1,
                             (SectorCount * BLOCK_SIZE) + 255);
    if (store2 == NULL)
    {
        STTBX_Print(("Failed to allocate store2\n"));
        STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
        return ST_ERROR_NO_MEMORY;
    }

    /* Align to 256byte boundaries; only needs to be 128, really */
    DataTest1_p = (U8 *)(((U32)store1 + 255) & 0xffffff00);
    DataTest2_p = (U8 *)(((U32)store2 + 255) & 0xffffff00);

    for (i = 0; i < SectorCount * BLOCK_SIZE; i++)
    {
        DataTest1_p[i] = i;
        DataTest2_p[i] = 0x00;
    }

    Cmd.Features = 0x00;
    Cmd.UseLBA = Addr->LBA;
    if (Cmd.UseLBA)
    {
        Cmd.LBA = (U32)Addr->Value.LBA_Value;
    }
    else
    {
        Cmd.Sector = Addr->Value.CHS_Value.Sector;
        Cmd.CylinderLow =  Addr->Value.CHS_Value.CylLow;
        Cmd.CylinderHigh = Addr->Value.CHS_Value.CylHigh;
        Cmd.Head = Addr->Value.CHS_Value.Head;
    }

    Cmd.SectorCount = BlockNo;
    Cmd.CmdCode = STATAPI_CMD_WRITE_DMA;

    /* UDMA writes don't work on this cut, so we skip them */
#if defined(HDDI_5514_CUT_3_0)
    if (IsUDMA == FALSE)
#endif
    {
#ifdef RW_INFO
        STTBX_Print(("Writing Data  : "));
#endif
        Error = STATAPI_CmdOut(Handle, &Cmd, DataTest1_p,
                               SectorCount * BLOCK_SIZE,
                               &NumberWrite, &CmdStatus);
        if (Error != ST_NO_ERROR)
        {
            STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
            STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
            return Error;
        }

        STOS_SemaphoreWait(StepSemaphore_p);

        if ((NumberWrite != (SectorCount * BLOCK_SIZE)) ||
            (EvtError != ST_NO_ERROR))
        {
            U8 ExtError;

            STTBX_Print(("Didn't write all bytes; wrote %i, wanted %i (error %u)\n",
                        NumberWrite, (SectorCount * BLOCK_SIZE), EvtError));
            STATAPI_GetExtendedError(Handle, &ExtError);
            STTBX_Print(("Extended error code: 0x%02x\n", ExtError));
            STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
            STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
            if (EvtError != ST_NO_ERROR)
                return EvtError;
            else if (CmdStatus.Error != 0)
                return CmdStatus.Error;
            else
                return STATAPI_ERROR_CMD_ERROR;
        }
    }

#ifdef RW_INFO
    ShowStatus(&CmdStatus);
    STTBX_Print(("Reading Data  :  "));
#endif

    Cmd.SectorCount = BlockNo;
    Cmd.CmdCode = STATAPI_CMD_READ_DMA;
    NumberWrite = 0;

    Error = STATAPI_CmdIn(Handle, &Cmd, DataTest2_p, SectorCount * BLOCK_SIZE,
                          &NumberWrite, &CmdStatus);
    if (Error != ST_NO_ERROR)
    {
        STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
        STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
        return Error;
    }

    STOS_SemaphoreWait(StepSemaphore_p);

    if ((NumberWrite != (SectorCount * BLOCK_SIZE)) ||
        (EvtError != ST_NO_ERROR))
    {
        STTBX_Print(("Didn't read back all bytes; read %i, wanted %i (error %u)\n",
                    NumberWrite, (SectorCount * BLOCK_SIZE), EvtError));
        STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
        STOS_MemoryDeallocate(TINY_PARTITION_1, store2);
        if (CmdStatus.Error != 0)
            return CmdStatus.Error;
        else if (EvtError != ST_NO_ERROR)
            return EvtError;
        else
            return STATAPI_ERROR_CMD_ERROR;
    }

#ifdef RW_INFO
    ShowStatus(&CmdStatus);

    STTBX_Print(("Comparing Data  :   "));
#endif
    if (CompareData(DataTest1_p, DataTest2_p, SectorCount * BLOCK_SIZE))
    {
        Error = ST_NO_ERROR;
    }
    else
    {
        STTBX_Print(("Data doesn't match\n"));
        Error = STATAPI_ERROR_CMD_ERROR;
    }

    STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
    STOS_MemoryDeallocate(TINY_PARTITION_1, store2);

    return Error;
}

/************************************************************
Name: Rw256SectorsDma

Description:
    Tries to read and write 256-sectors using DMA. Needs to
    be a separate function from SimpleRWTestDma() due to the
    need for large uncached buffers - in unified memory we
    can only get one of those, so this test was moved to here

Returns:
    ST_NO_ERROR on success
    Anything else is failure.

Notes:
*************************************************************/
static ST_ErrorCode_t Rw256SectorsDma(STATAPI_Handle_t Handle,
                                      ATAPI_Address_t *Addr)
{
    ST_ErrorCode_t Error;
    STATAPI_Cmd_t Cmd;
    STATAPI_CmdStatus_t CmdStatus;
    U32 NumberWrite = 0;
    U32 i;
    /* It's not going to change, but this makes the code more readable */
    U16 SectorCount = 256;
    U8 *DataTest1_p = NULL, *store1 = NULL;

    /* Allocate from ncache region */
    store1 = STOS_MemoryAllocate(TINY_PARTITION_1,
                             (SectorCount * BLOCK_SIZE) + 255);
    if (store1 == NULL)
    {
        STTBX_Print(("Failed to allocate store1\n"));
        return ST_ERROR_NO_MEMORY;
    }

    /* Align to 256byte boundaries; only needs to be 128, really */
    DataTest1_p = (U8 *)(((U32)store1 + 255) & 0xffffff00);

    for (i = 0; i < SectorCount * BLOCK_SIZE; i++)
        DataTest1_p[i] = i;

    Cmd.Features = 0x00;
    Cmd.UseLBA = Addr->LBA;
    if (Cmd.UseLBA)
    {
        Cmd.LBA = (U32)Addr->Value.LBA_Value;
    }
    else
    {
        Cmd.Sector = Addr->Value.CHS_Value.Sector;
        Cmd.CylinderLow =  Addr->Value.CHS_Value.CylLow;
        Cmd.CylinderHigh = Addr->Value.CHS_Value.CylHigh;
        Cmd.Head = Addr->Value.CHS_Value.Head;
    }

    /* 0 sectorcount == 256 sectors */
    Cmd.SectorCount = 0;
    Cmd.CmdCode = STATAPI_CMD_WRITE_DMA;

    /* UDMA writes don't work on this cut, so we skip it */
#if defined(HDDI_5514_CUT_3_0)
    if (IsUDMA == FALSE)
#endif
    {
#ifdef RW_INFO
        STTBX_Print(("Writing Data  : "));
#endif
        Error = STATAPI_CmdOut(Handle, &Cmd, DataTest1_p,
                               SectorCount * BLOCK_SIZE,
                               &NumberWrite, &CmdStatus);
        if (Error != ST_NO_ERROR)
        {
            STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
            return Error;
        }

        STOS_SemaphoreWait(StepSemaphore_p);

        if ((NumberWrite != (SectorCount * BLOCK_SIZE)) ||
            (EvtError != ST_NO_ERROR))
        {
            STTBX_Print(("Didn't write all bytes; wrote %i, wanted %i (error %u)\n",
                        NumberWrite, (SectorCount * BLOCK_SIZE), EvtError));
            STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
            if (EvtError != ST_NO_ERROR)
                return EvtError;
            else if (CmdStatus.Error != 0)
                return CmdStatus.Error;
            else
                return STATAPI_ERROR_CMD_ERROR;
        }
    }

#ifdef RW_INFO
    ShowStatus(&CmdStatus);
    STTBX_Print(("Reading Data  :  "));
#endif

    /* Clear the buffer out */
    for (i = 0; i < SectorCount * BLOCK_SIZE; i++)
        DataTest1_p[i] = i;

    /* 0 sectorcount == 256 sectors */
    Cmd.SectorCount = 0;
    Cmd.CmdCode = STATAPI_CMD_READ_DMA;
    NumberWrite = 0;

    Error = STATAPI_CmdIn(Handle, &Cmd, DataTest1_p, SectorCount * BLOCK_SIZE,
                          &NumberWrite, &CmdStatus);
    if (Error != ST_NO_ERROR)
    {
        STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
        return Error;
    }

    STOS_SemaphoreWait(StepSemaphore_p);

    if ((NumberWrite != (SectorCount * BLOCK_SIZE)) ||
        (EvtError != ST_NO_ERROR))
    {
        STTBX_Print(("Didn't read back all bytes; read %i, wanted %i (error %u)\n",
                    NumberWrite, (SectorCount * BLOCK_SIZE), EvtError));

        STOS_MemoryDeallocate(TINY_PARTITION_1, store1);
        if (CmdStatus.Error != 0)
            return CmdStatus.Error;
        else if (EvtError != ST_NO_ERROR)
            return EvtError;
        else
            return STATAPI_ERROR_CMD_ERROR;
    }

#ifdef RW_INFO
    ShowStatus(&CmdStatus);

    STTBX_Print(("Comparing Data  :   "));
#endif

    for (i = 0; i < SectorCount * BLOCK_SIZE; i++)
    {
        if (DataTest1_p[i] != (i % 256))
        {
            STTBX_Print(("Data doesn't match (first point %i - got %i, wanted %i)\n", i, DataTest1_p[i], i));
            break;
        }
    }
    if (i == (SectorCount * BLOCK_SIZE))
        Error = ST_NO_ERROR;
    else
        Error = STATAPI_ERROR_CMD_ERROR;

    STOS_MemoryDeallocate(TINY_PARTITION_1, store1);

    return Error;
}
#endif /* defined(DMA_PRESENT) */

/************************************************************
Name: ATAPI_LBA48Test

Description:
    Test the LBA-48 commands on the drive (assuming the disk
    supports them).

Returns:

Notes:
*************************************************************/
static ATAPI_TestResult_t ATAPI_LBA48Test(ATAPI_TestParams_t * TestParams)
{
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;
    STATAPI_Cmd_t           Cmd;
    STATAPI_CmdStatus_t     Status;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    U8                      *SrcBuf = NULL, *DstBuf = NULL,
                            *SrcBuf_Orig = NULL, *DstBuf_Orig = NULL;
    U32                     NumberRead, NumberWritten, i, DriveLoop;
    BOOL                    Passed;
    STATAPI_DriveType_t     Type;
    U32 					BuffSize;

#if defined (ATAPI_FDMA) || defined(STATAPI_CRYPT)
    SrcBuf = AllocateAligned(TINY_PARTITION_1, LBA48_SECTOR_COUNT * 512,
                             256, &SrcBuf_Orig);
#else
    SrcBuf = AllocateAligned(TEST_PARTITION_1, LBA48_SECTOR_COUNT * 512,
                             256, &SrcBuf_Orig);
#endif

    if (NULL == SrcBuf)
    {
        ATAPI_TestFailed(Result, "Couldn't allocate SrcBuf");
#if defined (ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1, SrcBuf_Orig);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1, SrcBuf_Orig);
#endif
        return Result;
    }
#if defined (ATAPI_FDMA) || defined(STATAPI_CRYPT)
    DstBuf = AllocateAligned(TINY_PARTITION_1, LBA48_SECTOR_COUNT * 512,
                             256, &DstBuf_Orig);
#else
    DstBuf = AllocateAligned(TEST_PARTITION_1, LBA48_SECTOR_COUNT * 512,
                             256, &DstBuf_Orig);
#endif

    if (NULL == DstBuf)
    {
        ATAPI_TestFailed(Result, "Couldn't allocate DstBuf");
#if defined (ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1, DstBuf_Orig);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1, DstBuf_Orig);
#endif
        return Result;
    }

#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType      = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType      = STATAPI_SATA;
#endif
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif
    AtapiInitParams.DriverPartition = TEST_PARTITION_1;
#if defined(ATAPI_FDMA)
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#endif
    AtapiInitParams.BaseAddress     = (U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress = (U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel  = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency  = ClockFrequency;

#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p = (STATAPI_RegisterMap_t *)PATARegMasks;    
#endif    
    strcpy(AtapiInitParams.EVTDeviceName, EVTDevName);

    /* STATAPI_Init() */
    STTBX_Print(("%d.0 STATAPI_Init()\n", TestParams->Ref));
    STTBX_Print(("Purpose: to ensure a ATAPI device can be initialized\n"));
    ErrorCode = STATAPI_Init(AtapiDevName, &AtapiInitParams);
    if (CheckCodeOk(ErrorCode))
    {
        ATAPI_TestFailed(Result,"Unable to initialize ATAPI device");
        goto api_end;
    }
    ATAPI_TestPassed(Result);

    for (DriveLoop = 0; DriveLoop < 1; DriveLoop++)
    {
        if (Drives[DriveLoop].Present)
        {
            /* STATAPI_Open() */
            if (DriveLoop == 0)
                AtapiOpenParams.DeviceAddress = STATAPI_DEVICE_0;
            else
                AtapiOpenParams.DeviceAddress = STATAPI_DEVICE_1;
            STTBX_Print(("%d.1 STATAPI_Open() \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure a handle can be opened\n"));
            ErrorCode = STATAPI_Open(AtapiDevName, &AtapiOpenParams,
                                     &Drives[DriveLoop].Handle);
            if (CheckCodeOk(ErrorCode))
            {
                ATAPI_TestFailed(Result,"Unable to open ATAPI device");
                goto api_end;
            }
            ATAPI_TestPassed(Result);
            STTBX_Print(("Setting PIO mode 4 (driver)\n"));
            ErrorCode = STATAPI_SetPioMode(Drives[DriveLoop].Handle,
                                           STATAPI_PIO_MODE_4);
            CheckCodeOk(ErrorCode);
            STTBX_Print(("Setting PIO mode 4 (drive)\n"));
            ErrorCode = SetTransferModePio(Drives[DriveLoop].Handle,
                                           STATAPI_PIO_MODE_4);
            CheckCodeOk(ErrorCode);
   			ErrorCode = STATAPI_GetDriveType ( Drives[DriveLoop].Handle, &Type );
		
			if ( ErrorCode != ST_NO_ERROR )
			{
		        STTBX_Print(("STATAPI_GetDriveType()\n"));
			}
	        if (Drives[DriveLoop].Type== STATAPI_ATAPI_DRIVE)
	        {
	          Cmd.CmdCode= STATAPI_CMD_IDENTIFY_PACKET_DEVICE;
	        }else
	        {
	          Cmd.CmdCode= STATAPI_CMD_IDENTIFY_DEVICE;
	        }
		
	        Cmd.SectorCount=0;
	        Cmd.Features=0x00;
	        Cmd.UseLBA= FALSE;
	        Cmd.Sector= 0x00;
	        Cmd.CylinderLow= 0x00;
	        Cmd.CylinderHigh= 0x00;
	        Cmd.Head= 0x00;
	        BuffSize= BLOCK_SIZE;
		
	        ErrorCode = STATAPI_CmdIn(Drives[DriveLoop].Handle, &Cmd, DstBuf,
	                              BuffSize, &NumberRead, &Status);
	        if (CheckCodeOk(ErrorCode))
	        {
	            ATAPI_TestFailed(Result, "Couldn't issue identify device command");
	            break;
	        }
	        STOS_SemaphoreWait(StepSemaphore_p);
	        if (CheckCodeOk(ErrorCode))
	        {
	             ATAPI_TestFailed(Result,"Couldn't receive the identify info ");
	             break;
	         }else
	         {
	              ATAPI_TestPassed(Result);
	         }
	
	        STTBX_Print(("------------------------------------------------------\n"));
	        ParseAndShow(&HardInfo,DstBuf);
	            /* Drop out early if the disk doesn't do it */
		    if (FALSE == LBA48_Capable)
		    {
		    	        /* STATAPI_Close() */
		        STTBX_Print(("%d.10 STATAPI_Close \n",TestParams->Ref));
		        STTBX_Print(("Purpose: to ensure we can close a connection\n"));
		        ErrorCode = STATAPI_Close(Drives[DriveLoop].Handle);
		        if (CheckCodeOk(ErrorCode))
		        {
		            ATAPI_TestFailed(Result,"Unable to close a connection");
		        }
		        ATAPI_TestPassed(Result);
		 	    AtapiTermParams.ForceTerminate= FALSE;
		
			    /* STATAPI_Term() */
			
			    STTBX_Print(("%d.11 STATAPI_Term \n",TestParams->Ref));
			    STTBX_Print(("Purpose: to ensure we can terminate the driver\n"));
			    ErrorCode = STATAPI_Term(AtapiDevName, &AtapiTermParams);
			    if (CheckCodeOk(ErrorCode))
			    {
			        ATAPI_TestFailed(Result, "Unable to terminate the driver");
			    }
			    else
			    {
			        ATAPI_TestPassed(Result);
			    }
		        STTBX_Print(("Disk is not LBA48-capable.\n"));
		        return Result;
			 }

            /* Fill buffers */
            memset(DstBuf, 0, sizeof(DstBuf));
            for (i = 0; i < LBA48_SECTOR_COUNT * 512; i++)
                SrcBuf[i] = (i % 256);

            /* Common to all commands */
            Cmd.SectorCount = LBA48_SECTOR_COUNT;
            Cmd.UseLBA = TRUE;
            if (MaxLBA48_2 >= Cmd.SectorCount)
            {
                Cmd.LBAExtended = (MaxLBA48_1 & 0xffff);
                Cmd.LBA = MaxLBA48_2 - Cmd.SectorCount;
            }
            else
            {
                Cmd.LBAExtended = (MaxLBA48_1 & 0xffff) - 1;
                Cmd.LBA = MaxLBA48_2 - Cmd.SectorCount;
            }
            STTBX_Print(("Using %i sectors at %u%u\n", Cmd.SectorCount,
                        Cmd.LBAExtended, Cmd.LBA));

            /* Clear the sectors on disk, to avoid misleading results */
            STTBX_Print(("%d.2 Clearing disk\n",TestParams->Ref));
            STTBX_Print(("Purpose: clearing sectors on disk\n"));

            Cmd.CmdCode = STATAPI_CMD_WRITE_SECTORS_EXT;
            ErrorCode = STATAPI_CmdOut(Drives[DriveLoop].Handle,
                                       &Cmd, DstBuf, LBA48_SECTOR_COUNT * 512,
                                       &NumberWritten, &Status);
            CheckCodeOk(ErrorCode);
            if (ST_NO_ERROR == ErrorCode)
            {
                STOS_SemaphoreWait(StepSemaphore_p);
                CheckCodeOk(EvtError);
            }
            if ((ErrorCode != ST_NO_ERROR) || (EvtError != ST_NO_ERROR))
            {
                ATAPI_TestFailed(Result, "Error clearing sectors");
            }
            else
            {
                ATAPI_TestPassed(Result);
            }

            STTBX_Print(("%d.3 STATAPI_CmdOut() \n",TestParams->Ref));
            STTBX_Print(("Purpose: check PIO extended write\n"));

            Cmd.CmdCode = STATAPI_CMD_WRITE_SECTORS_EXT;
            NumberWritten = (U32)-1;
            ErrorCode = STATAPI_CmdOut(Drives[DriveLoop].Handle,
                                       &Cmd,
                                       SrcBuf,
                                       LBA48_SECTOR_COUNT * 512,
                                       &NumberWritten,
                                       &Status);
            CheckCodeOk(ErrorCode);

            Passed = TRUE;
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Await STATAPI_CMD_COMPLETE*/
                STOS_SemaphoreWait(StepSemaphore_p);
                if (EvtError != ST_NO_ERROR)
                {
                    STTBX_Print(("Error carried by event: "));
                    DisplayError(EvtError);
                    STTBX_Print(("\n"));
                    Passed = FALSE;
                }
                else if (NumberWritten != (Cmd.SectorCount * 512))
                {
                    STTBX_Print(("Didn't write the right number of bytes (wrote %i, not %i)\n",
                                NumberWritten, Cmd.SectorCount * 512));
                    Passed = FALSE;
                }
            }
            else
            {
                STTBX_Print(("Error from STATAPI_CmdOut: "));
                DisplayErrorNew(ErrorCode);
                Passed = FALSE;
            }
            if (Passed == FALSE)
            {
                ATAPI_TestFailed(Result, "write failed");
            }
            else
            {
                ATAPI_TestPassed(Result);
            }
            STTBX_Print(("%d.4 STATAPI_CmdIn() \n",TestParams->Ref));
            STTBX_Print(("Purpose: check PIO extended read\n"));

            /* Now read sectors of data from LBA 0x0001ffffffff*/
            Cmd.CmdCode = STATAPI_CMD_READ_SECTORS_EXT;

            /* Issue command*/
            NumberRead = (U32)-1;
            STTBX_Print(("Purpose: check PIO extended read\n"));
            ErrorCode = STATAPI_CmdIn(Drives[DriveLoop].Handle,
                                      &Cmd,
                                      DstBuf,
                                      LBA48_SECTOR_COUNT * 512,
                                      &NumberRead,
                                      &Status);
            CheckCodeOk(ErrorCode);

            Passed = TRUE;
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Await STATAPI_CMD_COMPLETE*/
                STOS_SemaphoreWait(StepSemaphore_p);
                if (EvtError != ST_NO_ERROR)
                {
                    STTBX_Print(("Error carried by event: "));
                    DisplayError(EvtError);
                    STTBX_Print(("\n"));
                    Passed = FALSE;
                    break;
                }
                else if (NumberRead != (Cmd.SectorCount * 512))
                {
                    STTBX_Print(("Didn't read the right number of bytes (read %i, not %i)\n",
                                NumberRead, Cmd.SectorCount * 512));
                    Passed = FALSE;
                    break;
                }
            }
            else
            {
                STTBX_Print(("Error returned from STATAPI_CmdIn: "));
                DisplayErrorNew(ErrorCode);
                
                Passed = FALSE;
                break;
            }
            if (Passed == FALSE)
            {
                ATAPI_TestFailed(Result, "read failed");
            }
            else
            {
                ATAPI_TestPassed(Result);
            }
            STTBX_Print(("%d.5 Verifying buffer\n",TestParams->Ref));

            /* All apparently okay so far, check buffer contents*/
            Passed = TRUE;
            for (i = 0; i < (Cmd.SectorCount * 512); i++)
            {
                if (DstBuf[i] != SrcBuf[i])
                {
                    STTBX_Print(("Error on position %i - got %i, expected %i\n",
                                i, DstBuf[i], SrcBuf[i]));
                    Passed = FALSE;
                }
            }
            if (Passed == FALSE)
            {
                ATAPI_TestFailed(Result, "buffer mismatch");
            }
            else
            {
                ATAPI_TestPassed(Result);
            }

#if defined(DMA_PRESENT)
            /* Fill buffers, with different data to above */
            memset(DstBuf, 0, sizeof(DstBuf));
            for (i = 0; i < Cmd.SectorCount * 512; i++)
                SrcBuf[i] = (i % 128);

            /* Clear the sectors on disk, to avoid misleading results */
            STTBX_Print(("%d.6 Clearing disk\n",TestParams->Ref));
            STTBX_Print(("Purpose: clearing sectors on disk\n"));

            Cmd.CmdCode = STATAPI_CMD_WRITE_SECTORS_EXT;
            ErrorCode = STATAPI_CmdOut(Drives[DriveLoop].Handle,
                                       &Cmd, DstBuf, LBA48_SECTOR_COUNT * 512,
                                       &NumberWritten, &Status);
            CheckCodeOk(ErrorCode);
            if (ST_NO_ERROR == ErrorCode)
            {
                STOS_SemaphoreWait(StepSemaphore_p);
                CheckCodeOk(EvtError);
            }
            if ((ErrorCode != ST_NO_ERROR) || (EvtError != ST_NO_ERROR))
            {
                ATAPI_TestFailed(Result, "Error clearing sectors");
            }
            else
            {
                ATAPI_TestPassed(Result);
            }

            STTBX_Print(("Setting UDMA mode 4 (driver)\n"));
            ErrorCode = STATAPI_SetDmaMode(Drives[DriveLoop].Handle,
                                           STATAPI_DMA_UDMA_MODE_4);
            CheckCodeOk(ErrorCode);

            STTBX_Print(("Setting UDMA mode 4 (drive)\n"));
            ErrorCode = SetTransferModeDma(Drives[DriveLoop].Handle,
                                           STATAPI_DMA_UDMA_MODE_4);
            CheckCodeOk(ErrorCode);

            STTBX_Print(("%d.7 STATAPI_CmdOut() \n",TestParams->Ref));
            STTBX_Print(("Purpose: check DMA extended write\n"));

            /* Write sectors of data to LBA 0x0001ffffffff */
            Cmd.CmdCode = STATAPI_CMD_WRITE_DMA_EXT;

            /* Poison variables */
            NumberWritten = (U32)-1;
            ErrorCode = STATAPI_CmdOut(Drives[DriveLoop].Handle,
                                       &Cmd,
                                       SrcBuf,
                                       LBA48_SECTOR_COUNT * 512,
                                       &NumberWritten,
                                       &Status);
            CheckCodeOk(ErrorCode);

            Passed = TRUE;
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Await STATAPI_CMD_COMPLETE */
                STOS_SemaphoreWait(StepSemaphore_p);
                if (EvtError != ST_NO_ERROR)
                {
                    STTBX_Print(("Error carried by event: "));
                    DisplayError(EvtError);
                    STTBX_Print(("\n"));
                    Passed = FALSE;
                }
                else if (NumberWritten != (Cmd.SectorCount * 512))
                {
                    STTBX_Print(("Didn't write the right number of bytes (wrote %i, not %i)\n",
                                NumberWritten, Cmd.SectorCount * 512));
                    Passed = FALSE;
                }
            }
            else
            {
                STTBX_Print(("Error from STATAPI_CmdOut: "));
                DisplayErrorNew(ErrorCode);
                Passed = FALSE;
            }
            if (Passed == FALSE)
            {
                ATAPI_TestFailed(Result, "write failed");
            }
            else
            {
                ATAPI_TestPassed(Result);
            }

            STTBX_Print(("%d.8 STATAPI_CmdIn() \n",TestParams->Ref));
            STTBX_Print(("Purpose: check DMA extended read\n"));

            /* Now read sectors of data from LBA 0x0001ffffffff */
            Cmd.CmdCode = STATAPI_CMD_READ_DMA_EXT;

            /* Issue command */
            NumberRead = (U32)-1;
            ErrorCode = STATAPI_CmdIn(Drives[DriveLoop].Handle,
                                      &Cmd,
                                      DstBuf,
                                      LBA48_SECTOR_COUNT * 512,
                                      &NumberRead,
                                      &Status);
            CheckCodeOk(ErrorCode);

            Passed = TRUE;
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Await STATAPI_CMD_COMPLETE */
                STOS_SemaphoreWait(StepSemaphore_p);
                if (EvtError != ST_NO_ERROR)
                {
                    STTBX_Print(("Error carried by event: "));
                    DisplayError(EvtError);
                    STTBX_Print(("\n"));
                    Passed = FALSE;
                }
                else if (NumberRead != (Cmd.SectorCount * 512))
                {
                    STTBX_Print(("Didn't read the right number of bytes (read %i, not %i)\n",
                                NumberRead, Cmd.SectorCount * 512));
                    Passed = FALSE;
                }
            }
            else
            {
                STTBX_Print(("Error returned from STATAPI_CmdIn: "));
                DisplayErrorNew(ErrorCode);
                Passed = FALSE;
            }
            if (Passed == FALSE)
            {
                ATAPI_TestFailed(Result, "read failed");
            }
            else
            {
                ATAPI_TestPassed(Result);
            }

            STTBX_Print(("%d.9 Verifying buffer\n",TestParams->Ref));

            /* All apparently okay so far, check buffer contents */
            Passed = TRUE;
            for (i = 0; i < (Cmd.SectorCount * 512); i++)
            {
                if (DstBuf[i] != SrcBuf[i])
                {
                    STTBX_Print(("Error on buffer position %i - got %i, expected %i\n",
                                i, DstBuf[i], SrcBuf[i]));
                    Passed = FALSE;
                    break;
                }
            }
            if (Passed == FALSE)
            {
                ATAPI_TestFailed(Result, "buffer mismatch");
            }
            else
            {
                ATAPI_TestPassed(Result);
            }

            STTBX_Print(("Setting PIO mode 4 (driver)\n"));
            ErrorCode = STATAPI_SetPioMode(Drives[DriveLoop].Handle,
                                           STATAPI_PIO_MODE_4);
            CheckCodeOk(ErrorCode);
            STTBX_Print(("Setting PIO mode 4 (drive)\n"));
            ErrorCode = SetTransferModePio(Drives[DriveLoop].Handle,
                                           STATAPI_PIO_MODE_4);
            CheckCodeOk(ErrorCode);
#endif
        }

        /* STATAPI_Close() */
        STTBX_Print(("%d.10 STATAPI_Close \n",TestParams->Ref));
        STTBX_Print(("Purpose: to ensure we can close a connection\n"));
        ErrorCode = STATAPI_Close(Drives[DriveLoop].Handle);
        if (CheckCodeOk(ErrorCode))
        {
            ATAPI_TestFailed(Result,"Unable to close a connection");
        }
        ATAPI_TestPassed(Result);
    }

    AtapiTermParams.ForceTerminate= FALSE;

    /* STATAPI_Term() */

    STTBX_Print(("%d.11 STATAPI_Term \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure we can terminate the driver\n"));
    ErrorCode = STATAPI_Term(AtapiDevName, &AtapiTermParams);
    if (CheckCodeOk(ErrorCode))
    {
        ATAPI_TestFailed(Result, "Unable to terminate the driver");
    }
    else
    {
        ATAPI_TestPassed(Result);
    }

api_end:
#if defined (ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1, SrcBuf_Orig);
    STOS_MemoryDeallocate(TINY_PARTITION_1, DstBuf_Orig);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1, SrcBuf_Orig);
    STOS_MemoryDeallocate(TEST_PARTITION_1, DstBuf_Orig);

#endif

    return Result;
}

/************************************************************
Name: ATAPI_SMARTTest

Description:
    Test the SMART commands on the drive (assuming the disk
    supports them).

Returns:

Notes:
*************************************************************/
static ATAPI_TestResult_t ATAPI_SMARTTest(ATAPI_TestParams_t * TestParams)
{
    ATAPI_TestResult_t Result = TEST_RESULT_ZERO;
    STATAPI_InitParams_t    AtapiInitParams;
    STATAPI_TermParams_t    AtapiTermParams;
    STATAPI_OpenParams_t    AtapiOpenParams;
    STATAPI_Cmd_t           Cmd;
    STATAPI_CmdStatus_t     Status;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    U32                     NumberRead, i, DriveLoop;
#if !defined(ATAPI_FDMA) && !defined(STATAPI_CRYPT)
    U8                      Buf[512];
#else
    U8                      *Buf = NULL,*Temp_p = NULL;
    Temp_p = STOS_MemoryAllocate(TINY_PARTITION_1,BLOCK_SIZE+31);
    Buf = (U8 *)(((U32)(Temp_p) + 31) & ~31);
#endif

    /* Drop out early if the disk doesn't do it */
    if (FALSE == SMART_Capable)
    {
        STTBX_Print(("Disk does not support SMART commands.\n"));
        return Result;
    }

#if defined(DMA_PRESENT)
#if !defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_HDD_UDMA4;
#else
    AtapiInitParams.DeviceType = STATAPI_SATA;
#endif /* SATA_SUPPORTED */
#elif defined(SATA_SUPPORTED)
    AtapiInitParams.DeviceType = STATAPI_SATA;
#else
    AtapiInitParams.DeviceType = STATAPI_EMI_PIO4;
#endif

    AtapiInitParams.DriverPartition = TEST_PARTITION_1;
#if defined(ATAPI_FDMA)
    AtapiInitParams.NCachePartition = TINY_PARTITION_1;
#endif
    AtapiInitParams.BaseAddress     = (U32 *)ATA_BASE_ADDRESS;
    AtapiInitParams.HW_ResetAddress = (U16 *)ATA_HRD_RST;
    AtapiInitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    AtapiInitParams.InterruptLevel  = ATA_INTERRUPT_LEVEL;
    AtapiInitParams.ClockFrequency  = ClockFrequency;
#if defined(STATAPI_SET_EMIREGISTER_MAP)    
    AtapiInitParams.RegisterMap_p = (STATAPI_RegisterMap_t *)PATARegMasks;    
#endif    
    strcpy(AtapiInitParams.EVTDeviceName, EVTDevName);

    /* STATAPI_Init() */
    STTBX_Print(("%d.0 STATAPI_Init()\n", TestParams->Ref));
    STTBX_Print(("Purpose: to ensure a ATAPI device can be initialized\n"));
    ErrorCode = STATAPI_Init(AtapiDevName, &AtapiInitParams);
    if (CheckCodeOk(ErrorCode))
    {
        ATAPI_TestFailed(Result,"Unable to initialize ATAPI device");
        goto api_end;
    }
    ATAPI_TestPassed(Result);

    for (DriveLoop = 0; DriveLoop < 1; DriveLoop++)
    {
        if (Drives[DriveLoop].Present)
        {
            /* STATAPI_Open() */
            if (DriveLoop == 0)
                AtapiOpenParams.DeviceAddress = STATAPI_DEVICE_0;
            else
                AtapiOpenParams.DeviceAddress = STATAPI_DEVICE_1;
            STTBX_Print(("%d.1 STATAPI_Open() \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure a handle can be opened\n"));
            ErrorCode = STATAPI_Open(AtapiDevName, &AtapiOpenParams,
                                     &Drives[DriveLoop].Handle);
            if (CheckCodeOk(ErrorCode))
            {
                ATAPI_TestFailed(Result,"Unable to open ATAPI device");
                goto api_end;
            }
            ATAPI_TestPassed(Result);
            STTBX_Print(("Setting PIO mode 4 (driver)\n"));
            ErrorCode = STATAPI_SetPioMode(Drives[DriveLoop].Handle,
                                           STATAPI_PIO_MODE_4);
            CheckCodeOk(ErrorCode);
            STTBX_Print(("Setting PIO mode 4 (drive)\n"));
            ErrorCode = SetTransferModePio(Drives[DriveLoop].Handle,
                                           STATAPI_PIO_MODE_4);
            CheckCodeOk(ErrorCode);

            /* This signature value seems to be required for all SMART
             * commands. If it doesn't work, though, then check the spec
             * for the details of that particular command.
             */
            Cmd.UseLBA = TRUE;
            Cmd.LBA = 0x00c24f00;

            /* Enable SMART, if we need to */
            if (FALSE == SMART_Enabled)
            {
                Cmd.CmdCode = STATAPI_CMD_SMART_NODATA;
                Cmd.Features = 0xd8;
                ErrorCode = STATAPI_CmdNoData(Drives[DriveLoop].Handle,
                                              &Cmd, &Status);
                CheckCodeOk(ErrorCode);
                if (ST_NO_ERROR == ErrorCode)
                {
                    STOS_SemaphoreWait(StepSemaphore_p);
                    CheckCodeOk(EvtError);
                }
                if ((ErrorCode != ST_NO_ERROR) || (EvtError != ST_NO_ERROR))
                {
                    U8 Code;

                    ATAPI_TestFailed(Result, "Error enabling SMART");
                    STATAPI_GetExtendedError(Drives[DriveLoop].Handle, &Code);
                    STTBX_Print(("ExtError: 0x%02x\n", Code));
                    DumpCmdStatus(Status);
                }
                else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            /* Return status */
            STTBX_Print(("%d.2 STATAPI_CmdNoData() \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure SMART_NODATA works\n"));
            Cmd.CmdCode = STATAPI_CMD_SMART_NODATA;
            Cmd.Features = 0xda;
            ErrorCode = STATAPI_CmdNoData(Drives[DriveLoop].Handle,
                                          &Cmd, &Status);
            CheckCodeOk(ErrorCode);
            if (ST_NO_ERROR == ErrorCode)
            {
                STOS_SemaphoreWait(StepSemaphore_p);
                CheckCodeOk(EvtError);
            }
            if ((ErrorCode != ST_NO_ERROR) || (EvtError != ST_NO_ERROR))
            {
                U8 Code;

                ATAPI_TestFailed(Result, "Error getting SMART status");
                STATAPI_GetExtendedError(Drives[DriveLoop].Handle, &Code);
                STTBX_Print(("ExtError: 0x%02x\n", Code));
                DumpCmdStatus(Status);
            }
            else
            {

                STTBX_Print(("LBA: 0x%08x\n", Status.LBA));
                /*LBA low value is NA*/
                Status.LBA = Status.LBA & (0x00FFFF00);
                if ((Status.LBA != 0x00c24f00) && (Status.LBA != 0x002cf400))
                {
                    STTBX_Print(("LBA: 0x%08x\n", Status.LBA));
                    ATAPI_TestFailed(Result, "Invalid status values.\n");
                }
                else
                {
                    ATAPI_TestPassed(Result);
                }
            }

            STTBX_Print(("%d.3 STATAPI_CmdIn() \n",TestParams->Ref));
            STTBX_Print(("Purpose: to ensure SMART_IN works\n"));
            /* Read data (?) */
            Cmd.CmdCode = STATAPI_CMD_SMART_IN;
            Cmd.Features = 0xd0;
            ErrorCode = STATAPI_CmdIn(Drives[DriveLoop].Handle, &Cmd,
                                      Buf, 512, &NumberRead, &Status);
            CheckCodeOk(ErrorCode);
            if (ST_NO_ERROR == ErrorCode)
            {
                STOS_SemaphoreWait(StepSemaphore_p);
                CheckCodeOk(EvtError);
            }
            if ((ErrorCode != ST_NO_ERROR) || (EvtError != ST_NO_ERROR))
            {
                U8 Code;

                ATAPI_TestFailed(Result, "Error doing SMART read");
                STATAPI_GetExtendedError(Drives[DriveLoop].Handle, &Code);
                STTBX_Print(("ExtError: 0x%02x\n", Code));
                DumpCmdStatus(Status);
            }
            else
            {
                U8 sum = 0;

                for (i = 0; i < 512; i++)
                    sum += Buf[i];

                if (0 == sum)
                {
                    ATAPI_TestPassed(Result);
                }
                else
                {
                    ATAPI_TestFailed(Result, "Incorrect checksum on SMART data");
                }
            }

            /* Restore to original state, if required */
            if (FALSE == SMART_Enabled)
            {
                Cmd.CmdCode = STATAPI_CMD_SMART_NODATA;
                Cmd.Features = 0xd9;
                ErrorCode = STATAPI_CmdNoData(Drives[DriveLoop].Handle,
                                              &Cmd, &Status);
                CheckCodeOk(ErrorCode);
                if (ST_NO_ERROR == ErrorCode)
                {
                    STOS_SemaphoreWait(StepSemaphore_p);
                    CheckCodeOk(EvtError);
                }
                if ((ErrorCode != ST_NO_ERROR) || (EvtError != ST_NO_ERROR))
                {
                    U8 Code;

                    ATAPI_TestFailed(Result, "Error disabling SMART");
                    STATAPI_GetExtendedError(Drives[DriveLoop].Handle, &Code);
                    STTBX_Print(("ExtError: 0x%02x\n", Code));
                    DumpCmdStatus(Status);
                }
                else
                {
                    ATAPI_TestPassed(Result);
                }
            }
        }

#if defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
#endif
        /* STATAPI_Close() */
        STTBX_Print(("%d.10 STATAPI_Close \n",TestParams->Ref));
        STTBX_Print(("Purpose: to ensure we can close a connection\n"));
        ErrorCode = STATAPI_Close(Drives[DriveLoop].Handle);
        if (CheckCodeOk(ErrorCode))
        {
            ATAPI_TestFailed(Result,"Unable to close a connection");
        }
        ATAPI_TestPassed(Result);
    }

    AtapiTermParams.ForceTerminate= FALSE;

    /* STATAPI_Term() */

    STTBX_Print(("%d.11 STATAPI_Term \n",TestParams->Ref));
    STTBX_Print(("Purpose: to ensure we can terminate the driver\n"));
    ErrorCode = STATAPI_Term(AtapiDevName, &AtapiTermParams);
    if (CheckCodeOk(ErrorCode))
    {
        ATAPI_TestFailed(Result, "Unable to terminate the driver");
    }
    else
    {
        ATAPI_TestPassed(Result);
    }

api_end:

    return Result;
}

/************************************************************
Name: DoSetGetTimingTests

Description:
    Given a drive handle, attempts to check that the API deals with set/get
    timing parameters properly (PIO only at present). This is done by taking
    the parameters for pio mode 4 and udma mode 4, changing them, setting,
    switching between them, etc. The code for this looks more involved
    than it actually is, honest. :-) (And the error checking *really* makes
    it longer)

Returns:
    ST_NO_ERROR on success
    Anything else is failure.

Notes:
    Assumes that PIO mode 4 and UDMA mode 4 are valid (for setting timing)
    Assuming all calls successful, should leave the handle with same
        timing params as when called, and in PIO mode 4 (if some
        calls fail, handle is in undefined state and should be closed)
*************************************************************/
ST_ErrorCode_t  DoSetGetTimingTests(STATAPI_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR, Overall = ST_NO_ERROR;
    STATAPI_PioTiming_t PioTimingOld, PioTimingNew, PioTimingTemp;
    STATAPI_DmaTiming_t DmaTimingOld, DmaTimingNew, DmaTimingTemp;
    STATAPI_PioMode_t PioMode;
    STATAPI_DmaMode_t DmaMode;

    /* Start off in UDMA mode 4 */
    error = STATAPI_SetDmaMode(Handle, STATAPI_DMA_UDMA_MODE_4);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    /* Get timing */
    error = STATAPI_GetDmaMode(Handle, &DmaMode, &DmaTimingOld);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    DmaTimingNew = DmaTimingOld;

    /* Change timing copy */
    DmaTimingNew.DmaTimingParams.UltraDmaTimingParams.AckT += 1;
    DmaTimingNew.DmaTimingParams.UltraDmaTimingParams.DataOutSetupT += 2;
    DmaTimingNew.DmaTimingParams.UltraDmaTimingParams.MinInterlockT += 3;

    error = STATAPI_SetDmaTiming(Handle, &DmaTimingNew);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    /* Switch to PIO mode 4 */
    error = STATAPI_SetPioMode(Handle, STATAPI_PIO_MODE_4);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    /* Get timing */
    error = STATAPI_GetPioMode(Handle, &PioMode, &PioTimingOld);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    PioTimingNew = PioTimingOld;
    /* Change the timing copy */
    PioTimingNew.InitSequenceDelay += 1;
    PioTimingNew.AddressHoldDelay += 2;
    PioTimingNew.ReadRecoverDelay += 3;

    error = STATAPI_SetPioTiming(Handle, &PioTimingNew);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    /* Switch back to UDMA 4 */
    error = STATAPI_SetDmaMode(Handle, STATAPI_DMA_UDMA_MODE_3);
    error = STATAPI_SetDmaMode(Handle, STATAPI_DMA_UDMA_MODE_4);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    /* Get and check timing */
    error = STATAPI_GetDmaMode(Handle, &DmaMode, &DmaTimingTemp);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }
    if (memcmp(&DmaTimingTemp, &DmaTimingNew, sizeof(STATAPI_DmaTiming_t)))
    {
        STTBX_Print(("Error, DMA timing didn't stick\n"));
        Overall = ST_ERROR_INVALID_HANDLE;
    }

    /* Restore old timing */
    error = STATAPI_SetDmaTiming(Handle, &DmaTimingOld);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    /* Switch back to PIO mode 4 */
    error = STATAPI_SetPioMode(Handle, STATAPI_PIO_MODE_3);
    error = STATAPI_SetPioMode(Handle, STATAPI_PIO_MODE_4);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    /* Get and check timing */
    error = STATAPI_GetPioMode(Handle, &PioMode, &PioTimingTemp);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }
    if (memcmp(&PioTimingTemp, &PioTimingNew, sizeof(STATAPI_PioTiming_t)))
    {
        STTBX_Print(("Error - PIO timing didn't stick\n"));
        Overall = ST_ERROR_INVALID_HANDLE;
    }

    /* Restore old timing */
    error = STATAPI_SetPioTiming(Handle, &PioTimingOld);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Failed - expected ST_NO_ERROR, got "));
        DisplayErrorNew(error);
        return error;
    }

    return Overall;
}

/********************************************************
    Funtion: ReadTOCTest

    Description:
    The READ TOC/PMA/ATIP Command requests that the CD Logical Unit transfer
    data from the Table of Contents, the Program Memory Area (PMA), or the
    Absolute Time in Pre-Grove (ATIP) from CD media.

    For DVD media, as there is no TOC, this command will return fabricated
    information that is similar to that of CD media for some formats

**********************************************************/
ST_ErrorCode_t  ReadTOCTest(STATAPI_Handle_t Handle)
{
    ST_ErrorCode_t Error;
    U8 *Data_p;
    U8 *SenseData_p;
    U32 rc,ndx;
    U32 repeat=0;
    U32 NumberRW=0;
    STATAPI_Packet_t       Pkt;
    STATAPI_PktStatus_t PktStatus;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *SenseTemp_p,*Temp_p;
    Temp_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,4*K + 31);
    Data_p =(U8 *)(((U32)(Temp_p) + 31) & ~31);
    SenseTemp_p=(U8*)STOS_MemoryAllocate(TINY_PARTITION_1,4*K);
    SenseData_p = (U8 *)(((U32)(SenseTemp_p) + 31) & ~31);
#else
    Data_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,4*K);
    SenseData_p=(U8*)STOS_MemoryAllocate(TEST_PARTITION_1,4*K);
#endif

      do
      {
	       memset( Pkt.Data, 0, 15 );
	       Pkt.OpCode= STATAPI_PKT_READ_TOC_PMA_ATIP;
	       Pkt.Data[0]=0x02;
	       Pkt.Data[1]=0x00; /*format field*/
	       Pkt.Data[6]=0x10;
	       Pkt.Data[7]=0x00;
	       Pkt.Data[8]=0x00;
	
	       Error = STATAPI_PacketIn(Handle, &Pkt,Data_p,
	                                4096,&NumberRW,&PktStatus);
	
           if (Error != ST_NO_ERROR)
           {
            	 STTBX_Print((" Read  TOC failed \n"));
            	 break;
           }
	
	       STOS_SemaphoreWait(StepSemaphore_p);
	       if (EvtError == ST_NO_ERROR)
	       {
	           STTBX_Print(("*** Number of retries: %d \n",repeat));
	           STTBX_Print(("### Bytes written: %d \n",NumberRW));
	           STTBX_Print(( "First Session=%02X, Last Session=%02X\n",
	                         Data_p[2],Data_p[3]));
	            STTBX_Print(( "TOC entries (11 bytes each)... \n" ));
	
	
	            rc = ( ( Data_p[0] * 256 ) + Data_p[1] - 2 ) / 11;
	            ndx = 4;
	            while ( rc > 0 )
		        {
		                STTBX_Print(( " %02X %02X %02X "
				                        "%02X %02x %02X "
				                        "%02X %02X %02X "
				                        "%02X %02x \n",
				                         Data_p[ndx+0], Data_p[ndx+1], Data_p[ndx+2],
				                         Data_p[ndx+3], Data_p[ndx+4], Data_p[ndx+5],
				                         Data_p[ndx+6], Data_p[ndx+7], Data_p[ndx+8],
				                         Data_p[ndx+9], Data_p[ndx+10], Data_p[ndx+11]
		                   			));
			             rc -- ;
			             ndx = ndx + 11;
	             }
	            Error =ST_NO_ERROR;
	            break;
	       }
	       else
	       {
		         memset( Pkt.Data, 0, 15 );
		         Pkt.OpCode= STATAPI_PKT_REQUEST_SENSE;
		         Pkt.Data[3]=0x20,
		
		         Error = STATAPI_PacketIn(Handle, &Pkt,SenseData_p,
		                                4096,&NumberRW,&PktStatus);
		         if (Error != ST_NO_ERROR)
		         {
		                STTBX_Print((" Request sense key  Failed \n"));
		                break;
		         }
		
		         STOS_SemaphoreWait(StepSemaphore_p);
		         STTBX_Print((" Sense data= %x  ASC= %x\r",SenseData_p[2],SenseData_p[12]));
		         if (SenseData_p[2]&0x0F)   repeat++;
		         if (repeat > 30)
		         {
		                STTBX_Print((" Read capacity failed, too many tries: %d \n",repeat));
		                Error =TRUE;
		                break;
		         }
	        }
	        task_delay ( 1000000 );
      }while(TRUE);


#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    STOS_MemoryDeallocate(TINY_PARTITION_1,Temp_p);
    STOS_MemoryDeallocate(TINY_PARTITION_1,SenseTemp_p);
#else
    STOS_MemoryDeallocate(TEST_PARTITION_1,Data_p);
    STOS_MemoryDeallocate(TEST_PARTITION_1,SenseData_p);
#endif

    return Error;

}
/************************************************************
Name: RequestSense

Description:
    The REQUEST SENSE Command requests that the Logical Unit transfer
    sense data to the Host.
Returns:
    ST_NO_ERROR on success
    Anything else is failure.

Notes:
   Logical Units shall be capable of returning at least 18 bytes of data
   in response to a REQUEST SENSE Command. If the Allocation Length is 18 or
   greater, and a Logical Unit returns less than 18 bytes of data, the Host
   should assume that the bytes not transferred would have been zeros had
   the Logical Unit returned those bytes
*************************************************************/
static ST_ErrorCode_t RequestSense ( STATAPI_Handle_t hATAPIDevHandle )
{
	STATAPI_Packet_t    Pkt;
	STATAPI_PktStatus_t PktStatus;
    ST_ErrorCode_t      error;
	U32	ui32NumberRead;
	U8	Buffer[4096];

	memset(Pkt.Data,0,15);

	Pkt.OpCode 	= STATAPI_PKT_REQUEST_SENSE;
	Pkt.Data[3]	= REQ_SENSE_DATA_LENGTH;

	error = STATAPI_PacketIn (hATAPIDevHandle,&Pkt,Buffer,REQ_SENSE_DATA_LENGTH,
	                                                    &ui32NumberRead, &PktStatus );

	if (error != ST_NO_ERROR)
	{
	  STTBX_Print(("STATAPI_PacketIn() - "));
	  DisplayErrorNew(error);
	}

	STOS_SemaphoreWait(StepSemaphore_p);

	if (EvtError != ST_NO_ERROR)
	{
	   STTBX_Print(("Req sense Error %d\n", error));
		DisplayErrorNew(EvtError);
	}
	else
	{
		STTBX_Print (( "Request sense Success\n" ));
		STTBX_Print (( "Sense key =%x\n", Buffer[SENSE_KEY_OFFSET]));
		STTBX_Print (( "ASC = %x\n", Buffer[ASC_OFFSET]));
		STTBX_Print (( "ASCQ = %x\n", Buffer[ASCQ_OFFSET]));
	}

	return error;

}
/************************************************************
Name: ControlTray

Description:
   The START/STOP UNIT Command requests that the Logical Unit enable or disable
   media access operations.

Returns:
    ST_NO_ERROR on success
    Anything else is failure.
Notes:
   An immediate (Immed) bit of one indicates that status shall be returned
   as soon as the Command Packet has been validated. An Immed bit of zero
   indicates that status shall be returned after the operation is completed.
*************************************************************/
static ST_ErrorCode_t ControlTray ( STATAPI_Handle_t hATAPIDevHandle, U8 TrayOpen )
{
	U32	                error;
	STATAPI_Packet_t    Pkt;
	STATAPI_PktStatus_t PktStatus;

	Pkt.OpCode= STATAPI_PKT_START_STOP_UNIT;
	memset(Pkt.Data,0,15);
    Pkt.Data[0]=1;     /*IMMMED BIt set */
	if ( TrayOpen )
	{
		Pkt.Data[3]=2; /*Eject the Disc*/
	}
	else
	{
		Pkt.Data[3]=3; /*Load the Disc*/
	}
	error = STATAPI_PacketNoData(hATAPIDevHandle,&Pkt,&PktStatus);

	if (error != ST_NO_ERROR)
	{
	  STTBX_Print(("STATAPI_CmdOut() - "));
	  DisplayErrorNew(error);
	}

	STOS_SemaphoreWait(StepSemaphore_p);

	if (EvtError != ST_NO_ERROR)
	{
	    STTBX_Print(("Tray Eject and Disc Load %d\n", error));
	    return TRUE;
	}
	return FALSE;

}
static void ATAPI_Callback( STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p )
{
    STATAPI_EvtParams_t *EvtData;

    EvtData = (STATAPI_EvtParams_t *)EventData;

    if (!Quiet)
    {
        switch ( Event )
        {
            case STATAPI_HARD_RESET_EVT:
                STTBX_Print( ("STATAPI_HARD_RESET_EVT event received\n") );
                break;

            case STATAPI_SOFT_RESET_EVT:
                STTBX_Print( ("STATAPI_SOFT_RESET_EVT event received\n") );
                break;

            case STATAPI_CMD_COMPLETE_EVT:
                STTBX_Print( ("STATAPI_CMD_COMPLETE_EVT event received\n") );
                break;

            case STATAPI_PKT_COMPLETE_EVT:
                STTBX_Print( ("STATAPI_PKT_COMPLETE_EVT event received\n") );
                break;
        }

        STTBX_Print( ("Device %d  ===>",EvtData->DeviceAddress) );
        STTBX_Print( ("Error code : ") );
        DisplayError(EvtData->Error);
        STTBX_Print( ("\n") );
    }
    EvtError = EvtData->Error;
    STOS_SemaphoreSignal(StepSemaphore_p);
}

/*SATA callback*/

#if defined(TEST_TWO_DEVICE)
static void ATAPI_Callback2( STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t SATAEvent,
                            const void *SATAEventData,
                            const void *SATASubscriberData_p )
{
    STATAPI_EvtParams_t *EvtData;

    EvtData = (STATAPI_EvtParams_t *)SATAEventData;

    if (!Quiet)
    {
        switch ( SATAEvent )
        {
            case STATAPI_HARD_RESET_EVT:
                STTBX_Print( ("STATAPI_HARD_RESET_EVT event received\n") );
                break;

            case STATAPI_SOFT_RESET_EVT:
                STTBX_Print( ("STATAPI_SOFT_RESET_EVT event received\n") );
                break;

            case STATAPI_CMD_COMPLETE_EVT:
                STTBX_Print( ("STATAPI_CMD_COMPLETE_EVT event received\n") );
                break;

            case STATAPI_PKT_COMPLETE_EVT:
                STTBX_Print( ("STATAPI_PKT_COMPLETE_EVT event received\n") );
                break;
        }

        STTBX_Print( ("Device %d  ===>",EvtData->DeviceAddress) );
        STTBX_Print( ("Error code : ") );
        DisplayError(EvtData->Error);
        STTBX_Print( ("\n") );
    }
    EvtError = EvtData->Error;
    STOS_SemaphoreSignal(StepSemaphore2_p);
}
#endif/*SATA_SUPPORTED*/

static U8 *AllocateAligned(partition_t *partition, U32 Size,
                           U32 Alignment, U8 **OriginalPtr)
{
    U8 *NewPtr, *tmp;
    U32 Diff;

    /* Allocate the memory, making sure space for alignment */
    tmp = STOS_MemoryAllocate(partition, Size + Alignment);
    /* Before alignment... */
    *OriginalPtr = tmp;
    if (tmp != NULL)
    {
        Diff = ((U32)tmp / Alignment) * Alignment;
        if (Diff != (U32)tmp)
            tmp = (U8 *)((((U32)tmp / Alignment) + 1) * Alignment);
    }
    /* ... and afterwards */
    NewPtr = tmp;

    return NewPtr;
}

#ifdef SOAK_TEST
/****************************************************************************
VerifyBuffer()
Description:
    Given two buffers (and a count), check that the contents are identical.
    If a byte differs, print out a warning, 16-byte dump surrounding the
    area.

Parameters:
    Src, Dst    - U8 buffers to compare
    Count       - how many bytes to compare

Return Value:
    TRUE        - buffers were identical
    FALSE       - buffers differed

See Also:
    PCP_NormalUse()
    PCP_ErrantUse()
*****************************************************************************/
static BOOL VerifyBuffer(U8 *Src, U8 *Dst, U32 Count)
{
    U32 Loop, i, start;

    for (Loop = 0; Loop < Count; Loop++)
    {
        if (Src[Loop] != Dst[Loop])
        {
            STTBX_Print(("Warning! Corrupted buffer (position 0x%02x)\n",
                            Loop));

            /* Print out 16 bytes, from the last 16-byte boundary */
            start = (Loop / 16) * 16;
            STTBX_Print(("Received:\n"));
            STTBX_Print(("Offset 0x%02x: ", start));
            for (i = start; (i < Count) && (i < (start + 16)); i++)
            {
                STTBX_Print(("%02x ", Dst[i]));
            }
            STTBX_Print(("\n"));

            STTBX_Print(("Expected:\n"));
            STTBX_Print(("Offset 0x%02x: ", start));
            for (i = start; (i < Count) && (i < (start + 16)); i++)
            {
                STTBX_Print(("%02x ", Src[i]));
            }
            STTBX_Print(("\n"));

            /* Abort the main loop */
            break;
        }
    }

    return (Loop == Count)?TRUE:FALSE;
}
#endif

static ST_ErrorCode_t TestUnitReady ( STATAPI_Handle_t hATAPIDevHandle )
{
	STATAPI_Packet_t Pkt;
	STATAPI_PktStatus_t PktStatus;
    ST_ErrorCode_t  error;

	memset(Pkt.Data,0,15);
	Pkt.OpCode = STATAPI_PKT_TEST_UNIT_READY;

	error = STATAPI_PacketNoData ( hATAPIDevHandle, &Pkt, &PktStatus );

	if (error != ST_NO_ERROR)
	{
	  STTBX_Print(("STATAPI_PacketNoData() - "));
	  DisplayErrorNew(error);
	}

	STOS_SemaphoreWait(StepSemaphore_p);

	if (EvtError != ST_NO_ERROR)
	{
	     STTBX_Print(("Waiting for Test Unit to become ready ...\n"));
   		 error = ST_ERROR_DEVICE_BUSY;
	}
	else
	{
		ATAPI_DEBUG_PRINT ( ( "Test Unit Ready" ) );
		error = ST_NO_ERROR;
	}

	return error;

}
static ST_ErrorCode_t SimpleRTestDVD(STATAPI_Handle_t hATAPIDevHandle,U32 StartSector, U32 NoOfSectors )
{
	STATAPI_Packet_t Pkt;
	STATAPI_PktStatus_t PktStatus;
    ST_ErrorCode_t  error;
	U8 	*read_buffer_p = NULL;
	U8  *read_buffer_base = NULL;
   	U32	 NumberRead;    
   	U32  NumberToRead =  NoOfSectors * DVD_SECTOR_SIZE + 128;
  	U32	 RetryCount = 0;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Tempbuffer_p;
    Tempbuffer_p = (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,NumberToRead + 31);
    read_buffer_base = (U8 *)(((U32)(Tempbuffer_p) + 31) & ~31);
#else
    read_buffer_base = (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,NumberToRead);
#endif

		if (read_buffer_base == (U8 *) NULL)
		{
		  STTBX_Print(("Failed to allocate memory\n"));
		  return TRUE;
		}

		read_buffer_p = (U8*)((((U32)read_buffer_base / 128)*128)+128);
		NumberToRead -= 128;

		memset(read_buffer_p, '\0', NumberToRead);
		memset(Pkt.Data,0,15);

		Pkt.OpCode 						    = STATAPI_PKT_READ_12;
		Pkt.Data [LSN_ADDRESS_OFFSET] 		= (U8)(StartSector >> 24 ) & 0xFF;
		Pkt.Data [LSN_ADDRESS_OFFSET + 1]   = (U8)(StartSector >> 16 ) & 0xFF;
		Pkt.Data [LSN_ADDRESS_OFFSET + 2]   = (U8)(StartSector >> 8 ) & 0xFF;
		Pkt.Data [LSN_ADDRESS_OFFSET + 3]   = (U8)(StartSector & 0xFF);

		Pkt.Data [ATAPI_XFER_LENGTH_OFFSET]	    = (U8)((NoOfSectors >> 24 ) & 0xFF);
		Pkt.Data [ATAPI_XFER_LENGTH_OFFSET + 1]	= (U8)((NoOfSectors >> 16)  & 0xFF);
	    Pkt.Data [ATAPI_XFER_LENGTH_OFFSET + 2]	= (U8)((NoOfSectors >> 8 )  & 0xFF);
		Pkt.Data [ATAPI_XFER_LENGTH_OFFSET + 3]	= (U8)((NoOfSectors) & 0xFF);

		do
	    {
		     if ( TestUnitReady ( hATAPIDevHandle) == ST_NO_ERROR )
		     {
            		error = STATAPI_PacketIn (
            								hATAPIDevHandle,
            								&Pkt,
            								read_buffer_p,
            								NumberToRead,
            								&NumberRead,
            								&PktStatus );

            		if (error != ST_NO_ERROR)
            		{
            		  STTBX_Print(("STATAPI_PacketIn() - "));
            		  DisplayErrorNew(error);
            		}

            		STOS_SemaphoreWait(StepSemaphore_p);

            		if (EvtError != ST_NO_ERROR)
            		{
            		   STTBX_Print(("DVD Read %d\n", error));
            			DisplayErrorNew(EvtError);
            		}
            		else
            		{

            				STTBX_Print ( ( "DVD Read Success:" ) );
            				STTBX_Print((" Read %d Sectors from DVD\n",NoOfSectors));
            				error = ST_NO_ERROR;
            				break;
            		}
	        }
        	else
        	{
        			++RetryCount;
        			STTBX_Print ( ( "Unit Not Ready" ) );
        			error = ST_ERROR_TIMEOUT;
        	}
          }while ( RetryCount < 30 );

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Tempbuffer_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,read_buffer_base);
#endif

	return error;

}

static ST_ErrorCode_t  SimpleRTestCD(STATAPI_Handle_t hATAPIDevHandle, U32 StartSector, U32 NoOfSectors )
{
	STATAPI_Packet_t Pkt;
	STATAPI_PktStatus_t PktStatus;
    ST_ErrorCode_t  error;
	U8 	*read_buffer_p,*read_buffer_base;
   	U32	 NumberRead;
	U32	 RetryCount = 0;

    U32  NumberToRead = NoOfSectors * CD_SECTOR_SIZE + 128;

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
    U8 *Tempbuffer_p = NULL;
    Tempbuffer_p = (U8*)STOS_MemoryAllocate(TINY_PARTITION_1,NumberToRead + 31);
    read_buffer_base = (U8 *)(((U32)(Tempbuffer_p) + 31) & ~31);
#else
    read_buffer_base = (U8*)STOS_MemoryAllocate(TEST_PARTITION_1,NumberToRead);
#endif
		if (read_buffer_base == (U8 *) NULL)
		{
		  STTBX_Print(("Failed to allocate memory\n"));
		  return TRUE;
		}

		read_buffer_p = (U8*)((((U32)read_buffer_base / 128)*128)+ 128);
		NumberToRead -= 128;

		memset(read_buffer_p, '\0', NumberToRead);
		memset(Pkt.Data,0,15);

		StartSector -= 150;

		Pkt.OpCode 								= STATAPI_PKT_READ_CD;
		Pkt.Data [ LSN_ADDRESS_OFFSET ] 		= (U8)( StartSector >> 24 ) & 0xFF;
		Pkt.Data [ LSN_ADDRESS_OFFSET + 1 ]     = (U8)( StartSector >> 16 ) & 0xFF;
		Pkt.Data [ LSN_ADDRESS_OFFSET + 2 ]     = (U8)( StartSector >> 8 ) & 0xFF;
		Pkt.Data [ LSN_ADDRESS_OFFSET + 3 ]     = (U8)( StartSector & 0xFF );

	    Pkt.Data [ATAPI_XFER_LENGTH_OFFSET ]	= (U8)((NoOfSectors >> 16)  & 0xFF);
	    Pkt.Data [ATAPI_XFER_LENGTH_OFFSET + 1]	= (U8)((NoOfSectors >> 8 )  & 0xFF);
		Pkt.Data [ATAPI_XFER_LENGTH_OFFSET + 2]	= (U8)((NoOfSectors) & 0xFF);


	do
	{
		if ( TestUnitReady (hATAPIDevHandle ) == ST_NO_ERROR )
		{
		error = STATAPI_PacketIn (
								hATAPIDevHandle,
								&Pkt,
								read_buffer_p,
								NumberToRead,
								&NumberRead,
								&PktStatus );

		if (error != ST_NO_ERROR)
		{
		  STTBX_Print(("STATAPI_PacketIn() - "));
		  DisplayErrorNew(error);
		}

		STOS_SemaphoreWait(StepSemaphore_p);

		if (EvtError != ST_NO_ERROR)
		{
		   STTBX_Print(("CD Read %d\n", error));
			DisplayErrorNew(EvtError);
		}
		else
		{
    			STTBX_Print(( "CD Read Success" ));
    			STTBX_Print((" Read %d Sectors from CD\n",NoOfSectors));
    			error = ST_NO_ERROR;
    			break;
			}
		}
		else
		{
			++RetryCount;
			STTBX_Print (( "Unit Not Ready, Enter again" ));
			error = ST_ERROR_TIMEOUT;
		}
	}while ( RetryCount < 30 );

#if defined(ATAPI_BMDMA) || defined(ATAPI_FDMA) || defined(STATAPI_CRYPT)
        STOS_MemoryDeallocate(TINY_PARTITION_1,Tempbuffer_p);
#else
        STOS_MemoryDeallocate(TEST_PARTITION_1,read_buffer_base);
#endif
	return error;
}


