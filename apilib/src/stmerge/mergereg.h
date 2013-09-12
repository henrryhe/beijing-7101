/*****************************************************************************
File Name   : mergereg.h

Description : Register address defines for the STMERGE driver

Copyright (C) 2003 STMicroelectronics

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __MERGEREG_H
#define __MERGEREG_H


/* Includes --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif


/* base address of CFG register */
#define TSM_STREAM_CFG_BASE       0x0000

/* base address of SYNC register */
#define TSM_STREAM_SYNC_BASE      0x0008

/* base address of STATUS register */
#define TSM_STREAM_STA_BASE       0x0010

/* base address of CFG1 register */
#if defined(ST_7100) || defined(ST_7109)
#define TSM_STREAM_CFG2_BASE      0x0018
#endif

/* constant offset between each stream */
#define STREAM_OFFSET             0x0020
#if defined(ST_5528) || defined(ST_5525) || defined(ST_5524)
#define LAST_OFFSET               0x0120
#elif defined(ST_5100) || defined(ST_7710) || defined(ST_5301) || defined(ST_7100)
#define LAST_OFFSET               0x80
#elif defined(ST_7109)
#define LAST_OFFSET               0xC0 /* stream_conf_6 */
#endif

/* SWTS CFG registers */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#define TSM_SWTS_CFG_BASE         0x0000
#define TSM_SWTS_CFG_EXPANDED     0x0600
#else /* 5528,5100,7710,5301 */
#define TSM_SWTS_CFG_BASE         0x0288
#define TSM_SWTS_CFG_EXPANDED     0x02E0
#endif

#define SWTS_OFFSET               0x0010


/* Swts counter register */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#define TSM_CNT_BASE              0x0400
#define CNT_OFFSET                0x0010
#else /* 5528,5100,7710,5301 */
#define TSM_CNT_BASE              0x0240
#define CNT_OFFSET                0x0008
#endif

/* Altout counter register */
#if defined(ST_7100)
#define TSM_ALTOUT_CNT            0x0410
#elif defined(ST_7109)
#define TSM_ALTOUT_CNT            0x0430
#elif defined(ST_5301)
#define TSM_ALTOUT_CNT            0x0248
#elif defined(ST_5525) || defined(ST_5524)
#define TSM_ALTOUT_CNT            0x0440
#else /* 5528,5100,7710*/
#define TSM_ALTOUT_CNT            0x0250
#endif

/* Destination register addresses */
#define TSM_PTI0_DEST             0x0200
#if defined(ST_5528)
#define TSM_PTI1_DEST             0x0208
#elif defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#define TSM_PTI1_DEST             0x0210
#endif

#if defined(ST_7100)
#define TSM_P1394_DEST            0x0210
#elif defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#define TSM_P1394_DEST            0x0220
#else
#define TSM_P1394_DEST            0x0238
#endif

/* Additional config regs for altout & 1394 */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#define TSM_PTI_ALT_OUT_CFG       0x0800
#define TSM_P1394_CFG             0x0810
#else
#define TSM_PTI_ALT_OUT_CFG       0x0300
#define TSM_P1394_CFG             0x0338
#endif

/* System regs */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#define TSM_SYS_CFG               0x0820
#define TSM_SW_RESET              0x0830
#else /* 5528,5100,7710 */
#define TSM_SYS_CFG               0x03F0
#define TSM_SW_RESET              0x03F8
#endif

/* fix_disable register */
#if defined(ST_7100) || defined(ST_7109)
#define TSM_FIX_DISABLE           0x0840
#endif

/* new registers added from 7100 onwards */
#define TSM_WRITE_READ_PTR        0x0870
#define PTR_OFFSET                0x0010

/* Some constant values to be written into register */
#if defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#define TSIN_BYPASS               0x04
#define SWTS_BYPASS               0x06
#else
#define TSIN_BYPASS               0x02
#define SWTS_BYPASS               0x03
#endif

#if defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#define NORMAL_MODE_PTI0_BIT     2
#define NORMAL_MODE_PTI1_BIT     5
#define BYPASS_MODE_TSIN0_BIT    4
#define BYPASS_MODE_TSIN1_BIT    5
#define BYPASS_MODE_SWTS0_BIT    6
#define MASK_FOR_BYPASS          0x3F
#else
#define NORMAL_MODE_PTI0_BIT     0
#define NORMAL_MODE_PTI1_BIT     0
#define BYPASS_MODE_TSIN0_BIT    2
#define BYPASS_MODE_TSIN1_BIT    0
#define BYPASS_MODE_SWTS0_BIT    3
#define MASK_FOR_BYPASS          0x03
#endif

#if defined(STMERGE_DTV_PACKET)
#define TSIN_BYPASS_CFG           0xF0085
#define SWTS_BYPASS_CFG           0xF0085
#else
#define TSIN_BYPASS_CFG           0xF0087
#define SWTS_BYPASS_CFG           0xF0087
#endif

#define RESET                     0x0
#define MAX_PACE_VALUE            32768
#define MAX_CNT_VALUE             8388608
#define MAX_CNT_RATE              15
#define MAX_LOCK_DROP             15
#if !defined(ST_5528)
#define MAX_TRIGGER_VALUE         15
#endif

/* Bit manipulation constants*/
/* For TSM_STREAM_CFG_n register bits*/
#define PRIORITY_BIT              16
#define STREAM_ON_BIT             7
#define REPLACE_ID_TAG_BIT        6
#define ADD_TAG_BYTES_BIT         5
#define INVERT_BYTECLK_BIT        4
#define ASYNC_SOP_TOKEN_BIT       3
#define ALIGN_BYTE_SOP_BIT        2
#define SYNC_NOT_ASYNC_BIT        1
#define SERIAL_NOT_PARALLEL_BIT   0
#define RAM_ALLOC_START_BIT       8

#if defined(ST_5525) || defined(ST_5524)
#define COUNTER_BIT_27MHZ         24
#endif

/* For TSM_STREAM_SYNC_n register bits*/
#define SYNC_BIT                  0
#define DROP_BIT                  4
#define SOP_TOKEN_BIT             8
#define PACKET_LENGTH_BIT         16

/* For TSM_STREAM_STA_n register bits*/
#define STREAM_LOCK_BIT           0
#define INPUT_OVERFLOW_BIT        1
#define RAM_OVERFLOW_BIT          2
#define ERROR_PACKETS             0x00F8
#define COUNTER_VALUE             0xFFFFFF00

/* For TSM_STREAM_CFG2_n register bits */
#define CHANNEL_RST_BIT           0
#define MID_PKT_ERR_BIT           1
#define START_PKT_ERR_BIT         2
#define SHORT_PKT_COUNT_BIT       0x001F

/* For counter register bits */
#define CNT_VALUE_BIT             0
#define CNT_RATE_BIT              28
#define CNT_AUTO_BIT              24
#define CNT_RATE_MASK             0x0FFFFFFF

/* For swts_cfg register bits */
#define SWTS_PACE_BIT             0
#define SWTS_AUTO_PACE_BIT        16
#if !defined(ST_5528)
#define SWTS_REQ_TRIGGER_BIT      24
#endif

/* For PTI_ALTOUT register bits */
#define PTI_ALTOUT_PACE_BIT       0
#define PTI_ALTOUT_AUTO_PACE_BIT  16

#if defined(ST_5524) || defined(ST_5525) || defined(ST_7109)
#define PTI_ALTOUT_SELECT_BIT     24
#else
#define PTI_ALTOUT_SELECT_BIT     17
#endif

/* For 1394_cfg register bits */
#define P1394_PACE_BIT            0
#define P1394_REMOVE_TAGGING_BIT  18
#define P1394_DIR_OUT_NO_IN       17
#define P1394_CLKSRC              16

/* For TSM_RESET register values */
#define SW_RESET_CODE_VALUE           0x00000006

/* other constants */
#define ONE_TSIN_ONLY                 0xFFFFFFFE
#define SERIAL_NOT_PARALLEL           0x00000001

/* SRAM Configuration */
#define MAX_SRAM_ADDRESS              0x00001F00
#define MEMORY_MASK_VALUE             0x00001F00

#if defined(ST_5528)
#define DEFAULT_SRAM_SIZE_PER_STREAM  0x00000300
#elif defined(ST_5100) || defined(ST_7710) || defined(ST_5301) || defined(ST_7100) \
|| defined(ST_7109) || defined(ST_5525)
#define DEFAULT_SRAM_SIZE_PER_STREAM  0x00000400
#endif

#if defined(ST_7100) || defined(ST_7109)
#define READ_PTR_MASK   0x0000FFFF
#define WRITE_PTR_MASK  0xFFFF0000
#endif

#endif /* __MERGEREG_H */

