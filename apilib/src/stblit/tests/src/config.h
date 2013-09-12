/*******************************************************************************

File name   : Config.h

Description : Constants and exported functions

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                                 Name
----               ------------                                 ----
May 2002           Created                                      GS
*******************************************************************************/


/* Define to prevent recursive inclusion */
#ifndef __CONFIG_H
#define __CONFIG_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Macros and Defines -------------------------------------------- */

#if defined (ST_7015)
#define BLITTER_INT_EVENT ST7015_BLITTER_INT
#elif defined (ST_7020)
#define BLITTER_INT_EVENT ST7020_BLITTER_INT
#elif defined (ST_GX1)
#define BLITTER_INT_EVENT 0
#elif defined (ST_5528)
#define BLITTER_INT_LEVEL 0
#define BLITTER_INT_EVENT ST5528_BLITTER_INTERRUPT
#elif defined (ST_5100)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5100_BLIT_AQ1_INTERRUPT
#elif defined (ST_5301)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5301_BLIT_AQ1_INTERRUPT
#elif defined (ST_5105)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5105_BLIT_AQ1_INTERRUPT
#elif defined (ST_5188)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5188_BLIT_AQ1_INTERRUPT
#elif defined (ST_5107)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5107_BLIT_AQ1_INTERRUPT
#elif defined (ST_5525)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5525_BLIT_AQ1_INTERRUPT
#elif defined (ST_7200)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST7200_BLIT_AQ1_INTERRUPT
#elif defined (ST_7710)
#define BLITTER_INT_LEVEL 0
#define BLITTER_INT_EVENT ST7710_BLT_REG_INTERRUPT
#elif defined (ST_7100)
#define BLITTER_INT_LEVEL 0
#define BLITTER_INT_EVENT  ST7100_BLITTER_INTERRUPT
#elif defined (ST_7109)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST7109_BLITTER_INTERRUPT
#endif



#if defined (STBLIT_SOFTWARE)

#define CURRENT_BASE_ADDRESS    (void*)BlitterRegister
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_SOFTWARE;

#elif defined (STBLIT_EMULATOR)

#define CURRENT_BASE_ADDRESS    (void*)BlitterRegister
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION/*STBLIT_DEVICE_TYPE_BDISP_5100*/;

#elif defined (ST_GX1)

#define CURRENT_BASE_ADDRESS    (void*)(0xbb400a00)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_ST40GX1;

#elif defined (ST_7020)

#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_7020;
#define CURRENT_BASE_ADDRESS    (void*)(STI7020_BASE_ADDRESS + ST7020_GAMMA_OFFSET + ST7020_GAMMA_BLIT_OFFSET);

#elif defined (ST_5528)

#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_5528;
#define CURRENT_BASE_ADDRESS    (void*)(ST5528_BLITTER_BASE_ADDRESS);

#elif defined (ST_7710)

#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_7710;
#define CURRENT_BASE_ADDRESS    (void*)(ST7710_BLITTER_BASE_ADDRESS);

#elif defined (ST_7100)

#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_7100;
#define CURRENT_BASE_ADDRESS    (void*)(ST7100_BLITTER_BASE_ADDRESS);

#elif defined (ST_5100)

#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5100;
#define CURRENT_BASE_ADDRESS    (void*)(ST5100_BLITTER_BASE_ADDRESS);

#elif defined (ST_5301)

#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5301;
#define CURRENT_BASE_ADDRESS    (void*)(ST5301_BLITTER_BASE_ADDRESS);

#elif defined (ST_5105)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5105;
#define CURRENT_BASE_ADDRESS    (void*)(ST5105_BLITTER_BASE_ADDRESS);

#elif defined (ST_5188)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5188;
#define CURRENT_BASE_ADDRESS    (void*)(ST5188_BLITTER_BASE_ADDRESS);

#elif defined (ST_5107)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5107;
#define CURRENT_BASE_ADDRESS    (void*)(ST5107_BLITTER_BASE_ADDRESS);

#elif defined (ST_5525)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5525;
#define CURRENT_BASE_ADDRESS    (void*)(ST5525_BLITTER_BASE_ADDRESS);

#elif defined (ST_7109)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_7109;
#define CURRENT_BASE_ADDRESS    (void*)(ST7109_BLITTER_BASE_ADDRESS);

#elif defined (ST_7200)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_7200;
#define CURRENT_BASE_ADDRESS    (void*)(ST7200_BLITTER_BASE_ADDRESS);

#else
#define CURRENT_BASE_ADDRESS    (void*)(0x6000AA00)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_7015;

#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H */

