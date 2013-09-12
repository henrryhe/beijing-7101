/*****************************************************************************
File Name   : clkrvreg.h

Description : Defines Offsets for the Register Types and other constants

Copyright (C) 2004 STMicroelectronics

Reference   :

$ClearCase (VOB: stclkrv)

ST API Definition "Clock Recovery API"
*****************************************************************************/

#ifndef __CLKRVREG_H
#define __CLKRVREG_H

/***************************************************************************
There are major changes in 5100 and 7710 FS and clock recovery registers
this is why we have seperately defined the naming and offset etc.
****************************************************************************/


#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301) || defined (ST_5188)

/* REGISTER OFFSETS*/


/* FS Registers */
#define FSA_SETUP                0x010

#if defined (ST_5100) || defined (ST_5301)

#define CLK_SD_0_SETUP0          0x014
#define CLK_SD_0_SETUP1          0x018

#elif defined (ST_5188)

#define CLK_SD_0_SETUP0          0x310
#define CLK_SD_0_SETUP1          0x314

#else

#define CLK_SD_0_SETUP0          0x054
#define CLK_SD_0_SETUP1          0x058

#endif

#if defined (ST_5188)

#define CLK_PCM_0_SETUP0         0x320
#define CLK_PCM_0_SETUP1         0x324

#else

#define CLK_PCM_0_SETUP0         0x020
#define CLK_PCM_0_SETUP1         0x024

#endif

#if defined (ST_5188)

#define CLK_SPDIF_0_SETUP0       0x340
#define CLK_SPDIF_0_SETUP1       0x344

#else

#define CLK_SPDIF_0_SETUP0       0x030
#define CLK_SPDIF_0_SETUP1       0x034

#endif


/* Masks */
#define MD_MASK                  0x0000001F
#define SDIV_MASK                0x000001C0
#define IPE_MASK                 0x0000FFFF


/* "Freq synth normal use" & "Dig. Algo Works Normaly" & "O/P enabled" */
#if defined (ST_5105) || defined (ST_5107)
#define FS_OUTPUT_NORMAL         0x00000E20
#elif  defined (ST_5188)
#define FS_OUTPUT_NORMAL         0x00000A00
#else
#define FS_OUTPUT_NORMAL         0x00000E00
#endif

#elif defined (ST_5525)

/* REGISTER OFFSETS*/

/* FS Registers */
#define FSA_SETUP                0x010
#define FSB_SETUP                0x014
#define FSC_SETUP                0x018

/* "Freq synth normal use" & "Dig. Algo Works Normaly" & "O/P enabled" */
#define FS_OUTPUT_NORMAL         0x00000A00

#define CLK_SD_0_SETUP0          0x350
#define CLK_SD_0_SETUP1          0x354

#define CLK_SD_1_SETUP0          0x360
#define CLK_SD_1_SETUP1          0x364

#define CLK_PCM_0_SETUP0         0x310
#define CLK_PCM_0_SETUP1         0x314

#define CLK_PCM_1_SETUP0         0x320
#define CLK_PCM_1_SETUP1         0x324

#define CLK_PCM_2_SETUP0         0x330
#define CLK_PCM_2_SETUP1         0x334

#define CLK_PCM_3_SETUP0         0x340
#define CLK_PCM_3_SETUP1         0x344

#define CLK_SPDIF_0_SETUP0       0x370
#define CLK_SPDIF_0_SETUP1       0x374

/* Masks */
#define MD_MASK                  0x0000001F
#define SDIV_MASK                0x000001C0
#define IPE_MASK                 0x0000FFFF

#elif defined (ST_7710)

#define FS_0_CTRL                0x030
#define CLK_HD_0_SETUP0          0x04C
#define CLK_HD_0_SETUP1          0x050

#define FS_1_CTRL                0x060
#define CLK_SD_0_SETUP0          0x064
#define CLK_SD_0_SETUP1          0x068

#define FS_CLOCKGEN_CFG_0        0x0C0
#define FS_CLOCKGEN_CFG_1        0x0C4
#define FS_CLOCKGEN_CFG_2        0x0C8

#define EN_PRG_BIT_MASK          0x00000001
#define PE_BITS_MASK             0x0000FFFF
#define SDIV_BITS                3
#define MD_BITS                  6

#elif defined (ST_7100) || defined (ST_7109)

#define FS_CLOCKGEN_CFG_0        0x014
#define CLK_HD_0_MD1             0x018
#define CLK_HD_0_PE1             0x01c
#define CLK_HD_0_ENPRG1          0x020
#define CLK_HD_0_SDIV1           0x024

#define FS_CLOCKGEN_CFG_1        0x05c
#define CLK_SD_0_MD1             0x060
#define CLK_SD_0_PE1             0x064
#define CLK_SD_0_ENPRG1          0x068
#define CLK_SD_0_SDIV1           0x06c

#define CKGB_CLK_SRC             0x0a8
#define CKGB_DISP_CFG            0x0a4
#define CKG_CLK_EN               0x0b0

#define CKGB_REF_CLK_SRC         0x0b8

#define EN_PRG_BIT_MASK          0x00000001
#define PE_BITS_MASK             0x0000FFFF


#elif defined (ST_7200)

#define CLOCKGEN_SETUP_FS0       0x000
#define CLOCKGEN_SETUP_FS1       0x004

#define CLK_HD_0_CFG             0x00c
#define CLK_SD_0_CFG             0x01c

#define CLK_HD_1_CFG             0x010
#define CLK_SD_1_CFG             0x020

#define CKGB_REF_CLK_SRC         0x044
#define CKGB_OUT_MUX_CFG         0x048

/* Masks */
#define MD_MASK                  0x001F0000
#define SDIV_MASK                0x01C00000
#define EN_PRG_BIT_MASK          0x00200000
#define PE_BITS_MASK             0x0000FFFF

#endif

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301)

#define REF_MAX_CNT              0x160        /* DCO Ref Counter   */
#define CLK_REC_CMD              0x164        /* DCO CMD Register  */
#define CAPTURE_COUNTER_PCM      0x168        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_SPDIF    0x16C        /* DCO SPDIF Counter */
#define DCO_MODE_CFG             0x170        /* DCO Mode Cfg Register */


#define DEFAULT_VAL_FS_A_SETUP   0x04

#elif defined (ST_5188)

#define REF_MAX_CNT              0x600        /* DCO Ref Counter   */
#define CLK_REC_CMD              0x604        /* DCO CMD Register  */
#define CLK_REC_STATUS           0x608        /* DCO status Register  */
#define CAPTURE_COUNTER_PCM      0x610        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_SPDIF    0x614        /* DCO SPDIF Counter */
#define CAPTURE_COUNTER_AUX      0x60C        /* DCO SPDIF Counter */
#define DCO_MODE_CFG             0x170        /* DCO Mode Cfg Register */




#define UNLOCK_KEY               0x5AA50000

#elif defined (ST_5525)

#define REF_MAX_CNT              0x600        /* DCO Ref Counter   */
#define CLK_REC_CMD              0x604        /* DCO CMD Register  */
#define CLK_REC_STATUS           0x608        /* DCO status Register  */

#define DCO_MODE_CFG             0x170        /* DCO Mode Cfg Register */

#define REF_MAX_CNT_1            0x640        /* DCO Ref Counter   */
#define CLK_REC_CMD_1            0x644        /* DCO CMD Register  */
#define CLK_REC_STATUS_1         0x648        /* DCO status Register  */

#define CAPTURE_COUNTER_SPDIF    0x60C        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_PCM_0    0x610        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_PCM      CAPTURE_COUNTER_PCM_0
#define CAPTURE_COUNTER_PCM_1    0x614        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_PCM_2    0x618        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_PCM_3    0x61C        /* DCO PCM Counter   */

#define CAPTURE_COUNTER_1_SPDIF  0x64C        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_1_PCM_0  0x650        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_1_PCM    CAPTURE_COUNTER_1_PCM_0
#define CAPTURE_COUNTER_1_PCM_1  0x654        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_1_PCM_2  0x658        /* DCO PCM Counter   */
#define CAPTURE_COUNTER_1_PCM_3  0x65C        /* DCO PCM Counter   */

#define UNLOCK_KEY               0x5AA50000

#elif defined (ST_7710)

#define REF_MAX_CNT              0x0F0
#define CLK_REC_CMD              0x0F4
#define CAPTURE_COUNTER_PCM      0x0F8
#define CAPTURE_COUNTER_HD       0x0FC

/* OFFSET_DCO_MODE_CFG not needed EN_PRG bit will do the job */


#define ADSC_CKG_CFG             0x600
#define ADSC_CKG_MD              0x604
#define ADSC_CKG_PE              0x608
#define ADSC_CKG_SDIV            0x60C
#define ADSC_CKG_PSEL            0x610
#define ADSC_CKG_PROG            0x614
#define ADSC_IOCONTROL_PROG      0x700


#define UNLOCK_KEY               0x5AA50000

#elif defined (ST_7100) || defined (ST_7109)

#define REF_MAX_CNT              0x000
#define CLK_REC_CMD              0x004
#define CAPTURE_COUNTER_PCM      0x008
#define CAPTURE_COUNTER_HD       0x00C

#define AUD_FSYN_CFG             0x000
#define AUD_CKG_PCM0_MD          0x010
#define AUD_CKG_PCM0_PE          0x014
#define AUD_CKG_PCM0_SDIV        0x018
#define AUD_CKG_PCM0_PROG        0x01c

#define AUD_CKG_PCM1_MD          0x020
#define AUD_CKG_PCM1_PE          0x024
#define AUD_CKG_PCM1_SDIV        0x028
#define AUD_CKG_PCM1_PROG        0x02c

#define AUD_CKG_SPDIF_MD         0x030
#define AUD_CKG_SPDIF_PE         0x034
#define AUD_CKG_SPDIF_SDIV       0x038
#define AUD_CKG_SPDIF_PROG       0x03c

#define AUD_IOCONTROL_PROG       0x200

#elif defined (ST_7200)

#define REF_MAX_CNT              0x000
#define CLK_REC_CMD              0x004
#define CAPTURE_COUNTER_PCM      0x008
#define CAPTURE_COUNTER_HD       0x00C


#define REF_MAX_CNT_1            0x000        /* DCO Ref Counter   */
#define CLK_REC_CMD_1            0x004        /* DCO CMD Register  */
#define CAPTURE_COUNTER_PCM_1    0x008
#define CAPTURE_COUNTER_HD_1     0x00C

#define AUD_FSYNA_CFG            0x000
#define AUD_CKG_PCM0_MD          0x010
#define AUD_CKG_PCM0_PE          0x014
#define AUD_CKG_PCM0_SDIV        0x018
#define AUD_CKG_PCM0_PROG        0x01c

#define AUD_CKG_PCM1_MD          0x020
#define AUD_CKG_PCM1_PE          0x024
#define AUD_CKG_PCM1_SDIV        0x028
#define AUD_CKG_PCM1_PROG        0x02c

#define AUD_CKG_PCM2_MD          0x030
#define AUD_CKG_PCM2_PE          0x034
#define AUD_CKG_PCM2_SDIV        0x038
#define AUD_CKG_PCM2_PROG        0x03c

#define AUD_CKG_PCM3_MD          0x040
#define AUD_CKG_PCM3_PE          0x044
#define AUD_CKG_PCM3_SDIV        0x048
#define AUD_CKG_PCM3_PROG        0x04c

#define AUD_FSYNB_CFG            0x100
#define AUD_CKG_HDMI_MD          0x130
#define AUD_CKG_HDMI_PE          0x134
#define AUD_CKG_HDMI_SDIV        0x138
#define AUD_CKG_HDMI_PROG        0x13c

#define AUD_CKG_SPDIF_MD         0x140
#define AUD_CKG_SPDIF_PE         0x144
#define AUD_CKG_SPDIF_SDIV       0x148
#define AUD_CKG_SPDIF_PROG       0x14c

#define AUD_IOCONTROL_PROG       0x200

#endif

/* Modifications for 5525 dual support */

#ifdef ST_7200
#define CLKRV_REF_MAX_CNT(x)    ((x)?REF_MAX_CNT:REF_MAX_CNT)
#define CLKRV_REC_CMD(x)        ((x)?CLK_REC_CMD:CLK_REC_CMD)
#else
#define CLKRV_REF_MAX_CNT(x)    ((x)?(REF_MAX_CNT + 0x40):REF_MAX_CNT)
#define CLKRV_REC_CMD(x)        ((x)?(CLK_REC_CMD + 0x40):CLK_REC_CMD)
#endif

#if defined (ST_5525) || defined (ST_5188)
#define CLKRV_REC_STATUS(x)   ((x)?(CLK_REC_STATUS + 0x40):CLK_REC_STATUS)
#endif

#define CLKRV_CAPTURE_COUNTER_SPDIF(x)  ((x)?CAPTURE_COUNTER_1_SPDIF : CAPTURE_COUNTER_SPDIF)
#define CLKRV_CAPTURE_COUNTER_PCM_0(x)  ((x)?CAPTURE_COUNTER_1_PCM_0 : CAPTURE_COUNTER_PCM_0)
#define CLKRV_CAPTURE_COUNTER_PCM(x)    ((x)?CAPTURE_COUNTER_1_PCM_0 : CAPTURE_COUNTER_PCM)
#define CLKRV_CAPTURE_COUNTER_PCM_1(x)  ((x))?CAPTURE_COUNTER_1_PCM_1: CAPTURE_COUNTER_PCM_1)
#define CLKRV_CAPTURE_COUNTER_PCM_2(x)  ((x))?CAPTURE_COUNTER_1_PCM_2: CAPTURE_COUNTER_PCM_2)
#define CLKRV_CAPTURE_COUNTER_PCM_3(x)  ((x))?CAPTURE_COUNTER_1_PCM_3: CAPTURE_COUNTER_PCM_3)


#define ASSERT_1                      0x01
#define ASSERT_0                      0x00

#define COMPLEMENT2S_FOR_5BIT_VALUE   32
#define CLKRVSIM_REG_ARRAY_SIZE       0x16C

#endif  /* #ifndef __CLKRVREG_H */
