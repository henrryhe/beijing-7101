/******************************************************************************

File Name   : local

Description : Definitions local to STFDMA but used by severl modules within
              STFDMA.

******************************************************************************/

#ifndef  LOCAL_H
#define  LOCAL_H

/* Includes ------------------------------------------------------------ */

#ifdef STFDMA_EMU                       /* For test purposes only */
#define STTBX_Print(x)  printf x
#else
#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <asm/semaphore.h>
#include <asm/io.h>
#else
#include <string.h>                     /* C libs */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "sttbx.h"
#endif
#endif

#if defined(ST_498)
#include "stb0498.h"
#endif
#if defined(STFDMA_MPX_SUPPORT) && !defined(STFDMA_MPX_USE_EXT_IRQ3)
#include "stpio.h"
#endif
#include "stddefs.h"                     /* Standard includes */
#include "stdevice.h"
#include "stfdma.h"
#include "stos.h"
#include "stsys.h"

/* Exported Constants -------------------------------------------------- */

/* Number max dreq can be present at a given time */
#define MAX_DREQ               100

#if defined (ST_498) || defined (STFDMA_MPX_SUPPORT)
/*From MPX address mapping(498 shared LMI visible as this address on 7109) - should be passed at compile time*/
/*#define STFDMA_MPX_ESTB_BASE_VADDR		0xA3800000 */	

/*From MPX address mapping(498 shared LMI physical address) - should be passed at compile time*/
/*#define STFDMA_MPX_ECM_BASE_PADDR		0x0E000000*/ 	

/*From MPX address mapping(7109 shared LMI visible as this address on 498) - should be passed at compile time*/																															
/*#define STFDMA_MPX_ECM_BASE_VADDR		0xA0800000*/

/*From MPX address mapping(7109 shared LMI physical address) - should be passed at compile time*/
/*#define STFDMA_MPX_ESTB_BASE_PADDR		0x07000000*/	

/*Interrupt status stored in 498's shared memory(CPU visible from 498 side)  - should be passed at compile time*/																															
/*#define STFDMA_MPX_ECM_SHARED_INT_STATUS		0xAE300000*/ 	

/*This macro converts an 498_shared_LMI "virtual" address into 7109 "virtual" address*/
#define GET_MPX_ECM_SHARED_VADDRESS(eCMpaddress)	((U32)STFDMA_MPX_ESTB_BASE_VADDR + ((U32)eCMpaddress - (U32)STFDMA_MPX_ECM_BASE_PADDR))
/*This macro converts MPX shared "physical adress" on 7109 as seen by 498's FDMA  to 7109 virtual address*/
#define GET_MPX_ESTB_SHARED_VADDRESS(eCMeSTBpaddress) 	(((U32)STFDMA_MPX_ESTB_BASE_PADDR | 0xA0000000)  +  ((U32)eCMeSTBpaddress -  (U32)((U32)STFDMA_MPX_ECM_BASE_VADDR & 0x1FFFFFFF)))
/*Shared interrupt status visible from 7109*/
#define STFDMA_MPX_ESTB_SHARED_INT_STATUS	GET_MPX_ECM_SHARED_VADDRESS(((U32)STFDMA_MPX_ECM_SHARED_INT_STATUS & (U32)0x0FFFFFFF))
/*PIO used on ECM to signal FDMA interrupt to eSTB*/
#define MPX_ECM_PIOx_BITy   					STFDMA_MPX_ECM_PIO_PORT, STFDMA_MPX_ECM_PIO_BIT

/*Additional registers in DMEM for MCHI*/
#define MCHI_RX_TIMEOUT						0x803C
#define MCHI_BYTES_CURRENT_NODE 	0x80C4
#define MCHI_BYTES_ALL_NODES			0x80C8
/*Additional register for memory transfers over MPX */
#define DREQ_OVERRIDE							0x80cc

/*Common macros for MCHI channel*/
#define MCHI_CHANNEL_1			14
#define MCHI_CHANNEL_2			15
#define MCHI_CHANNEL_NUMBER_START	MCHI_CHANNEL_1
#define MCHI_CHANNEL_MASK	0xF0000000


#if defined (ST_498) 
/*PIO base address for 498*/
#define PIO0_OUT_SET        (PIO_0_BASE_ADDRESS + 0x04)
#define PIO1_OUT_SET        (PIO_1_BASE_ADDRESS + 0x04)
#define PIO2_OUT_SET        (PIO_2_BASE_ADDRESS + 0x04)
#define PIO3_OUT_SET        (PIO_3_BASE_ADDRESS + 0x04)
#define PIO4_OUT_SET        (PIO_4_BASE_ADDRESS + 0x04)
/*Pacing signal for MCHI Tx/RX*/
#define MCHI_RX_PACING_SIG	13
#define MCHI_TX_PACING_SIG	14
void stfdma_SetPIOBit(char PIONumber, char BitNumber);
void stfdma_SetPIOBitConfiguration(char PIONumber, char BitNumber);
void stfdma_SetDreqOverride(U32 RequestLineNo,  BOOL On, STFDMA_Block_t DeviceNo);
#if defined (STFDMA_MPX_USE_EXT_IRQ3)  
/*MPX mapped ILC base address of 7109 visible from 498*/
#define MPX_7109_ILC_BASE			STFDMA_MPX_ESTB_ILC_BASE /*7108+mb521: 0xA0C00000, Custom: 0xA1400000*/
#define MPX_7109_ILC3_SET_EN2	(MPX_7109_ILC_BASE + 0x508)
#endif
#endif /*ST_498*/



#if defined (STFDMA_MPX_SUPPORT)
/*MPX mapped PIO base address for 498 visible from 7109*/
#define MPX_498_PIO_0_BASE_ADDRESS           (STFDMA_MPX_ECM_PIO_0_BASE) /*7108+mb521:0xA3E20000 Custom:0xA2620000*/
#define MPX_498_PIO_1_BASE_ADDRESS           (STFDMA_MPX_ECM_PIO_1_BASE) /*7108+mb521: 0xA3E21000 Custom:0xA2621000*/
#define MPX_498_PIO_2_BASE_ADDRESS           (STFDMA_MPX_ECM_PIO_2_BASE) /*7108+mb521:0xA3E22000 Custom:0xA2622000*/
#define MPX_498_PIO_3_BASE_ADDRESS           (STFDMA_MPX_ECM_PIO_3_BASE) /*7108+mb521: 0xA3F20000 Custom:0xA2720000*/
#define MPX_498_PIO_4_BASE_ADDRESS           (STFDMA_MPX_ECM_PIO_4_BASE) /*7108+mb521: 0xA3F21000 Custom:0xA2721000*/

#define PIO0_OUT_CLEAR      (MPX_498_PIO_0_BASE_ADDRESS + 0x08)
#define PIO1_OUT_CLEAR      (MPX_498_PIO_1_BASE_ADDRESS + 0x08)
#define PIO2_OUT_CLEAR      (MPX_498_PIO_2_BASE_ADDRESS + 0x08)
#define PIO3_OUT_CLEAR      (MPX_498_PIO_3_BASE_ADDRESS + 0x08)
#define PIO4_OUT_CLEAR      (MPX_498_PIO_4_BASE_ADDRESS + 0x08)
void stfdma_ResetPIOBit(char PIONumber, char BitNumber);

#if defined (STFDMA_MPX_USE_EXT_IRQ3)  
#define MPX_498_ILC3_FDMA_INT_NUMBER        (14)
/*MPX mapped ILC base address for 498 visible from 7109*/
#define MPX_498_ILC_BASE											STFDMA_MPX_ECM_ILC_BASE 		/*7108+mb521: 0xA3E00000, Custom: 0xA2600000*/
#define MPX_498_COMMS_ILC_ENABLE0           		(MPX_498_ILC_BASE + 0x400)
#define MPX_498_COMMS_ILC_INT_PRIORITY14    (MPX_498_ILC_BASE + 0x870)

/*ILC register offsets for 7109*/
#define ILC3_WAKEUP_EN(addr)             			(addr + 0x608)
#define ILC3_WAKEUP_ACTIVELEVEL(addr)     (addr + 0x688)
#define ILC3_EXT_MODE(addr, n)            			(addr + 0x804 + ((n) * 0x008))
#define ILC3_EXT_PRIORITY(addr, n)        		(addr + 0xA00 + ((n) * 0x008))
#define ILC3_SET_EN2(addr)                				(addr + 0x508)
#define ILC3_CLR_EN2(addr)									(addr + 0x488)
/*SYSCONF register offsets for 7109*/
#define SYS_CFG10(addr)                   					(addr + 0x128)
#else
/*PIO used on eSTB to recieve FDMA interrupt from eCM*/
#define MPX_ESTB_PIOx_BITy   					STFDMA_MPX_ESTB_PIO_PORT, STFDMA_MPX_ESTB_PIO_BIT
#define MPX_ESTB_PIO_DEVICE_NAME		STFDMA_MPX_ESTB_PIO_NAME
void stfdma_PIOInterruptHandler(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
#endif /*STFDMA_MPX_USE_EXT_IRQ3*/
#endif /*STFDMA_MPX_SUPPORT*/

#endif /*defined (ST_498) || defined (STFDMA_MPX_SUPPORT)*/

#if defined(ST_5517) /*==================================================*/

/* CONFIGURATION FOR 5517 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x4000     /* Contains the control word interface */
#define IMEM_OFFSET             0x6000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0x5F88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            6

/* Number of available channel pools */
#define MAX_POOLS               1

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/

/* Hardware address offsets */
#define SW_ID                   0x4000     /* Block Id */

#define STATUS                  0x4004     /* Interrupt status register */

#define CMD_STAT_BASE           0x4014     /* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0xDEADC0DE      /*Node Pointer register - document not available !*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                              
#define COUNT_BASE              0x4188     /* Channel 0 bytes transfered count *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#elif defined (ST_7100)

/* CONFIGURATION FOR 7100 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/

/* Additional-data-region-0's offset & region interval */
#define ADD_DATA_REGION_0       0x9580
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define CMD_STAT_BASE           0x8040     /* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0x9180      /*Node Pointer register*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                                
#define COUNT_BASE              0x9188     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */

#elif defined  (ST_498) 
/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            14 /*For 498 last 2 channels reserved for MCHI*/

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/


/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define CMD_STAT_BASE           0x9140      /*Command Stat register*/
											/* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0x9400      /*Node Pointer register*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                            
#define COUNT_BASE              0x9408     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#define CURRENT_SADDR           0X940C
#define CURRENT_DADDR           0X9410

#elif (defined  (ST_7109) || defined (STFDMA_MPX_SUPPORT))

/* CONFIGURATION FOR 7109 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x9200
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define CMD_STAT_BASE           0x9140      /*Command Stat register*/
											/* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0x9400      /*Node Pointer register*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                            
#define COUNT_BASE              0x9408     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#define CURRENT_SADDR           0X940C
#define CURRENT_DADDR           0X9410

#elif defined  (ST_7200) || defined (ST_7111) || defined (ST_7105)

/* CONFIGURATION FOR 7200 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

#if defined (STFDMA_NAND_SUPPORT) 
#define NAND_PAGE_ERROR              0x80F8
#endif

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x9200
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define CMD_STAT_BASE           0x9140      /*Command Stat register*/
											/* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0x9580      /*Node Pointer register*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                                
#define COUNT_BASE              0x9588     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#define CURRENT_SADDR           0X958C
#define CURRENT_DADDR           0X9590

#elif defined (ST_5188)

/* CONFIGURATION FOR 5188 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            8

/* Number of available channel pools */
#define MAX_POOLS               5          /* No spdif pool available on 5188 */

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x84F0
#define REGION_INTERVAL         0x40
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define CMD_STAT_BASE           0x8030     /* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0x8070      /*Node Pointer register*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                                
#define COUNT_BASE              0x8078     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
                                            
#elif defined (ST_5162)

/* CONFIGURATION FOR 5162 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

#if defined (STFDMA_NAND_SUPPORT) 
#define NAND_PAGE_ERROR              0x802A
#endif

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4          

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x9200
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4 

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define CMD_STAT_BASE           0x9140     /* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0x9400      /*Node Pointer register*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                                
#define COUNT_BASE              0x9408     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
                                            
#elif defined  (ST_5525)

/* CONFIGURATION FOR 5525 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS                4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x9400
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define INTR_HOLDOFF            0xBFE0     /* Set command type with holdoff (READ ONLY)*/
#define INTR_SET_HOLDOFF        0xBFE4     /* Assert a command with holdoff (READ/WRITE)*/
#define INTR_CLR_HOLDOFF        0xBFE8     /* Acknowledge a command with holdoff (READ ONLY)*/
#define INTR_MASK_HOLDOFF       0xBFEC     /* Enable/Disable commands with holdoff(READ?WRITE)*/

#define CMD_STAT_BASE           0x9600      /*Command Stat register*/
											/* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0x9000      /*Node Pointer register*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                                
#define COUNT_BASE              0x9008     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#define CURRENT_SADDR           0X900C
#define CURRENT_DADDR           0X9010

#else

/* CONFIGURATION FOR 5100, 5101 and 5105 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x4000     /* Contains the control word interface */
#define IMEM_OFFSET             0x6000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0x5F88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define CMD_STAT_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */
#define NODE_PTR_CHAN_OFFSET	0x40		/*Node pointer channel offset = 64 bytes*/
#define NODE_CONTROL_OFFSET	0x04		/*Node control  offset in Node Struct = 4 bytes*/

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x44F0
#define REGION_INTERVAL         0x40
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x4000     /* Block Id */

#define STATUS                  0x5FD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0x5FD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0x5FD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0x5FDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0x5FC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0x5FC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0x5FC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0x5FCC     /* Enable/Disable commands (READ?WRITE)*/

#define CMD_STAT_BASE           0x4030     /* Channel 0 Node ptr address.
                                            * Other channels at (CMD_STAT_BASE + (Channel * CMD_STAT_CHAN_OFFSET)) */
#define NODE_PTR_BASE           0x4070      /*Node Pointer register*/
																			/* Channel 0 Node ptr address.
                                            							* Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */                                                
#define COUNT_BASE              0x4078     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#endif

/* Exported Types ------------------------------------------------------ */

/* Internal message queue control messages */
enum
{
    EXIT_MESSAGE,
    INTERRUPT_NODE_COMPLETE,
    INTERRUPT_NODE_COMPLETE_PAUSED,
    INTERRUPT_LIST_COMPLETE,
    INTERRUPT_ERROR_NODE_COMPLETE,
    INTERRUPT_ERROR_NODE_COMPLETE_PAUSED,
    INTERRUPT_ERROR_LIST_COMPLETE,
    INTERRUPT_ERROR_SCLIST_OVERFLOW,
    INTERRUPT_ERROR_BUFFER_OVERFLOW
#if defined (STFDMA_NAND_SUPPORT)    
    ,INTERRUPT_ERROR_NAND_READ
#endif    
#if defined (STFDMA_MPX_SUPPORT)
    ,INTERRUPT_ERROR_MCHI_TIMEOUT
#endif
};

/* Callback task reference data */
typedef struct
{
    void        *TaskStack_p;                    /* Pointer to stack for task usage */
    tdesc_t     TaskDesc;                       /* Task descriptor */
    task_t      *pTask;
} TaskData_t;

/* Channel descriptors */
typedef struct
{
    U32                 TransferId;             /* Id of the associated transfer */
    int                 Pool;                   /* Which pool does this channel belong to */
    BOOL                Error;                  /* Set when the transfer completes with the error bit set */
    BOOL                Locked;                 /* Channel locked indicator */
    BOOL                Paused;                 /* Channel paused indicator */
    STFDMA_RequestSignal_t    PacedSignalUsed; /*Set the Paced signal to be used*/
    BOOL                Abort;                  /* Abort of transfer is pendind */
    BOOL                Idle;                   /* A blocking transfer has completed */
    BOOL                Blocking;               /* Transfer is a blocking transfer */
    semaphore_t*        pBlockingSemaphore;     /* Semaphore to wait on when blocking */
    U32                 BlockingReason;         /* Reason for blocking semap signal */
    STFDMA_CallbackFunction_t   CallbackFunction_p; /* Ptr to application callback */
    void                *ApplicationData_p;     /* Application data passed through to callback */
    U32                 NextFreeChannel;        /* Index to next free channel */
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    U32			        GenericDMA;		        /* If true, indicate the channel is requested by
                      						       Linux driver via a generic DMA driver (Not a Stapi driver) */
#endif
} ChannelElement_t;

/* Message Queue element */
typedef struct
{
    U8  ChannelNumber;
    U8  InterruptType;
    U16 InterruptData;
    STOS_Clock_t InterruptTime;
} MessageElement_t;


/* Main control block */
typedef struct
{
    ST_DeviceName_t             DeviceName;                         /* Name of STFDMA instance */
    STFDMA_Block_t              DeviceNumber;                       /* STFDMA device number */
    U32                         *BaseAddress_p;                     /* Device base address */
    ST_Partition_t              *DriverPartition_p;                 /* General partition ptr */
    ST_Partition_t              *NCachePartition_p;                 /* Non cached parition */
    U32                         FreeChannelIndex[MAX_POOLS];        /* Next available free channel */
    semaphore_t*                pAccessSemaphore;
    ChannelElement_t            ChannelTable[NUM_CHANNELS];         /* Channel data */
    U8                          TransferTotal;                      /* Count of transfers */
    U32                         NumberCallbackTasks;                /* Number of callback tasks */
    TaskData_t                  *CallbackTaskTable_p;               /* Ptr to table of callback task ids */
    U32                         ClockTicksPerSecond;                /* clock ticks per second - used for seamphore_timeout */
    BOOL                        Terminate;                          /* Driver terminating flag */
    U32                         InterruptNumber;                    /* Driver interrupt number */
    U32                         InterruptLevel;                     /* Driver interrupt level */
#if defined (ST_OS21) || defined (ST_OSWINCE)
    interrupt_t*                hInterrupt;
#endif

    S32                         ProducerIndex;
    S32                         ConsumerIndex;
    BOOL                        QueueFull;
    BOOL                        QueueEmpty;
    MessageElement_t           *MessageQueue;
    semaphore_t                *MessageQueueSem;
    semaphore_t                *MessageReadySem;
#if defined (STFDMA_MPX_SUPPORT)
    STFDMA_Device_t             DeviceType;                         /* Type of FDMA device */
#endif
} ControlBlock_t;

#if defined (STFDMA_DEBUG_SUPPORT)
typedef struct STFDMA_Status_s
{
    struct
    {
        U32 InterruptsTotal;
        U32 InterruptsBlocking;
        U32 InterruptsNonBlocking;
        U32 InterruptsMissed;
        U32 InterruptBlockingSemaphoreReceived;
        U32 Callbacks;
    }Channel[NUM_CHANNELS];
}STFDMA_Status_t;
#endif

/* Exported Variables -------------------------------------------------- */

/* Global pointer for control block reference */
extern ControlBlock_t *stfdma_ControlBlock_p[STFDMA_MAX];

/* Exported Macros ----------------------------------------------------- */

#ifdef STFDMA_SIM
/*...for simulator usage only. Uses simulator register access functions */
#define SET_REG(Offset,Value)         FDMASIM_SetReg(Offset, Value)
#define GET_REG(Offset)               FDMASIM_GetReg(Offset)
#define SET_CMD_STAT(Channel, Value)     ( SET_REG((CMD_STAT_BASE + ((Channel) * CMD_STAT_CHAN_OFFSET)), ((U32)Value)) )
#else
#define SET_REG(Offset, Value, Device) STSYS_WriteRegDev32LE((((U32)stfdma_ControlBlock_p[Device]->BaseAddress_p) + (Offset)),((U32)Value))
#define GET_REG(Offset, Device)		STSYS_ReadRegDev32LE((void*)(((U32)stfdma_ControlBlock_p[Device]->BaseAddress_p) + (Offset)))
#define SET_CMD_STAT(Channel, Value, Device)     SET_REG((CMD_STAT_BASE + ((Channel) * CMD_STAT_CHAN_OFFSET)), (Value), Device) 
#endif

#define IS_INITIALISED(Device)                  (stfdma_ControlBlock_p[Device] != NULL)

#define GET_BYTE_COUNT(Channel, Device)          GET_REG(COUNT_BASE + ((Channel) * COUNT_CHAN_OFFSET), Device) 

#if defined (ST_5105) || defined (ST_5100)  || defined (ST_5188) || defined (ST_5107) 
#define SET_BYTE_COUNT(Channel, n, Device)       SET_REG((COUNT_BASE + ((Channel) * COUNT_CHAN_OFFSET)), n, Device)
#endif

#define IS_BLOCKING(Channel, Device)            (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Blocking)
#define SET_BLOCKING(Channel, Device)           (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Blocking = TRUE)
#define CLEAR_BLOCKING(Channel, Device)         (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Blocking = FALSE)

#define GET_BLOCKING_SEM_PTR(Channel, Device)   (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].pBlockingSemaphore)

#define GET_BLOCKING_REASON(Channel, Device)    (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].BlockingReason)
#define SET_BLOCKING_REASON(Channel, Reason, Device)(stfdma_ControlBlock_p[Device]->ChannelTable[Channel].BlockingReason = Reason)

#define IS_ABORTING(Channel, Device)            (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Abort)
#define SET_ABORTING(Channel, Device)           (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Abort = TRUE)
#define CLEAR_ABORTING(Channel, Device)         (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Abort = FALSE)

#define IS_IDLE(Channel, Device)                (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Idle)
#define SET_IDLE(Channel, Device)               (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Idle = TRUE)
#define CLEAR_IDLE(Channel, Device)             (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Idle = FALSE)

#if !defined (ST_OSWINCE) /* Collision - WinError.h */
#define IS_ERROR(Channel, Device)               (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Error)
#endif
#define SET_ERROR(Channel, Device)              (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Error = TRUE)
#define CLEAR_ERROR(Channel, Device)            (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Error = FALSE)

#if defined(ST_5517)
#define GET_CURRENT_NODE_PTR(Channel, Device)      GET_REG(CMD_STAT_BASE + ((Channel) * CMD_STAT_CHAN_OFFSET), Device) 
#else
#define GET_CURRENT_NODE_PTR(Channel, Device)      (GET_REG(CMD_STAT_BASE + ((Channel) * CMD_STAT_CHAN_OFFSET), Device) & ~0X1F )
#define GET_NODE_STATE(Channel, Device)   (GET_REG(CMD_STAT_BASE + ((Channel) * CMD_STAT_CHAN_OFFSET), Device) &  0X1F )
#endif

#define GET_NEXT_NODE_PTR(Channel, Device)      GET_REG(NODE_PTR_BASE + ((Channel) * NODE_PTR_CHAN_OFFSET), Device) 
#define GET_NODE_CONTROL(Channel, Device)	GET_REG(NODE_PTR_BASE + ((Channel) * NODE_PTR_CHAN_OFFSET) + NODE_CONTROL_OFFSET, Device) 

/* Exported Function Prototypes ---------------------------------------- */

#if defined (ST_5525) || defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
ST_ErrorCode_t   stfdma_MuxConf(void);
#endif

int  stfdma_FDMA1Conf(void *ProgAddr, void *DataAddr);
int  stfdma_FDMA2Conf(void *ProgAddr, void *DataAddr, STFDMA_Block_t DeviceNo);
void stfdma_InitContexts(U32 *BaseAddress_p, ST_Partition_t  *DriverPartition_p, STFDMA_Block_t DeviceNo);
int  stfdma_TermContexts(STFDMA_Block_t DeviceNo, ST_Partition_t  *DriverPartition_p);

ST_ErrorCode_t stfdma_ReqConf(ControlBlock_t *ControlBlock_p, STFDMA_Block_t DeviceNo);
BOOL stfdma_IsNodePauseEnabled(U32 Channel, STFDMA_Block_t DeviceNo);

STOS_INTERRUPT_DECLARE(stfdma_InterruptHandler, pParams);

U32 stfdma_GetNextNodePtr(U32 Channel, STFDMA_Block_t DeviceNo);

void stfdma_SendMessage(MessageElement_t * Message, STFDMA_Block_t DeviceNo);

#endif

/*eof*/
