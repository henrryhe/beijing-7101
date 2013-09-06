/************************************************************************
File Name   : sti7109.h

Description : System-wide devices header file for the STi7109.
              These definitions are CPU specific.

Copyright (C) 2005 STMicroelectronics

Reference   :

************************************************************************/

#ifndef __STI71XX_H
#define __STI71XX_H

#include "chiprevision.h"

#ifdef ST_OS21
#include <os21.h>
#include <os21/st40/stb7109.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ST40 INTERRUPTS  -------------------------------------------------------- */

/* Base address */

/* Interrupt quantities */
#define ST71XX_NUM_INTERRUPTS                 100   /* Total No. of interrupts */
#define ST71XX_NUM_INTERRUPT_LEVELS           16


#if defined(ST_OS21)

/* Interrupt numbers */
#define ST71XX_PIO0_INTERRUPT                 OS21_INTERRUPT_PIO_0
#define ST71XX_PIO1_INTERRUPT                 OS21_INTERRUPT_PIO_1
#define ST71XX_PIO2_INTERRUPT                 OS21_INTERRUPT_PIO_2
#define ST71XX_PIO3_INTERRUPT                 OS21_INTERRUPT_PIO_3
#define ST71XX_PIO4_INTERRUPT                 OS21_INTERRUPT_PIO_4
#define ST71XX_PIO5_INTERRUPT                 OS21_INTERRUPT_PIO_5
#define ST71XX_ASC0_INTERRUPT                 OS21_INTERRUPT_UART_0
#define ST71XX_ASC1_INTERRUPT                 OS21_INTERRUPT_UART_1
#define ST71XX_ASC2_INTERRUPT                 OS21_INTERRUPT_UART_2
#define ST71XX_ASC3_INTERRUPT                 OS21_INTERRUPT_UART_3
#define ST71XX_SSC0_INTERRUPT                 OS21_INTERRUPT_SSC_0
#define ST71XX_SSC1_INTERRUPT                 OS21_INTERRUPT_SSC_1
#define ST71XX_SSC2_INTERRUPT                 OS21_INTERRUPT_SSC_2
#define ST71XX_DISEQC0_INTERRUPT              OS21_INTERRUPT_DISEQ
#define ST71XX_IRB_INTERRUPT                  OS21_INTERRUPT_IRB
#define ST71XX_TELETEXT_INTERRUPT             OS21_INTERRUPT_TTXT
#define ST71XX_BLITTER_INTERRUPT              OS21_INTERRUPT_BLITTER
#define ST71XX_DAA_INTERRUPT                  OS21_INTERRUPT_DAA
#define ST71XX_PWM_A_INTERRUPT                OS21_INTERRUPT_PWM
#define ST71XX_TSMERGE_INTERRUPT              OS21_INTERRUPT_TSM_RAM_OVF
#define ST71XX_MAFE_INTERRUPT                 OS21_INTERRUPT_MAFE
#define ST71XX_FDMA_MAILBOX_FLAG_INTERRUPT    OS21_INTERRUPT_FDMA_MBOX
#define ST71XX_FDMA_GP_OUTPUTS_INTERRUPT      OS21_INTERRUPT_FDMA_GP0
#define ST71XX_PCM_PLAYER_0_INTERRUPT         OS21_INTERRUPT_AUD_PCM_PLYR0
#define ST71XX_PCM_PLAYER_1_INTERRUPT         OS21_INTERRUPT_AUD_PCM_PLYR1
#define ST71XX_PCM_READER_INTERRUPT           OS21_INTERRUPT_AUD_PCM_RDR
#define ST71XX_SPDIF_PLAYER_INTERRUPT         OS21_INTERRUPT_AUD_SPDIF_PLYR
#define ST71XX_SATA_INTERRUPT                 OS21_INTERRUPT_SATA
#define ST71XX_MB_LX_AUDIO_INTERRUPT          OS21_INTERRUPT_MB_LX_AUDIO    
#define ST71XX_MB_LX_DPHI_INTERRUPT           OS21_INTERRUPT_MB_LX_DPHI     
#define ST71XX_AUD_CPXM _INTERRUPT            OS21_INTERRUPT_AUD_CPXM       
#define ST71XX_AUD_I2S2SPDIF_INTERRUPT        OS21_INTERRUPT_AUD_I2S2SPDIF 
#define ST71XX_VID_DPHI_MBE_INTERRUPT         OS21_INTERRUPT_VID_DPHI_MBE   
#define ST71XX_VID_DPHI_PRE1_INTERRUPT        OS21_INTERRUPT_VID_DPHI_PRE1  
#define ST71XX_VID_DPHI_PRE0_INTERRUPT        OS21_INTERRUPT_VID_DPHI_PRE0  
#define ST71XX_DVP_INTERRUPT                  OS21_INTERRUPT_DVP
#define ST71XX_USB_EHCI_INTERRUPT             OS21_INTERRUPT_USBH_EHCI
#define ST71XX_USB_OHCI_INTERRUPT             OS21_INTERRUPT_USBH_OHCI
#define ST71XX_HDCP_INTERRUPT                 OS21_INTERRUPT_HDCP
#define ST71XX_VTG_0_INTERRUPT                OS21_INTERRUPT_VTG_1
#define ST71XX_VTG_1_INTERRUPT                OS21_INTERRUPT_VTG_2
#define ST71XX_VDP_END_PROCESSING_INTERRUPT   OS21_INTERRUPT_VDP_END_PROC
#define ST71XX_VDP_FIFO_EMPTY_INTERRUPT       OS21_INTERRUPT_VDP_FIFO_EMPTY
#define ST71XX_HDMI_INTERRUPT                 OS21_INTERRUPT_HDMI
#define ST71XX_PTI_INTERRUPT                  OS21_INTERRUPT_PTI
#if 1
#define ST71XX_PTIA_INTERRUPT                 OS21_INTERRUPT_PTI_0
#define ST71XX_PTIB_INTERRUPT                 OS21_INTERRUPT_PTI_1
#else
#define ST71XX_PTIA_INTERRUPT                 OS21_INTERRUPT_PTI
#define ST71XX_PTIB_INTERRUPT                 OS21_INTERRUPT_PTI
#endif

#define ST71XX_DES_INTERRUPT                  OS21_INTERRUPT_PDES
#define ST71XX_GLH_INTERRUPT                  OS21_INTERRUPT_VID_GLH
#define ST71XX_MPEG_CLK_REC_INTERRUPT         OS21_INTERRUPT_CKG_DCXO
#define ST71XX_IR_WAKE_UP_INTERRUPT           OS21_INTERRUPT_IRB_WAKEUP
#define ST71XX_EXT_INT_IN0_INTERRUPT          OS21_INTERRUPT_IRL_0
#define ST71XX_EXT_INT_IN1_INTERRUPT          OS21_INTERRUPT_IRL_1
#define ST71XX_EXT_INT_IN2_INTERRUPT          OS21_INTERRUPT_IRL_2
#define ST71XX_EXT_INT_IN3_INTERRUPT          OS21_INTERRUPT_IRL_3

#elif defined(ST_OSLINUX)

/* from arch/sh/kernel/cpu/sh4/irq_intc2.c */

/*For all SH4 INTEVT interrupts, the number (x) given in the datasheet should be converted to y as follows before being used:
y = (x - 512) / 32
*/

#define ST71XX_PIO0_INTERRUPT                 80
#define ST71XX_PIO1_INTERRUPT                 84
#define ST71XX_PIO2_INTERRUPT                 88
#define ST71XX_PIO3_INTERRUPT                 115
#define ST71XX_PIO4_INTERRUPT                 114
#define ST71XX_PIO5_INTERRUPT                 113
#define ST71XX_ASC0_INTERRUPT                 123
#define ST71XX_ASC1_INTERRUPT                 122
#define ST71XX_ASC2_INTERRUPT                 121
#define ST71XX_ASC3_INTERRUPT                 120
#define ST71XX_SSC0_INTERRUPT                 119
#define ST71XX_SSC1_INTERRUPT                 118
#define ST71XX_SSC2_INTERRUPT                 117
#define ST71XX_DISEQC0_INTERRUPT              129
#define ST71XX_IRB_INTERRUPT                  125
#define ST71XX_TELETEXT_INTERRUPT             131
#define ST71XX_BLITTER_INTERRUPT              156
#define ST71XX_DAA_INTERRUPT                  130
#define ST71XX_PWM_A_INTERRUPT                126
#define ST71XX_TSMERGE_INTERRUPT              134
#define ST71XX_MAFE_INTERRUPT                 127
#define ST71XX_FDMA_MAILBOX_FLAG_INTERRUPT    140
#define ST71XX_FDMA_GP_OUTPUTS_INTERRUPT      141
#define ST71XX_PCM_PLAYER_0_INTERRUPT         144
#define ST71XX_PCM_PLAYER_1_INTERRUPT         145
#define ST71XX_PCM_READER_INTERRUPT           146
#define ST71XX_SPDIF_PLAYER_INTERRUPT         147
#define ST71XX_SATA_INTERRUPT                 170
#define ST71XX_MB_LX_AUDIO_INTERRUPT          137
#define ST71XX_MB_LX_DPHI_INTERRUPT           136
#define ST71XX_AUD_CPXM_INTERRUPT             143
#define ST71XX_AUD_I2S2SPDIF_INTERRUPT        142
#define ST71XX_VID_DPHI_MBE_INTERRUPT         151
#define ST71XX_VID_DPHI_PRE1_INTERRUPT        150
#define ST71XX_VID_DPHI_PRE0_INTERRUPT        149
#define ST71XX_DVP_INTERRUPT                  157
#define ST71XX_USB_EHCI_INTERRUPT             169
#define ST71XX_USB_OHCI_INTERRUPT             168
#define ST71XX_HDCP_INTERRUPT                 159
#define ST71XX_VTG_0_INTERRUPT                154
#define ST71XX_VTG_1_INTERRUPT                155
#define ST71XX_VDP_END_PROCESSING_INTERRUPT   153
#define ST71XX_VDP_FIFO_EMPTY_INTERRUPT       152
#define ST71XX_HDMI_INTERRUPT                 158
#define ST71XX_PTI_INTERRUPT                  160
#define ST71XX_PTIA_INTERRUPT                 160 /*same as _PTI_*/
#define ST71XX_PTIB_INTERRUPT                 139
#define ST71XX_DES_INTERRUPT                  162
#define ST71XX_GLH_INTERRUPT                  148
#define ST71XX_MPEG_CLK_REC_INTERRUPT         138 /*dcxo*/
#define ST71XX_IR_WAKE_UP_INTERRUPT           124

#endif


/* These devices appear at the same physical address to both CPUs, though
   access from ST40 may need some control bits set */

#define ST40_CACHE_TRANSLATE(addr) (addr)                           /* P0 */
#define ST40_CACHE_NOTRANSLATE(addr) ((U32)(addr) | 0x80000000)     /* P1 */
#define ST40_NOCACHE_NOTRANSLATE(addr) ((U32)(addr) | 0xA0000000)   /* P2 */
/* P3 seems to duplicate P0; P4 only used to access ST40 devices,
   where it has been included in the basic address */

#ifndef NATIVE_CORE
#define tlm_to_virtual(add) add    
#endif


/* SYSTEM DEVICES (in alphabetical order) ---------------------------------- */

#define ST71XX_ILC_BASE_ADDRESS               tlm_to_virtual(0xB8000000)

/* ASC --------------------------------------------------------------------- */

#define ST71XX_ASC0_BASE_ADDRESS              tlm_to_virtual(0xB8030000)
#define ST71XX_ASC1_BASE_ADDRESS              tlm_to_virtual(0xB8031000)
#define ST71XX_ASC2_BASE_ADDRESS              tlm_to_virtual(0xB8032000)
#define ST71XX_ASC3_BASE_ADDRESS              tlm_to_virtual(0xB8033000)

/* AUDIO ------------------------------------------------------------------- */

/* Interface Config - really the base address for all audio register access */
#define ST71XX_AUDIO_IF_BASE_ADDRESS          tlm_to_virtual(0xB9210000) /*ADSC*/

/* BLITTER ----------------------------------------------------------------- */

#define ST71XX_BLITTER_BASE_ADDRESS           tlm_to_virtual(0xB920B000)

/* CACHE ------------------------------------------------------------------- */

#define ST71XX_ICACHE_BASE_ADDRESS            tlm_to_virtual(0x0) /*not used, just kept for backward compatability*/
#define ST71XX_DCACHE_BASE_ADDRESS            tlm_to_virtual(0x0) /*not used, just kept for backward compatability*/
#define ST71XX_CACHE_BASE_ADDRESS             tlm_to_virtual(ST71XX_ICACHE_BASE_ADDRESS) /*not used, just kept for backward compatability*/

/* CFG --------------------------------------------------------------------- */

/* system status & config registers */
#define ST71XX_CFG_BASE_ADDRESS               tlm_to_virtual(0xB9001000) /*SysConf registers*/

/*yxl 2007-05-25 add below line*/
#define ST7109_CFG_BASE_ADDRESS               ST71XX_CFG_BASE_ADDRESS 


/* STFAE - Add system registers */
#define    STB71XX_device_id          (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x000))
#define    STB71XX_extra_device_id    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x004))
#define    STB71XX_system_status0     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x008))
#define    STB71XX_system_status1     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x00c))
#define    STB71XX_system_status2     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x010))
#define    STB71XX_system_status3     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x014))
#define    STB71XX_system_status4     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x018))
#define    STB71XX_system_status5     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x01c))
#define    STB71XX_system_status6     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x020))
#define    STB71XX_system_status7     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x024))
#define    STB71XX_system_status8     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x028))
#define    STB71XX_system_status9     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x02c))
#define    STB71XX_system_status10    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x030))
#define    STB71XX_system_status11    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x034))
#define    STB71XX_system_status12    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x038))
#define    STB71XX_system_status13    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x03c))
#define    STB71XX_system_status14    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x040))
#define    STB71XX_system_config0     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x100))
#define    STB71XX_system_config1     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x104))
#define    STB71XX_system_config2     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x108))
#define    STB71XX_system_config3     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x10c))
#define    STB71XX_system_config4     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x110))
#define    STB71XX_system_config5     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x114))
#define    STB71XX_system_config6     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x118))
#define    STB71XX_system_config7     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x11c))
#define    STB71XX_system_config8     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x120))
#define    STB71XX_system_config9     (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x124))
#define    STB71XX_system_config10    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x128))
#define    STB71XX_system_config11    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x12c))
#define    STB71XX_system_config12    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x130))
#define    STB71XX_system_config13    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x134))
#define    STB71XX_system_config14    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x138))
#define    STB71XX_system_config15    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x13c))
#define    STB71XX_system_config16    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x140))
#define    STB71XX_system_config17    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x144))
#define    STB71XX_system_config18    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x148))
#define    STB71XX_system_config19    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x14c))
#define    STB71XX_system_config20    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x150))
#define    STB71XX_system_config21    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x154))
#define    STB71XX_system_config22    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x158))
#define    STB71XX_system_config23    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x15c))
#define    STB71XX_system_config24    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x160))
#define    STB71XX_system_config25    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x164))
#define    STB71XX_system_config26    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x168))
#define    STB71XX_system_config27    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x16c))
#define    STB71XX_system_config28    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x170))
#define    STB71XX_system_config29    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x174))
#define    STB71XX_system_config30    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x178))
#define    STB71XX_system_config31    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x17c))
#define    STB71XX_system_config32    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x180))
#define    STB71XX_system_config33    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x184))
#define    STB71XX_system_config34    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x188))
#define    STB71XX_system_config35    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x18c))
#define    STB71XX_system_config36    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x190))
#define    STB71XX_system_config37    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x194))
#define    STB71XX_system_config38    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x198))
#define    STB71XX_system_config39    (*(DU32*) (ST71XX_CFG_BASE_ADDRESS + 0x19c))

/* interconnect node control registers */
#define ST71XX_IC_BASE_ADDRESS                tlm_to_virtual(0xB920D000)

/* CLOCK GENERATOR --------------------------------------------------------- */

#define ST71XX_CKG_BASE_ADDRESS               tlm_to_virtual(0xB9000000) /*ST71XX_CKG_B_BASE_ADDRESS*/
#define ST71XX_CKG_A_BASE_ADDRESS             tlm_to_virtual(0xB9213000)
#define ST71XX_CKG_B_BASE_ADDRESS             tlm_to_virtual(0xB9000000)
#define ST71XX_CKG_C_BASE_ADDRESS             tlm_to_virtual(0xB9210000)

/* COMPOSITOR -------------------------------------------------------------- */

/* base address for whole compositor block */
#define ST71XX_COMPOSITOR_BASE_ADDRESS        tlm_to_virtual(0xB920A000)

/* base addresses for compositor subblocks */
#define ST71XX_CURSOR_LAYER_BASE_ADDRESS      tlm_to_virtual(0xB920A000)
#define ST71XX_GDP1_LAYER_BASE_ADDRESS        tlm_to_virtual(0xB920A100)
#define ST71XX_GDP2_LAYER_BASE_ADDRESS        tlm_to_virtual(0xB920A200)
#define ST71XX_GDP3_LAYER_BASE_ADDRESS        tlm_to_virtual(0xB920A300)
#define ST71XX_GDP4_LAYER_BASE_ADDRESS        tlm_to_virtual(0xB920A400)
#define ST71XX_ALP_LAYER_BASE_ADDRESS         tlm_to_virtual(0xB920A600) /* alpha plane */
#define ST71XX_VID1_LAYER_BASE_ADDRESS        tlm_to_virtual(0xB920A700)
#define ST71XX_VID2_LAYER_BASE_ADDRESS        tlm_to_virtual(0xB920A800)
#define ST71XX_VMIX1_BASE_ADDRESS             tlm_to_virtual(0xB920AC00)
#define ST71XX_VMIX2_BASE_ADDRESS             tlm_to_virtual(0xB920AD00)

/* DENC -------------------------------------------------------------------- */

/* DENC registers - part of video output stage register block */
#define ST71XX_DENC_BASE_ADDRESS              tlm_to_virtual(0xB920C000)

/* DISEQC ------------------------------------------------------------------ */
#define ST71XX_DISEQC_0_BASE_ADDRESS          tlm_to_virtual(0xB8068000)

/* DISP -------------------------------------------------------------------- */

/* Video display registers */
#define ST71XX_DISP0_BASE_ADDRESS             tlm_to_virtual(0xB9002000)
#define ST71XX_DISP1_BASE_ADDRESS             tlm_to_virtual(0xB9003000)

#define ST71XX_CRC_LUMA_BASE_ADDRESS          tlm_to_virtual(0xB9002400)
#define ST71XX_CRC_CHROMA_BASE_ADDRESS        tlm_to_virtual(0xB9002404)


/* DVO --------------------------------------------------------------------- */

/* digital video output registers - part of video output stage reg block */
#define ST71XX_DVO_BASE_ADDRESS               tlm_to_virtual(0xB9005000)

/* DVP --------------------------------------------------------------------- */

/* digital video input port registers */
#define ST71XX_DVP_BASE_ADDRESS               tlm_to_virtual(0xB9241000)

/* EMI --------------------------------------------------------------------- */

/* generic EMI block registers */
#define ST71XX_EMI_BASE_ADDRESS               tlm_to_virtual(0xBA100000)

/* EMI buffer registers (for bank boundaries) */
#define ST71XX_EMI_BUFFER_BASE_ADDRESS        tlm_to_virtual(0xBA100800)

/* actual bus window, as visible from ST40 P2 region and providing 64MB address space */
#define ST71XX_EMI_BUS_BASE                   tlm_to_virtual(0xA0000000)

/* FDMA ------------------------------------------------------------------- */

#define ST71XX_FDMA_BASE_ADDRESS              tlm_to_virtual(0xB9220000)

/* FUSE ------------------------------------------------------------------- */
#define ST71XX_FUSE_BASE_ADDRESS              tlm_to_virtual(0xB9208000)

/* HDCP ------------------------------------------------------------------- */

#define ST71XX_HDCP_BASE_ADDRESS              tlm_to_virtual(0xB9005400)

/* HDMI ------------------------------------------------------------------- */

#define ST71XX_HDMI_BASE_ADDRESS              tlm_to_virtual(0xB9005800)

/* IRBLASTER --------------------------------------------------------------- */

#define ST71XX_IRB_BASE_ADDRESS               tlm_to_virtual(0xB8018000)

/* LMI --------------------------------------------------------------------- */

/* LMI control registers */
#define ST71XX_LMI_CONFIG_BASE_ADDRESS        tlm_to_virtual(0xAF000000) /*LMI_SYS*/

/* actual memory window, as visible from ST40 P2 region and 176 MB (256 MB from cut2) address space */
#define ST71XX_LMI_MEMORY_BASE                tlm_to_virtual(0xA4000000) /*LMI_SYS*/

/* LMI_VID control registers */
#define ST71XX_LMI_VID_CONFIG_BASE_ADDRESS    tlm_to_virtual(0xB7000000) /*LMI_VID*/

/* actual memory window, as visible from ST40 P2 region and 112 MB (128MB from cut2) address space */
#define ST71XX_LMI_VID_MEMORY_BASE            tlm_to_virtual(0xB0000000) /*LMI_VID*/

/* LPC --------------------------------------------------------------------- */

#define ST71XX_LPC_BASE_ADDRESS               tlm_to_virtual(0xB8008000)

/* MAFE -------------------------------------------------------------------- */

#define ST71XX_MAFE_BASE_ADDRESS              tlm_to_virtual(0xB8058000)

/* MAILBOX ----------------------------------------------------------------- */

#define ST71XX_MAILBOX_BASE_ADDRESS           tlm_to_virtual(0xB9211000) /*video*/
#define ST71XX_MAILBOX_0_BASE_ADDRESS         tlm_to_virtual(0xB9211000) /*video*/
#define ST71XX_MAILBOX_1_BASE_ADDRESS         tlm_to_virtual(0xB9212000) /*audio*/

/* PCM PLAYER -------------------------------------------------------------- */

#define ST71XX_PCM_PLAYER_BASE_ADDRESS        tlm_to_virtual(0xB8101000)
#define ST71XX_PCM_PLAYER_0_BASE_ADDRESS      tlm_to_virtual(ST71XX_PCM_PLAYER_BASE_ADDRESS)
#define ST71XX_PCM_PLAYER_1_BASE_ADDRESS      tlm_to_virtual(0xB8101800)

/* PCM READER -------------------------------------------------------------- */

#define ST71XX_PCM_READER_BASE_ADDRESS        tlm_to_virtual(0xB8102000)


/* PCP (DES) --------------------------------------------------------------- */

#define ST71XX_PCP_BASE_ADDRESS               tlm_to_virtual(0xB9200000)

/* PIO --------------------------------------------------------------------- */

#define ST71XX_PIO0_BASE_ADDRESS              tlm_to_virtual(0xB8020000)
#define ST71XX_PIO1_BASE_ADDRESS              tlm_to_virtual(0xB8021000)
#define ST71XX_PIO2_BASE_ADDRESS              tlm_to_virtual(0xB8022000)
#define ST71XX_PIO3_BASE_ADDRESS              tlm_to_virtual(0xB8023000)
#define ST71XX_PIO4_BASE_ADDRESS              tlm_to_virtual(0xB8024000)
#define ST71XX_PIO5_BASE_ADDRESS              tlm_to_virtual(0xB8025000)

/* PTI --------------------------------------------------------------------- */

#define ST71XX_PTI_BASE_ADDRESS               tlm_to_virtual(0xB9230000)
#define ST71XX_PTIA_BASE_ADDRESS              tlm_to_virtual(0xB9230000)
#define ST71XX_PTIB_BASE_ADDRESS              tlm_to_virtual(0xB9260000) /*only from 71XX cut 2*/

/* PWM --------------------------------------------------------------------- */

#define ST71XX_PWM_BASE_ADDRESS               tlm_to_virtual(0xB8010000)

/* SATA -------------------------------------------------------------------- */

#define ST71XX_SATA_BASE_ADDRESS              tlm_to_virtual(0xB9209000)

/* SMARTCARD clock generators ---------------------------------------------- */

#define ST71XX_SC0_BASE_ADDRESS               tlm_to_virtual(0xB8048000)
#define ST71XX_SC1_BASE_ADDRESS               tlm_to_virtual(0xB8049000)

/* SPDIF PLAYER ------------------------------------------------------------ */

#define ST71XX_SPDIF_PLAYER_BASE_ADDRESS      tlm_to_virtual(0xB8103000)

/* SSC (I2C) --------------------------------------------------------------- */

#define ST71XX_SSC0_BASE_ADDRESS              tlm_to_virtual(0xB8040000)
#define ST71XX_SSC1_BASE_ADDRESS              tlm_to_virtual(0xB8041000)
#define ST71XX_SSC2_BASE_ADDRESS              tlm_to_virtual(0xB8042000)

/* SCIF -------------------------------------------------------------------- */

#define ST71XX_SCIF_BASE_ADDRESS			  tlm_to_virtual(0xFFE80000)


/* TSMERGE ----------------------------------------------------------------- */

#define ST71XX_TSMERGE_BASE_ADDRESS           tlm_to_virtual(0xB9242000) /*address of tsmerge config*/
#define ST71XX_TSMERGE1_BASE_ADDRESS          tlm_to_virtual(0xB9242000) /*address of tsmerge config*/
#define ST71XX_TSMERGE2_BASE_ADDRESS          tlm_to_virtual(0xBA300000) /*address of tsmerge SWTS data*/

/* TTX --------------------------------------------------------------------- */

/* teletext dma engines */
#define ST71XX_TTX_BASE_ADDRESS               tlm_to_virtual(0xB8060000)

/* USB --------------------------------------------------------------------- */

#define ST71XX_USB_BASE_ADDRESS               tlm_to_virtual(0xB9100000)

/* VIDEO ------------------------------------------------------------------- */

/* video decoders - note some regs only appear in the lower block */
#define ST71XX_VIDEO_BASE_ADDRESS             tlm_to_virtual(0xB9240000)
#define ST71XX_VID1_BASE_ADDRESS              tlm_to_virtual(0xB9240000)

#define ST71XX_DELTA_BASE_ADDRESS             tlm_to_virtual(0xB9540000)

/* VMIX -------------------------------------------------------------------- */

/* see COMPOSITOR addresses above - MIX1/2 */

/* VOUT -------------------------------------------------------------------- */

/* base address of whole video output stage register block
  - see DENC, VTG, DVO sections */
#define ST71XX_VOS_BASE_ADDRESS               tlm_to_virtual(0xB9005000)

/* miscellaneous video output registers */
#define ST71XX_VOUT_BASE_ADDRESS              tlm_to_virtual(0xB9005000)

/* VTG --------------------------------------------------------------------- */

/* this is part of the video output stage register block */
#define ST71XX_VTG_BASE_ADDRESS               tlm_to_virtual(0xB9005000)
#define ST71XX_VTG1_BASE_ADDRESS              tlm_to_virtual(0xB9005000)
#define ST71XX_VTG2_BASE_ADDRESS              tlm_to_virtual(0xB9005034)



/*****************************************************************************
             WRAPPER LAYER FOR BACKWARDS-COMPATIBILITY ONLY
*****************************************************************************/

#if !defined (REMOVE_GENERIC_ADDRESSES) || defined(USE_71XX_GENERIC_ADDRESSES)


/* INTERRUPT CONTROLLER ---------------------------------------------------- */

/* Interrupt quantities */
#ifndef INTERRUPT_NUMBERS
#define INTERRUPT_NUMBERS               ST71XX_NUM_INTERRUPTS
#endif
#ifndef INTERRUPT_LEVELS
#define INTERRUPT_LEVELS                ST71XX_NUM_INTERRUPT_LEVELS
#endif

/* Externally sourced interrupt numbers (cell interrupts covered later) */
#ifndef EXTERNAL_0_INTERRUPT
#define EXTERNAL_0_INTERRUPT            ST71XX_EXT_INT_IN0_INTERRUPT
#endif
#ifndef EXTERNAL_1_INTERRUPT
#define EXTERNAL_1_INTERRUPT            ST71XX_EXT_INT_IN1_INTERRUPT
#endif
#ifndef EXTERNAL_2_INTERRUPT
#define EXTERNAL_2_INTERRUPT            ST71XX_EXT_INT_IN2_INTERRUPT
#endif
#ifndef EXTERNAL_3_INTERRUPT
#define EXTERNAL_3_INTERRUPT            ST71XX_EXT_INT_IN3_INTERRUPT
#endif

/* SYSTEM DEVICES (in alphabetical order) ---------------------------------- */
#ifndef ILC_BASE_ADDRESS
#define ILC_BASE_ADDRESS                ST71XX_ILC_BASE_ADDRESS
#endif

/* ASC --------------------------------------------------------------------- */

/* Base addresses */
#ifndef ASC_0_BASE_ADDRESS
#define ASC_0_BASE_ADDRESS              ST71XX_ASC0_BASE_ADDRESS
#endif
#ifndef ASC_1_BASE_ADDRESS
#define ASC_1_BASE_ADDRESS              ST71XX_ASC1_BASE_ADDRESS
#endif
#ifndef ASC_2_BASE_ADDRESS
#define ASC_2_BASE_ADDRESS              ST71XX_ASC2_BASE_ADDRESS
#endif
#ifndef ASC_3_BASE_ADDRESS
#define ASC_3_BASE_ADDRESS              ST71XX_ASC3_BASE_ADDRESS
#endif

/* Interrupt numbers */
#ifndef ASC_0_INTERRUPT
#define ASC_0_INTERRUPT                 ST71XX_ASC0_INTERRUPT
#endif
#ifndef ASC_1_INTERRUPT
#define ASC_1_INTERRUPT                 ST71XX_ASC1_INTERRUPT
#endif
#ifndef ASC_2_INTERRUPT
#define ASC_2_INTERRUPT                 ST71XX_ASC2_INTERRUPT
#endif
#ifndef ASC_3_INTERRUPT
#define ASC_3_INTERRUPT                 ST71XX_ASC3_INTERRUPT
#endif

/* AUDIO ------------------------------------------------------------------- */

/* Base address for interface configuration */
#ifndef AUDIO_IF_BASE_ADDRESS
#define AUDIO_IF_BASE_ADDRESS           ST71XX_AUDIO_IF_BASE_ADDRESS
#endif

/* BLITTER ----------------------------------------------------------------- */

#ifndef BLITTER_BASE_ADDRESS
#define BLITTER_BASE_ADDRESS            ST71XX_BLITTER_BASE_ADDRESS
#endif

#ifndef BLITTER_INTERRUPT
#define BLITTER_INTERRUPT               ST71XX_BLITTER_INTERRUPT
#endif

/* CACHE ------------------------------------------------------------------- */

#ifndef CACHE_BASE_ADDRESS
#define CACHE_BASE_ADDRESS              ST71XX_CACHE_BASE_ADDRESS
#endif

#ifndef ICACHE_BASE_ADDRESS
#define ICACHE_BASE_ADDRESS             ST71XX_ICACHE_BASE_ADDRESS
#endif

#ifndef DCACHE_BASE_ADDRESS
#define DCACHE_BASE_ADDRESS             ST71XX_DCACHE_BASE_ADDRESS
#endif

/* CFG --------------------------------------------------------------------- */

#ifndef CFG_BASE_ADDRESS
#define CFG_BASE_ADDRESS                ST71XX_CFG_BASE_ADDRESS
#endif

/* CLOCK GENERATOR --------------------------------------------------------- */

#ifndef CKG_BASE_ADDRESS
#define CKG_BASE_ADDRESS                ST71XX_CKG_BASE_ADDRESS
#endif
#ifndef CKG_B_BASE_ADDRESS
#define CKG_B_BASE_ADDRESS              ST71XX_CKG_B_BASE_ADDRESS
#endif
#ifndef CKG_A_BASE_ADDRESS
#define CKG_A_BASE_ADDRESS              ST71XX_CKG_A_BASE_ADDRESS
#endif
#ifndef CKG_C_BASE_ADDRESS
#define CKG_C_BASE_ADDRESS              ST71XX_CKG_C_BASE_ADDRESS
#endif

/* COMPOSITOR -------------------------------------------------------------- */

#ifndef COMPOSITOR_BASE_ADDRESS
#define COMPOSITOR_BASE_ADDRESS         ST71XX_COMPOSITOR_BASE_ADDRESS
#endif

#ifndef CURSOR_LAYER_BASE_ADDRESS
#define CURSOR_LAYER_BASE_ADDRESS       ST71XX_CURSOR_LAYER_BASE_ADDRESS
#endif
#ifndef GDP1_LAYER_BASE_ADDRESS
#define GDP1_LAYER_BASE_ADDRESS         ST71XX_GDP1_LAYER_BASE_ADDRESS
#endif
#ifndef GDP2_LAYER_BASE_ADDRESS
#define GDP2_LAYER_BASE_ADDRESS         ST71XX_GDP2_LAYER_BASE_ADDRESS
#endif
#ifndef GDP3_LAYER_BASE_ADDRESS
#define GDP3_LAYER_BASE_ADDRESS         ST71XX_GDP3_LAYER_BASE_ADDRESS
#endif
#ifndef GDP4_LAYER_BASE_ADDRESS
#define GDP4_LAYER_BASE_ADDRESS         ST71XX_GDP4_LAYER_BASE_ADDRESS
#endif
#ifndef ALP_LAYER_BASE_ADDRESS
#define ALP_LAYER_BASE_ADDRESS          ST71XX_ALP_LAYER_BASE_ADDRESS
#endif
#ifndef VID1_LAYER_BASE_ADDRESS
#define VID1_LAYER_BASE_ADDRESS         ST71XX_VID1_LAYER_BASE_ADDRESS
#endif
#ifndef VID2_LAYER_BASE_ADDRESS
#define VID2_LAYER_BASE_ADDRESS         ST71XX_VID2_LAYER_BASE_ADDRESS
#endif
#ifndef VMIX1_LAYER_BASE_ADDRESS
#define VMIX1_LAYER_BASE_ADDRESS        ST71XX_VMIX1_BASE_ADDRESS
#endif
#ifndef VMIX2_LAYER_BASE_ADDRESS
#define VMIX2_LAYER_BASE_ADDRESS        ST71XX_VMIX2_BASE_ADDRESS
#endif

/* DENC -------------------------------------------------------------------- */

#ifndef DENC_BASE_ADDRESS
#define DENC_BASE_ADDRESS               ST71XX_DENC_BASE_ADDRESS
#endif

/* DISEQC ------------------------------------------------------------------ */
#ifndef DISEQC_BASE_ADDRESS
#define DISEQC_BASE_ADDRESS             ST71XX_DISEQC_0_BASE_ADDRESS
#endif
#ifndef DISEQC_0_BASE_ADDRESS
#define DISEQC_0_BASE_ADDRESS           ST71XX_DISEQC_0_BASE_ADDRESS
#endif

#ifndef DISEQC_INTERRUPT
#define DISEQC_INTERRUPT                ST71XX_DISEQC0_INTERRUPT
#endif
#ifndef DISEQC0_INTERRUPT
#define DISEQC0_INTERRUPT               ST71XX_DISEQC0_INTERRUPT
#endif

/* DISP -------------------------------------------------------------------- */

#ifndef DISP0_BASE_ADDRESS
#define DISP0_BASE_ADDRESS              ST71XX_DISP0_BASE_ADDRESS
#endif
#ifndef DISP1_BASE_ADDRESS
#define DISP1_BASE_ADDRESS              ST71XX_DISP1_BASE_ADDRESS
#endif

#ifndef CRC_LUMA_BASE_ADDRESS
#define CRC_LUMA_BASE_ADDRESS           ST71XX_CRC_LUMA_BASE_ADDRESS
#endif
#ifndef CRC_CHROMA_BASE_ADDRESS
#define CRC_CHROMA_BASE_ADDRESS         ST71XX_CRC_CHROMA_BASE_ADDRESS
#endif


/* DVO -------------------------------------------------------------------- */

#ifndef DVO_BASE_ADDRESS
#define DVO_BASE_ADDRESS                ST71XX_DVO_BASE_ADDRESS
#endif

/* DVP -------------------------------------------------------------------- */

#ifndef DVP_BASE_ADDRESS
#define DVP_BASE_ADDRESS                ST71XX_DVP_BASE_ADDRESS
#endif

#ifndef DVP_INTERRUPT
#define DVP_INTERRUPT                   ST71XX_DVP_INTERRUPT
#endif

/* EMI --------------------------------------------------------------------- */

#ifndef EMI_BASE_ADDRESS
#define EMI_BASE_ADDRESS                ST71XX_EMI_BASE_ADDRESS
#endif

#ifndef EMI_BUS_BASE
#define EMI_BUS_BASE                    ST71XX_EMI_BUS_BASE
#endif

/* FDMA ------------------------------------------------------------------- */

#ifndef FDMA_BASE_ADDRESS
#define FDMA_BASE_ADDRESS               ST71XX_FDMA_BASE_ADDRESS
#endif

/* FDMA Interrupt numbers */
#ifndef FDMA_INTERRUPT
#define FDMA_INTERRUPT                  ST71XX_FDMA_MAILBOX_FLAG_INTERRUPT 
#endif
#ifndef FDMA_BASE_INTERRUPT
#define FDMA_BASE_INTERRUPT             ST71XX_FDMA_MAILBOX_FLAG_INTERRUPT
#endif

/* FUSE ------------------------------------------------------------------- */
#ifndef FUSE_BASE_ADDRESS
#define FUSE_BASE_ADDRESS               ST71XX_FUSE_BASE_ADDRESS
#endif

/* HDCP ------------------------------------------------------------------- */

#ifndef HDCP_BASE_ADDRESS
#define HDCP_BASE_ADDRESS               ST71XX_HDCP_BASE_ADDRESS
#endif

#ifndef HDCP_INTERRUPT
#define HDCP_INTERRUPT                  ST71XX_HDCP_INTERRUPT 
#endif

/* HDMI ------------------------------------------------------------------- */

#ifndef HDMI_BASE_ADDRESS
#define HDMI_BASE_ADDRESS               ST71XX_HDMI_BASE_ADDRESS
#endif

#ifndef HDMI_INTERRUPT
#define HDMI_INTERRUPT                  ST71XX_HDMI_INTERRUPT 
#endif

/* IRB --------------------------------------------------------------------- */

#ifndef IRB_BASE_ADDRESS
#define IRB_BASE_ADDRESS                ST71XX_IRB_BASE_ADDRESS
#endif

#ifndef IRB_INTERRUPT
#define IRB_INTERRUPT                   ST71XX_IRB_INTERRUPT
#endif

/* LMI --------------------------------------------------------------------- */

#ifndef LMI_CONFIG_BASE_ADDRESS
#define LMI_CONFIG_BASE_ADDRESS         ST71XX_LMI_CONFIG_BASE_ADDRESS
#endif

#ifndef LMI_MEMORY_BASE
#define LMI_MEMORY_BASE                 ST71XX_LMI_MEMORY_BASE
#endif

#ifndef LMI_VID_CONFIG_BASE_ADDRESS
#define LMI_VID_CONFIG_BASE_ADDRESS     ST71XX_LMI_VID_CONFIG_BASE_ADDRESS
#endif

#ifndef LMI_VID_MEMORY_BASE
#define LMI_VID_MEMORY_BASE             ST71XX_LMI_VID_MEMORY_BASE
#endif

/* LOW POWER AND WATCHDOG -------------------------------------------------- */

#ifndef LPC_BASE_ADDRESS
#define LPC_BASE_ADDRESS                ST71XX_LPC_BASE_ADDRESS
#endif

/* MAFE -------------------------------------------------------------------- */

#ifndef MAFE_BASE_ADDRESS
#define MAFE_BASE_ADDRESS               ST71XX_MAFE_BASE_ADDRESS
#endif

#ifndef MAFE_INTERRUPT
#define MAFE_INTERRUPT                  ST71XX_MAFE_INTERRUPT
#endif

/* MAILBOX ----------------------------------------------------------------- */

#ifndef MAILBOX_BASE_ADDRESS
#define MAILBOX_BASE_ADDRESS            ST71XX_MAILBOX_BASE_ADDRESS
#endif
#ifndef MAILBOX_0_BASE_ADDRESS
#define MAILBOX_0_BASE_ADDRESS          ST71XX_MAILBOX_0_BASE_ADDRESS
#endif
#ifndef MAILBOX_1_BASE_ADDRESS
#define MAILBOX_1_BASE_ADDRESS          ST71XX_MAILBOX_1_BASE_ADDRESS
#endif

/* CLOCK RECOVERY ---------------------------------------------------------- */

#ifndef MPEG_CLK_REC_INTERRUPT
#define MPEG_CLK_REC_INTERRUPT          ST71XX_MPEG_CLK_REC_INTERRUPT
#endif

/* PCM PLAYER -------------------------------------------------------------- */

#ifndef PCM_PLAYER_BASE_ADDRESS
#define PCM_PLAYER_BASE_ADDRESS         ST71XX_PCM_PLAYER_BASE_ADDRESS
#endif
#ifndef PCM_PLAYER_0_BASE_ADDRESS
#define PCM_PLAYER_0_BASE_ADDRESS       ST71XX_PCM_PLAYER_0_BASE_ADDRESS
#endif
#ifndef PCM_PLAYER_1_BASE_ADDRESS
#define PCM_PLAYER_1_BASE_ADDRESS       ST71XX_PCM_PLAYER_1_BASE_ADDRESS
#endif

#ifndef PCM_PLAYER_INTERRUPT
#define PCM_PLAYER_INTERRUPT            ST71XX_PCM_PLAYER_0_INTERRUPT
#endif
#ifndef PCM_PLAYER_0_INTERRUPT
#define PCM_PLAYER_0_INTERRUPT          ST71XX_PCM_PLAYER_0_INTERRUPT
#endif
#ifndef PCM_PLAYER_1_INTERRUPT
#define PCM_PLAYER_1_INTERRUPT          ST71XX_PCM_PLAYER_1_INTERRUPT
#endif

/* PCM READER -------------------------------------------------------------- */

#ifndef PCM_READER_BASE_ADDRESS
#define PCM_READER_BASE_ADDRESS         ST71XX_PCM_READER_BASE_ADDRESS
#endif

#ifndef PCM_READER_INTERRUPT
#define PCM_READER_INTERRUPT            ST71XX_PCM_READER_INTERRUPT
#endif

/* PCP (DES) --------------------------------------------------------------- */

/* PCP base address */
#ifndef PCP_BASE_ADDRESS
#define PCP_BASE_ADDRESS                ST71XX_PCP_BASE_ADDRESS
#endif

/* PCP interrupt number */
#ifndef PCP_INTERRUPT
#define PCP_INTERRUPT                   ST71XX_DES_INTERRUPT
#endif

/* PIO --------------------------------------------------------------------- */

/* Base addresses */
#ifndef PIO_0_BASE_ADDRESS
#define PIO_0_BASE_ADDRESS              ST71XX_PIO0_BASE_ADDRESS
#endif
#ifndef PIO_1_BASE_ADDRESS
#define PIO_1_BASE_ADDRESS              ST71XX_PIO1_BASE_ADDRESS
#endif
#ifndef PIO_2_BASE_ADDRESS
#define PIO_2_BASE_ADDRESS              ST71XX_PIO2_BASE_ADDRESS
#endif
#ifndef PIO_3_BASE_ADDRESS
#define PIO_3_BASE_ADDRESS              ST71XX_PIO3_BASE_ADDRESS
#endif
#ifndef PIO_4_BASE_ADDRESS
#define PIO_4_BASE_ADDRESS              ST71XX_PIO4_BASE_ADDRESS
#endif
#ifndef PIO_5_BASE_ADDRESS
#define PIO_5_BASE_ADDRESS              ST71XX_PIO5_BASE_ADDRESS
#endif

/* Interrupt numbers */
#ifndef PIO_0_INTERRUPT
#define PIO_0_INTERRUPT                 ST71XX_PIO0_INTERRUPT
#endif
#ifndef PIO_1_INTERRUPT
#define PIO_1_INTERRUPT                 ST71XX_PIO1_INTERRUPT
#endif
#ifndef PIO_2_INTERRUPT
#define PIO_2_INTERRUPT                 ST71XX_PIO2_INTERRUPT
#endif
#ifndef PIO_3_INTERRUPT
#define PIO_3_INTERRUPT                 ST71XX_PIO3_INTERRUPT
#endif
#ifndef PIO_4_INTERRUPT
#define PIO_4_INTERRUPT                 ST71XX_PIO4_INTERRUPT
#endif
#ifndef PIO_5_INTERRUPT
#define PIO_5_INTERRUPT                 ST71XX_PIO5_INTERRUPT
#endif

/* PTI --------------------------------------------------------------------- */

/* Base addresses */
#ifndef PTI_BASE_ADDRESS
#define PTI_BASE_ADDRESS                ST71XX_PTI_BASE_ADDRESS
#endif
#ifndef PTIA_BASE_ADDRESS
#define PTIA_BASE_ADDRESS               ST71XX_PTIA_BASE_ADDRESS
#endif
#ifndef PTIB_BASE_ADDRESS
#define PTIB_BASE_ADDRESS               ST71XX_PTIB_BASE_ADDRESS /*only from 71XX cut 2*/
#endif

/* Interrupt numbers */
#ifndef PTI_INTERRUPT
#define PTI_INTERRUPT                   ST71XX_PTI_INTERRUPT
#endif
#ifndef PTIA_INTERRUPT
#define PTIA_INTERRUPT                  ST71XX_PTIA_INTERRUPT
#endif
#ifndef PTIB_INTERRUPT
#define PTIB_INTERRUPT                  ST71XX_PTIB_INTERRUPT
#endif

/* PWM --------------------------------------------------------------------- */

/* Base addresses */
#ifndef PWM_BASE_ADDRESS
#define PWM_BASE_ADDRESS                ST71XX_PWM_BASE_ADDRESS
#endif
#ifndef STCOUNT_CONTROLLER_BASE
#define STCOUNT_CONTROLLER_BASE         ST71XX_PWM_BASE_ADDRESS
#endif

/* Interrupt number */
#ifndef PWM_CAPTURE_INTERRUPT
#define PWM_CAPTURE_INTERRUPT           ST71XX_PWM_A_INTERRUPT
#endif
#ifndef PWM_A_CAPTURE_INTERRUPT
#define PWM_A_CAPTURE_INTERRUPT         ST71XX_PWM_A_INTERRUPT
#endif

/* SATA -------------------------------------------------------------------- */

#ifndef SATA_BASE_ADDRESS
#define SATA_BASE_ADDRESS               ST71XX_SATA_BASE_ADDRESS
#endif

#ifndef SATA_INTERRUPT
#define SATA_INTERRUPT                  ST71XX_SATA_INTERRUPT
#endif

/* SCIF -------------------------------------------------------------------- */
#ifndef SCIF_BASE_ADDRESS
#define SCIF_BASE_ADDRESS               ST71XX_SCIF_BASE_ADDRESS
#endif

/* SPDIF PLAYER ------------------------------------------------------------ */

#ifndef SPDIF_PLAYER_BASE_ADDRESS
#define SPDIF_PLAYER_BASE_ADDRESS       ST71XX_SPDIF_PLAYER_BASE_ADDRESS
#endif

#ifndef SPDIF_BASE_ADDRESS
#define SPDIF_BASE_ADDRESS              ST71XX_SPDIF_PLAYER_BASE_ADDRESS /*this define (name) is for backward compatability*/
#endif

#ifndef SPDIF_PLAYER_INTERRUPT
#define SPDIF_PLAYER_INTERRUPT          ST71XX_SPDIF_PLAYER_INTERRUPT
#endif

/* SMART ------------------------------------------------------------------- */

/* Base addresses */
#ifndef SMARTCARD0_BASE_ADDRESS
#define SMARTCARD0_BASE_ADDRESS         ST71XX_SC0_BASE_ADDRESS
#endif
#ifndef SMARTCARD1_BASE_ADDRESS
#define SMARTCARD1_BASE_ADDRESS         ST71XX_SC1_BASE_ADDRESS
#endif

/* SSC (I2C) --------------------------------------------------------------- */

/* Base addresses */
#ifndef SSC_0_BASE_ADDRESS
#define SSC_0_BASE_ADDRESS              ST71XX_SSC0_BASE_ADDRESS
#endif
#ifndef SSC_1_BASE_ADDRESS
#define SSC_1_BASE_ADDRESS              ST71XX_SSC1_BASE_ADDRESS
#endif
#ifndef SSC_2_BASE_ADDRESS
#define SSC_2_BASE_ADDRESS              ST71XX_SSC2_BASE_ADDRESS
#endif

/* Interrupt numbers */
#ifndef SSC_0_INTERRUPT
#define SSC_0_INTERRUPT                 ST71XX_SSC0_INTERRUPT
#endif
#ifndef SSC_1_INTERRUPT
#define SSC_1_INTERRUPT                 ST71XX_SSC1_INTERRUPT
#endif
#ifndef SSC_2_INTERRUPT
#define SSC_2_INTERRUPT                 ST71XX_SSC2_INTERRUPT
#endif

/* TSMERGE ----------------------------------------------------------------- */

#ifndef TSMERGE_BASE_ADDRESS            /*address of tsmerge config*/
#define TSMERGE_BASE_ADDRESS            ST71XX_TSMERGE_BASE_ADDRESS
#endif
#ifndef TSMERGE1_BASE_ADDRESS           /*address of tsmerge config*/
#define TSMERGE1_BASE_ADDRESS           ST71XX_TSMERGE1_BASE_ADDRESS
#endif
#ifndef TSMERGE2_BASE_ADDRESS           /*address of tsmerge SWTS data*/
#define TSMERGE2_BASE_ADDRESS           ST71XX_TSMERGE2_BASE_ADDRESS
#endif

#ifndef TSMERGE_INTERRUPT
#define TSMERGE_INTERRUPT               ST71XX_TSMERGE_INTERRUPT
#endif

/* TTX --------------------------------------------------------------------- */

/* TTX base addresses */
#ifndef TTXT_BASE_ADDRESS
#define TTXT_BASE_ADDRESS               ST71XX_TTX_BASE_ADDRESS
#endif

/* TTX interrupt numbers */
#ifndef TELETEXT_INTERRUPT
#define TELETEXT_INTERRUPT              ST71XX_TELETEXT_INTERRUPT
#endif

/* USB ------------------------------------------------------------------- */

/* Base address */
#ifndef USB_BASE_ADDRESS
#define USB_BASE_ADDRESS                ST71XX_USB_BASE_ADDRESS
#endif

/* Interrupt numbers */
#ifndef USB_INTERRUPT
#define USB_INTERRUPT                   ST71XX_USB_OHCI_INTERRUPT
#endif
#ifndef USB0_INTERRUPT
#define USB0_INTERRUPT                  ST71XX_USB_OHCI_INTERRUPT
#endif
#ifndef USB1_INTERRUPT
#define USB1_INTERRUPT                  ST71XX_USB_EHCI_INTERRUPT
#endif

/* VDP --------------------------------------------------------------------- */
#ifndef VDP_END_PROCESSING_INTERRUPT
#define VDP_END_PROCESSING_INTERRUPT    ST71XX_VDP_END_PROCESSING_INTERRUPT
#endif

#ifndef VDP_FIFO_EMPTY_INTERRUPT
#define VDP_FIFO_EMPTY_INTERRUPT        ST71XX_VDP_FIFO_EMPTY_INTERRUPT
#endif

/* VIDEO ------------------------------------------------------------------- */

/* Base addresses */
#ifndef VIDEO_BASE_ADDRESS
#define VIDEO_BASE_ADDRESS              ST71XX_VIDEO_BASE_ADDRESS
#endif

#ifndef DELTA_BASE_ADDRESS
#define DELTA_BASE_ADDRESS              ST71XX_DELTA_BASE_ADDRESS
#endif

/* Interrupt numbers */
#ifndef VIDEO_INTERRUPT
#define VIDEO_INTERRUPT                 ST71XX_GLH_INTERRUPT
#endif

/* VOS ------------------------------------------------------------------- */

#ifndef VOS_BASE_ADDRESS
#define VOS_BASE_ADDRESS                ST71XX_VOS_BASE_ADDRESS
#endif
#ifndef VOUT_BASE_ADDRESS
#define VOUT_BASE_ADDRESS               ST71XX_VOUT_BASE_ADDRESS
#endif

/* VTG ------------------------------------------------------------------- */

#ifndef VTG_BASE_ADDRESS
#define VTG_BASE_ADDRESS                ST71XX_VTG_BASE_ADDRESS
#endif

#ifndef VTG_LINE_INTERRUPT
#define VTG_LINE_INTERRUPT              ST71XX_VTG_1_INTERRUPT
#endif
#ifndef VTG_VSYNC_INTERRUPT
#define VTG_VSYNC_INTERRUPT             ST71XX_VTG_0_INTERRUPT
#endif

#endif /* GENERIC_ADDRESSES */

#ifdef __cplusplus
}
#endif

#endif /* __STI71XX_H */
