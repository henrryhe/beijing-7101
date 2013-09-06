/******************************************************************************/
/*                                                                            */
/* File name   : COCOREF_V2_7100.H                                            */
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

#ifndef _COCOREF_V2_7100_H_
#define _COCOREF_V2_7100_H_

/* C++ support */
/* ----------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
/* -------- */
#include "sti7100.h"

/* Memory mapping */
/* -------------- */
/*    LMI_SYS => LX Deltaphi         : 1MB  */
/*               LX Audio            : 2MB  */
/*               ST40 Code           : 29MB */
/*               AVMEM Partition 0   : 32MB */
/*                                          */
/*    LMI_VID => AVMEM Partition 1   : 64MB */

/* Memory mapping for LMI SYS */
/* -------------------------- */
#define RAM1_BASE_ADDRESS                      0xA4000000 /* LMI_SYS */
#define RAM1_SIZE                              0x04000000 /* 64 Mbytes */
#define RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1    0x04000000
#define AVMEM1_BASE_ADDRESS                    0xA6000000 /* Start AVMEM partition after 32MB */ 
#define AVMEM1_BASE_ADDRESS_SEEN_FROM_DEVICE1  0x06000000
#define AVMEM1_BASE_ADDRESS_SEEN_FROM_DEVICE2  0x06000000
#define AVMEM1_SIZE                            0x02000000 /* 32 Mbytes */
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

/* Memory mapping for driver partition */
/* ----------------------------------- */
#define SYSTEM_MEMORY_SIZE (0x00C00000) /* 12 Mbytes */
#define NCACHE_MEMORY_SIZE (0x00300000) /*  3 Mbytes */

/* Lx code position in LMI SYS */
/* --------------------------- */
#define VIDEO_COMPANION_OFFSET 0x00000000
#define AUDIO_COMPANION_OFFSET 0x00100000

/* Board memory mapping */
/* -------------------- */
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


