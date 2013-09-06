/******************************************************************************/
/*                                                                            */
/* File name   : COCOREF_V2_7109.H                                            */
/*                                                                            */
/* Description : Configuration of the COCOREF V2 board                        */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/*                                                                            */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 02/03/05           Created                      M.CHAPPELLIER              */
/*                                                                            */
/******************************************************************************/

#ifndef _COCOREF_V2_7109_H_
#define _COCOREF_V2_7109_H_

/* C++ support */
/* ----------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
/* -------- */
#include "sti7109.h"

/*yxl 2007-05-30 add below section*/

/* Clock rates ------------------------------------------------------------- */

/* These are for internal use to simplify STCOMMON. For external use,
   see ST_GetClockInfo. The values are in Hz, as in ST_GetClockInfo_t */
#ifdef ST_7100
#define STCOMMON_IC_CLOCK                  100000000 /*for cut2*/
#define STCOMMON_ST40CORE_CLOCK            266000000
#define STCOMMON_ST40PERIPHERAL_CLOCK       66000000
#define STCOMMON_ST20_CLOCK                        0 /*No ST20*/
#define STCOMMON_ST20TICK_CLOCK                    0 /*No ST20*/
#define STCOMMON_USB_IRDA_CLOCK             48000000
#define STCOMMON_PWM_CLOCK                  27000000
#define STCOMMON_HDD_CLOCK                 100000000
#define STCOMMON_PCI_CLOCK                         0
#define STCOMMON_EMI_CLOCK                 100000000
#define STCOMMON_LMI_CLOCK                 192000000 /*192 for cut2&3*/
#define STCOMMON_VDEC_SPDIFDEC_CLOCK        66600000
#define STCOMMON_ADEC_CLOCK                384000000 /*Audio decoder ST231 core clock - 384MHz-cut 2 & 3*/
#define STCOMMON_ABIT_CLOCK                 27000000
#define STCOMMON_SMARTCARD_CLOCK            27000000
#define STCOMMON_VIDPIX2X_TTX_CLOCK         27000000
#define STCOMMON_VIDPIX1X_CLOCK            (STCOMMON_VIDPIX2X_TTX_CLOCK/2)
#define STCOMMON_CLKOUT_CLOCK                      0
#define STCOMMON_FLASH_CLOCK               STCOMMON_EMI_CLOCK
#define STCOMMON_SDRAM_CLOCK               STCOMMON_EMI_CLOCK
#endif

#ifdef ST_7109
#define STCOMMON_IC_CLOCK                  100000000
#define STCOMMON_ST40CORE_CLOCK            266000000
#define STCOMMON_ST40PERIPHERAL_CLOCK       66000000
#define STCOMMON_ST20_CLOCK                        0 /*No ST20*/
#define STCOMMON_ST20TICK_CLOCK                    0 /*No ST20*/
#define STCOMMON_USB_IRDA_CLOCK             48000000
#define STCOMMON_PWM_CLOCK                  27000000
#define STCOMMON_HDD_CLOCK                 100000000
#define STCOMMON_PCI_CLOCK                         0
#define STCOMMON_EMI_CLOCK                 100000000
#define STCOMMON_LMI_CLOCK                 200000000
#define STCOMMON_VDEC_SPDIFDEC_CLOCK        66600000
#define STCOMMON_ADEC_CLOCK                400000000 /*Audio decoder ST231 core clock*/
#define STCOMMON_ABIT_CLOCK                 27000000
#define STCOMMON_SMARTCARD_CLOCK            27000000
#define STCOMMON_VIDPIX2X_TTX_CLOCK         27000000
#define STCOMMON_VIDPIX1X_CLOCK            (STCOMMON_VIDPIX2X_TTX_CLOCK/2)
#define STCOMMON_CLKOUT_CLOCK                      0
#define STCOMMON_FLASH_CLOCK               STCOMMON_EMI_CLOCK
#define STCOMMON_SDRAM_CLOCK               STCOMMON_EMI_CLOCK
#endif

/*end yxl 2007-05-30 add below section*/

/* Memory mapping */
/* -------------- */
/*    LMI_SYS => LX Deltaphi         : 2MB  */
/*               LX Audio            : 2MB  */
/*               ST40 Code           : 28MB */
/*               AVMEM Partition 0   : 32MB */
/*                                          */
/*    LMI_VID => AVMEM Partition 1   : 64MB */

/* Memory mapping for LMI SYS */
/* -------------------------- */
#if 0/*20070808 change memory*/

#define RAM1_BASE_ADDRESS                        0xA4000000 /* LMI_SYS */
#define RAM1_SIZE                                         0x04000000 /* 64 Mbytes */
#define RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1    0x04000000
#define AVMEM1_BASE_ADDRESS                    0xA6400000 /* Start AVMEM partition after 32MB */ 
#define AVMEM1_BASE_ADDRESS_SEEN_FROM_DEVICE1  0x06400000
#define AVMEM1_BASE_ADDRESS_SEEN_FROM_DEVICE2  0x06400000
#define AVMEM1_SIZE                            0x01C00000 /* 32 Mbytes */
#define CACHE_START_ADDRESS                    0x00000000 /* Not used on ST40 */
#define CACHE_STOP_ADDRESS                     0x00000000 /* Not used on ST40 */

/* Memory mapping for LMI VID */
/* -------------------------- */
#define RAM2_BASE_ADDRESS                      0xB0000000 /* LMI_VID */
#define RAM2_SIZE                              (0x04000000-0xF000) /* 64 Mbytes - 1920*32 for video decoders parser */
#define AVMEM2_BASE_ADDRESS                    0xB0000000 
#define AVMEM2_BASE_ADDRESS_SEEN_FROM_DEVICE1  0x10000000
#define AVMEM2_BASE_ADDRESS_SEEN_FROM_DEVICE2  0x10000000
#define AVMEM2_SIZE                            (0x04000000-0xF000) /* 64 Mbytes - 1920*32 for video decoders parser */ 
#else
#ifdef LMI_SYS128

#define RAM1_BASE_ADDRESS                       0xA4000000 /* LMI_SYS */
#define RAM1_SIZE                               0x10000000 /* 128 Mbytes */
#define RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1     0x04000000
#define AVMEM1_BASE_ADDRESS                     (0xAA000000) /* Start AVMEM partition after 32MB */ 
#define AVMEM1_BASE_ADDRESS_SEEN_FROM_DEVICE1   (0xAA000000)
#define AVMEM1_BASE_ADDRESS_SEEN_FROM_DEVICE2   (0xAA000000)
#define AVMEM1_SIZE                             (0x02000000) /* 32 Mbytes */
#define CACHE_START_ADDRESS                     0x00000000 /* Not used on ST40 */
#define CACHE_STOP_ADDRESS                      0x00000000 /* Not used on ST40 */

/* Memory mapping for LMI VID */
/* -------------------------- */
#define RAM2_BASE_ADDRESS                      0xB0000000 /* LMI_VID */
#define RAM2_SIZE                            (0x04000000-0xF000) 
#define AVMEM2_BASE_ADDRESS                    0xB0000000 
#define AVMEM2_BASE_ADDRESS_SEEN_FROM_DEVICE1  0x10000000
#define AVMEM2_BASE_ADDRESS_SEEN_FROM_DEVICE2  0x10000000
#define AVMEM2_SIZE                            (0x04000000-0xF000) /* 64 Mbytes - 1920*32 for video decoders parser */

#else

#define RAM1_BASE_ADDRESS                        0xA4000000 /* LMI_SYS */
#define RAM1_SIZE                                         0x08000000 /* 64 Mbytes */
#define RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1    0x04000000
#define AVMEM1_BASE_ADDRESS                    (0xA7100000-0x00500000) /* Start AVMEM partition after 32MB */ 
#define AVMEM1_BASE_ADDRESS_SEEN_FROM_DEVICE1  (0x07100000-0x00500000)
#define AVMEM1_BASE_ADDRESS_SEEN_FROM_DEVICE2  (0x07100000-0x00500000)
#define AVMEM1_SIZE                                    (0x00F00000+0x00500000) /* 20 Mbytes */
#define CACHE_START_ADDRESS                    0x00000000 /* Not used on ST40 */
#define CACHE_STOP_ADDRESS                     0x00000000 /* Not used on ST40 */

/* Memory mapping for LMI VID */
/* -------------------------- */
#define RAM2_BASE_ADDRESS                      0xB0000000 /* LMI_VID */
/*#define RAM2_SIZE                              (0x04000000-0xF000)  lzf modi this *//* 64 Mbytes - 1920*32 for video decoders parser */
#define RAM2_SIZE                              (0x10000000 -0xF000)
#define AVMEM2_BASE_ADDRESS                    0xB0000000 
#define AVMEM2_BASE_ADDRESS_SEEN_FROM_DEVICE1  0x10000000
#define AVMEM2_BASE_ADDRESS_SEEN_FROM_DEVICE2  0x10000000
#define AVMEM2_SIZE                            (0x04000000-0xF000) /* 64 Mbytes - 1920*32 for video decoders parser */
#endif

#endif


/* Memory mapping for driver partition */
/* ----------------------------------- */
#define SYSTEM_MEMORY_SIZE (0x00C00000) /* 12 Mbytes */
#define NCACHE_MEMORY_SIZE (0x00300000) /*  3 Mbytes */

/* Lx code position in LMI SYS */
/* --------------------------- */
#define VIDEO_COMPANION_OFFSET 0x00000000
#define AUDIO_COMPANION_OFFSET 0x00200000 /*0x00200000 0x00300000*/

/* Board memory mapping */
/* -------------------- */
/* Please undefine this LAN91 constant to switch to internal MII 7109 interface */
#define LAN91C111_BASE_ADDRESS 0xA2000000
#define ATAPI_BASE_ADDRESS     0xA2800000

/* Interrupt level mapping */
/* ----------------------- */
#define ASC_0_INTERRUPT_LEVEL       1
#define ASC_1_INTERRUPT_LEVEL       1
#define ASC_2_INTERRUPT_LEVEL       1
#define ASC_3_INTERRUPT_LEVEL       1
#define ASC_4_INTERRUPT_LEVEL       1
#define PIO_0_INTERRUPT_LEVEL       1
#define PIO_1_INTERRUPT_LEVEL       1
#define PIO_2_INTERRUPT_LEVEL       1
#define PIO_3_INTERRUPT_LEVEL       1
#define PIO_4_INTERRUPT_LEVEL       1
#define PIO_5_INTERRUPT_LEVEL       1
#define PIO_6_INTERRUPT_LEVEL       1
#define PIO_7_INTERRUPT_LEVEL       1
#define TTXT_INTERRUPT_LEVEL        2
#define IRB_INTERRUPT_LEVEL         2
#define AUDIO_INTERRUPT_LEVEL       3
#define EXTERNAL_0_INTERRUPT_LEVEL  4
#define EXTERNAL_1_INTERRUPT_LEVEL  4
#define EXTERNAL_2_INTERRUPT_LEVEL  4
#define EXTERNAL_3_INTERRUPT_LEVEL  4
#define VIDEO_INTERRUPT_LEVEL       4
#define SSC_0_INTERRUPT_LEVEL       5
#define SSC_1_INTERRUPT_LEVEL       5
#define SSC_2_INTERRUPT_LEVEL       5
#define PCP_INTERRUPT_LEVEL         5
#define BLIT_INTERRUPT_LEVEL        5
#define DCO_INTERRUPT_LEVEL         5
#define ATA_INTERRUPT_LEVEL         6
#define PWM_INTERRUPT_LEVEL         6
#define DENC_INTERRUPT_LEVEL        7
#define VTG_INTERRUPT_LEVEL         7
#define HDMI_INTERRUPT_LEVEL        8
#define DMA_INTERRUPT_LEVEL         13
#define PTI_INTERRUPT_LEVEL         14

/* STAPI modules priorities */
/* ------------------------ */
/* Task priority is a sensible part of the system                                 */
/* We assume that range of priorities between [8..15] are reserved for critical   */
/* and real time drivers.                                                         */
/* So please don't change these tasks priorities without to know the consequences */
#if 0/*20070717 del for test*/
#define STPTI_INDEX_TASK_PRIORTY                     ((MAX_USER_PRIORITY-15)+9)
#define STPTI_INTERRUPT_TASK_PRIORTY                 ((MAX_USER_PRIORITY-15)+15)
#define STPTI_EVENT_TASK_PRIORTY                     ((MAX_USER_PRIORITY-15)+8)
#define STTUNER_SCAN_SAT_TASK_PRIORITY               ((MAX_USER_PRIORITY-15)+3)
#define STTUNER_SCAN_TER_TASK_PRIORITY               ((MAX_USER_PRIORITY-15)+3)
#define STTUNER_SCAN_CAB_TASK_PRIORITY               ((MAX_USER_PRIORITY-15)+3)
#define STUART_TIMER_TASK_PRIORITY                   ((MAX_USER_PRIORITY-15)+4)
#define STVID_TASK_PRIORITY_DECODE                   ((MAX_USER_PRIORITY-15)+11)
#define STVID_TASK_PRIORITY_DISPLAY                  ((MAX_USER_PRIORITY-15)+13)
#define STVID_TASK_PRIORITY_ERROR_RECOVERY           ((MAX_USER_PRIORITY-15)+10)
#define STVID_TASK_PRIORITY_TRICKMODE                ((MAX_USER_PRIORITY-15)+12)
#define STVID_TASK_PRIORITY_INJECTERS                ((MAX_USER_PRIORITY-15)+12)
#define STFDMA_CALLBACK_TASK_PRIORITY                ((MAX_USER_PRIORITY-15)+9)
#define STCLKRV_FILTER_TASK_PRIORITY                 ((MAX_USER_PRIORITY-15)+6)
#define STBLAST_TIMER_TASK_PRIORITY                  ((MAX_USER_PRIORITY-15)+5)
#define /*STATAPI_TASK_PRIORITY*/BAT_PRIORITY        ((MAX_USER_PRIORITY-15)+8) 
#define STSMART_EVENTMGR_TASK_PRIORITY               ((MAX_USER_PRIORITY-15)+7)
#define STBLIT_MASTER_TASK_STACK_PRIORITY            ((MAX_USER_PRIORITY-15)+4)
#define STBLIT_SLAVE_TASK_STACK_PRIORITY             ((MAX_USER_PRIORITY-15)+4)
#define STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY ((MAX_USER_PRIORITY-15)+7)
#define DATAPROCESSER_TASK_PRIORITY                  ((MAX_USER_PRIORITY-15)+6)
#define AUDIO_DECODER_TASK_PRIORITY                  ((MAX_USER_PRIORITY-15)+6)
#define AUDIO_MIXER_TASK_PRIORITY                    ((MAX_USER_PRIORITY-15)+6)
#define PES_ES_PARSER_TASK_PRIORITY                  ((MAX_USER_PRIORITY-15)+8)
#define SPDIFPLAYER_TASK_PRIORITY                    ((MAX_USER_PRIORITY-15)+6)
#define PCMPLAYER_TASK_PRIORITY                      ((MAX_USER_PRIORITY-15)+6)
#define AUDIO_PP_TASK_PRIORITY                       ((MAX_USER_PRIORITY-15)+6)
#endif
/* Application declarations */
/* ------------------------ */
#define TRACE_UART_ID    0 /* UART_DeviceName[id] used for traces   */
#define TESTTOOL_UART_ID 0 /* UART_DeviceName[id] used for testtool */

/* C++ support */
/* ----------- */
#ifdef __cplusplus
}
#endif
#endif


