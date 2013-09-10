/************************************************************************
File Name   : swconfig.h

Description : System-wide s/w configuration file for STAPI.
              This contains s/w-specific definitions for STAPI systems.

Copyright (C) 1999 STMicroelectronics

Reference   :

NOTE : This file is now deprecated. Application designers should determine
the interrupt levels that best suit their system.
************************************************************************/

#ifndef __SWCONFIG_H
#define __SWCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* System devices (in alphabetical order) ---------------------------------- */

/* 1284 ------------------------------------------------------------------- */

/* Interrupt level */
#define ST1284_INTERRUPT_LEVEL          2

/* ASC --------------------------------------------------------------------- */

/* Interrupt levels */
#define ASC_0_INTERRUPT_LEVEL           1
#define ASC_1_INTERRUPT_LEVEL           1
#define ASC_2_INTERRUPT_LEVEL           1
#define ASC_3_INTERRUPT_LEVEL           1
#define ASC_4_INTERRUPT_LEVEL           1

/* ATA   ------------------------------------------------------------------- */

/* Interrupt level */
#define ATA_INTERRUPT_LEVEL             6

/* AUDIO ------------------------------------------------------------------- */

/* Interrupt level */
#define AUDIO_INTERRUPT_LEVEL           3

/* BACKEND DEVICE ---------------------------------------------------------- */

/* Interrupt level */
#define BACKEND_INTERRUPT_LEVEL         4

/* BLOCKMOVE --------------------------------------------------------------- */

/* Interrupt level */
#define BLOCKMOVE_INTERRUPT_LEVEL       7

/* PIO --------------------------------------------------------------------- */

/* Interrupt levels */
#define PIO_0_INTERRUPT_LEVEL           1
#define PIO_1_INTERRUPT_LEVEL           1
#define PIO_2_INTERRUPT_LEVEL           2
#define PIO_3_INTERRUPT_LEVEL           6
#define PIO_4_INTERRUPT_LEVEL           1
#define PIO_5_INTERRUPT_LEVEL           1

/* the following two definitions are added for consistency on 5528,
  but the interrupt level chosen is somewhat arbitrary. Apart from
  the ASC on PIO 6, the correct number depends on what you choose
  to use the pins for */
  
#define PIO_6_INTERRUPT_LEVEL           1
#define PIO_7_INTERRUPT_LEVEL           1
  
/* MPEGDMA levels ---------------------------------------------------------- */
#define MPEGDMA_0_INTERRUPT_LEVEL       2
#define MPEGDMA_1_INTERRUPT_LEVEL       2
#define MPEGDMA_2_INTERRUPT_LEVEL       2

/* PTI or LINK ------------------------------------------------------------- */
/* Interrupt level */
#define PTI_INTERRUPT_LEVEL             6
#define LINK_INTERRUPT_LEVEL            PTI_INTERRUPT_LEVEL

/* HDD BUFFER -------------------------------------------------------------- */
/* Interrupt level */
#define LINK_HDD_INTERRUPT_LEVEL        6

/* PWM --------------------------------------------------------------------- */

/* Interrupt level */
#define PWM_INTERRUPT_LEVEL             6

/* SSC (I2C) --------------------------------------------------------------- */

/* Interrupt levels */
#define SSC_0_INTERRUPT_LEVEL           5
#define SSC_1_INTERRUPT_LEVEL           5

/* TTX --------------------------------------------------------------------- */

/* Interrupt level */
#define TTXT_INTERRUPT_LEVEL            2

/* PCP --------------------------------------------------------------------- */

/* Interrupt level */
#define PCP_INTERRUPT_LEVEL             5

/* VIDEO ------------------------------------------------------------------- */

/* Interrupt level */
#define VIDEO_INTERRUPT_LEVEL           4

/* IR Blaster -------------------------------------------------------------- */

/* Interrupt level */
#define IRB_INTERRUPT_LEVEL             2

/* OTHER DEFINITIONS ------------------------------------------------------- */

#define EXTERNAL_0_INTERRUPT_LEVEL      4
#define EXTERNAL_1_INTERRUPT_LEVEL      4
#define EXTERNAL_2_INTERRUPT_LEVEL      4
#define EXTERNAL_3_INTERRUPT_LEVEL      4

#ifdef __cplusplus
}
#endif

#endif /* __SWCONFIG_H */
