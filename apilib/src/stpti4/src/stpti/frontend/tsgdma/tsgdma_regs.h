/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2006, 2007.  All rights reserved.

   File Name: tsgdma_regs.h
 Description: TSGDMA Register definitions

******************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __TSGDMA_REGS_H
#define __TSGDMA_REGS_H


#include "stdevice.h"


/* PJW This should be in a 7200 soecific file */
#define TSGDMA_NUM_LIVE_CHANNELS        (6)
#define TSGDMA_NUM_SW_CHANNELS          (3)
#define TSGDMA_NUM_TS                   (TSGDMA_NUM_LIVE_CHANNELS + TSGDMA_NUM_SW_CHANNELS)

#define TSGDMA_NUM_PTIS                 (2)
#define TSGDMA_NUM_DEST                 (4) /* PJW should this be 2*4 = 8 */

#define TSGDMA_TAG_BYTES_LENGTH         (8)
#define TSGDMA_TSG_LIVE_CH_PARCEL_SIZE  (4)

#define TSGDMA_SW_CHANNEL_MASK          (0x07)

/*********** TSGDMA General  **************/
#define TSGDMA_BASE                           (STPTI_Driver.TSGDMA_Base_Address)
#define TSGDMA_REGION_SIZE                    (0x10000)
#define TSGDMA_MAILBOX_FLAG_INTERRUPT         (ST7200_TSG_PTI_MAILBOX_FLAG_INTERRUPT)

#define TSGDMA_DMEM_MEM_BASE                  (TSGDMA_BASE + 0x8000) /* DMEM memory */
#define TSGDMA_DMEM_PER_BASE                  (TSGDMA_BASE + 0xBF00) /* DMEM peripherals : this offset is not actually used in this file */

#define TSGDMA_IMEM_BASE                      (TSGDMA_BASE + 0xC000)
#define TSGDMA_SLIM_CPU_RUN                   (TSGDMA_BASE + 0x0008) /* Enable slimcore */
#define TSGDMA_SLIM_CPU_CLK_GATE              (TSGDMA_BASE + 0x000C) /* Reset slimcore */
#define TSGDMA_SLIM_CPU_PC                    (TSGDMA_BASE + 0x0020)
/*********** TSGDMA DMEM Peripherals ************** */

/* HARDWARE SPECIFIC*/
#define TSGDMA_DMAREQHOLDOFF(Req_No)          (TSGDMA_DMEM_MEM_BASE + ( (0xFC0+ Req_No) <<2) )

#define TSGDMA_STBUS_SYNC                     (TSGDMA_DMEM_MEM_BASE + (0xFE2 << 2) )
#define TSGDMA_STBUS_ACCESS                   (TSGDMA_DMEM_MEM_BASE + (0xFE3 << 2) )
#define TSGDMA_STBUS_ADDRESS                  (TSGDMA_DMEM_MEM_BASE + (0xFE4 << 2) )

#define TSGDMA_COUNTER                        (TSGDMA_DMEM_MEM_BASE + (0xFE5 << 2) )
#define TSGDMA_COUNTER_TRIGGER_VAL            (TSGDMA_DMEM_MEM_BASE + (0xFE6 << 2) )
#define TSGDMA_COUNTER_TRIGGER                (TSGDMA_DMEM_MEM_BASE + (0xFE7 << 2) )

#define TSGDMA_PACE_COUNTER                   (TSGDMA_DMEM_MEM_BASE + (0xFE8 << 2) )
#define TSGDMA_PACE_COUNTER_TRIGGER_VAL       (TSGDMA_DMEM_MEM_BASE + (0xFE9 << 2) )
#define TSGDMA_PACE_COUNTER_TRIGGER           (TSGDMA_DMEM_MEM_BASE + (0xFEA << 2) )
#define TSGDMA_PACING_MULTIPLE                (TSGDMA_DMEM_MEM_BASE + (0xFEB << 2) )

#define TSGDMA_T2INIT_DATA_FLAG               (TSGDMA_DMEM_MEM_BASE + (0xFEC << 2) )
#define TSGDMA_TSCOPRO_IDLE_FLAG              (TSGDMA_DMEM_MEM_BASE + (0xFED << 2) )
#define TSGDMA_1394_CLOCK_SELECT0             (TSGDMA_DMEM_MEM_BASE + (0xFEE << 2) )
#define TSGDMA_1394_CLOCK_SELECT1             (TSGDMA_DMEM_MEM_BASE + (0xFEF << 2) )


/*** MAILBOXES ***/
#define TSGDMA_MAILBOX0_STATUS                (TSGDMA_DMEM_MEM_BASE + (0xFF0 << 2) ) /* 0xFF0 << 2 = 0x3FC0 */
#define TSGDMA_MAILBOX0_SET                   (TSGDMA_DMEM_MEM_BASE + (0xFF1 << 2) )
#define TSGDMA_MAILBOX0_CLEAR                 (TSGDMA_DMEM_MEM_BASE + (0xFF2 << 2) )
#define TSGDMA_MAILBOX0_MASK                  (TSGDMA_DMEM_MEM_BASE + (0xFF3 << 2) )

#define TSGDMA_MAILBOX1_STATUS                (TSGDMA_DMEM_MEM_BASE + (0xFF4 << 2) )
#define TSGDMA_MAILBOX1_SET                   (TSGDMA_DMEM_MEM_BASE + (0xFF5 << 2) )
#define TSGDMA_MAILBOX1_CLEAR                 (TSGDMA_DMEM_MEM_BASE + (0xFF6 << 2) )
#define TSGDMA_MAILBOX1_MASK                  (TSGDMA_DMEM_MEM_BASE + (0xFF7 << 2) )


/*  HW specific */
#define TSGDMA_DMAREQSTATUS                   (TSGDMA_DMEM_MEM_BASE + (0xFF8 << 2) )
#define TSGDMA_DMAREQMASK                     (TSGDMA_DMEM_MEM_BASE + (0xFF9 << 2) )
#define TSGDMA_DMAREQDIV                      (TSGDMA_DMEM_MEM_BASE + (0xFFA << 2) )
#define TSGDMA_DMAREQDIVCOUNT                 (TSGDMA_DMEM_MEM_BASE + (0xFFB << 2) )
#define TSGDMA_DMAREQTEST                     (TSGDMA_DMEM_MEM_BASE + (0xFFC << 2) )
#define TSGDMA_GPOUT                          (TSGDMA_DMEM_MEM_BASE + (0xFFD << 2) )

#define TSGDMA_T2_INIT                        (TSGDMA_DMEM_MEM_BASE + (0xFEC << 2) )
#define TSGDMA_CP_IDLE                        (TSGDMA_DMEM_MEM_BASE + (0xFED << 2) )

/*** FIRMWARE DEFINED registers ***/
#define TSGDMA_FW_BASE                        (0x100)

/* Sw_Ch_No is in [0,1,2] */
#define TSGDMA_SWTS_REG_BASE(Sw_Ch_No)        ( TSGDMA_DMEM_MEM_BASE + TSGDMA_FW_BASE + 0x40*(TSGDMA_NUM_LIVE_CHANNELS + Sw_Ch_No) )

/* Live_Ch_No is in [0,1,2,3,4,5] */
#define TSGDMA_LIVE_REG_BASE(Live_Ch_No)      ( TSGDMA_DMEM_MEM_BASE + TSGDMA_FW_BASE + 0x40*Live_Ch_No )

/* SW Stream */
#define TSGDMA_BYTES_THRESHOLD(Sw_Ch_No)      ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x00 )
#define TSGDMA_CURRENT_NODE(Sw_Ch_No)         ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x04 )
#define TSGDMA_NEXT_NODE(Sw_Ch_No)            ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x08 )
#define TSGDMA_CTRL(Sw_Ch_No)                 ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x0C )
#define TSGDMA_SOFT_BUFFERBASE(Sw_Ch_No)      ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x10 )
#define TSGDMA_SOFT_BUFFERSIZE(Sw_Ch_No)      ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x14 )
#define TSGDMA_SOFT_PACKET_LENGTH(Sw_Ch_No)   ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x18 )
#define TSGDMA_SOFT_DEST(Sw_Ch_No)            ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x1C )
#define TSGDMA_TAG_HEADER(Sw_Ch_No)           ( TSGDMA_SWTS_REG_BASE(Sw_Ch_No) +  0x20 )

/* Live Stream */
#define TSGDMA_LIVE_BUFFERBASE(Live_Ch_No)    ( TSGDMA_LIVE_REG_BASE(Live_Ch_No) + 0x00 )
#define TSGDMA_LIVE_BUFFERSIZE(Live_Ch_No)    ( TSGDMA_LIVE_REG_BASE(Live_Ch_No) + 0x04 )
#define TSGDMA_LIVE_PACKET_LENGTH(Live_Ch_No) ( TSGDMA_LIVE_REG_BASE(Live_Ch_No) + 0x08 )
#define TSGDMA_LIVE_DEST(Live_Ch_No)          ( TSGDMA_LIVE_REG_BASE(Live_Ch_No) + 0x0C )

/* OTHER */
#define TSGDMA_CHANNEL_STALL_ENABLE           ( TSGDMA_DMEM_MEM_BASE + TSGDMA_FW_BASE  + 0x40*TSGDMA_NUM_TS)
#define TSGDMA_CHANNEL_0_CFG                  ( TSGDMA_DMEM_MEM_BASE + TSGDMA_FW_BASE  + (0x40*TSGDMA_NUM_TS)+4)
#define TSGDMA_CHANNEL_1_CFG                  ( TSGDMA_DMEM_MEM_BASE + TSGDMA_FW_BASE  + (0x40*TSGDMA_NUM_TS)+8)

#endif /* __TSGDMA_REGS_H */
