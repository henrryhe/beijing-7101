/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
14 Jun 2002        Update with new flags, re-organize.              HSdLM
28 Jun 2002        Remove video, add PTI & TTX for 5516 & 7020      HSdLM
11 Feb 2003        Add support of 5517 and mb290                    HSdLM
14 May 2003        Add support for 7020stem                         HSdLM
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

#if defined(ST_5517)
#define RS232_B
#endif

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

/* All chips */
#ifdef ST_OSLINUX
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/vbi/default.mac"
#else
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.mac"
#endif

#if !defined(ST_OSLINUX)
#define USE_UART
#define USE_AVMEM
#define USE_TBX
#endif /** !ST_OSLINUX **/

#define USE_TESTTOOL
#define USE_EVT
#define USE_DENC
#define USE_VTG
#define USE_VOUT
#define USE_VBI
#define USE_API_TOOL

#if defined(ST_7710)||defined (ST_7109)||defined (ST_7100) || defined (ST_7200)
#define USE_VMIX
#define USE_LAYER
#endif

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined (ST_TP3)
#define USE_PIO
#define USE_PWM
#define USE_CLKRV
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */
#define USE_TTX
#define USE_DISP_OSD
#endif /* ST_5510 || ST_5512 || ST_5508 || ST_5518 || ST_TP3 */

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
#define USE_STCFG
#define USE_PIO
#define USE_PWM

#if !defined(ST_7020) /* mb290:5514+7020, 7020stem can be on mb314 (5514) or mb382 (5517) */
#define USE_CLKRV
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */
#define USE_TTX
#define USE_DISP_OSD  /* if 7020 is defined, the display is done in */
#endif /* !defined(ST_7020) */
#endif /* #if ST_5514 || ST_5516 || ST_5517 */

#if defined(ST_7015) || defined(ST_GX1)
#define USE_INTMR
#define USE_DISP_GAM
#endif /* ST_7015 GX1 */

#if defined(ST_5528)
#define USE_PIO
#define USE_PWM
#define USE_TTX
#define USE_DISP_GAM
#endif /* ST_5528  */

#if defined(ST_5100)
#define USE_PIO
#define USE_PWM
#define USE_CLKRV
#define USE_TTX
#define USE_DISP_GDMA   /* to be developped in stapigat */
#endif /*ST_5100 */

#if defined(ST_5301)
#define USE_PIO
#define USE_TTX
#define USE_DISP_GDMA   /* to be developped in stapigat */
#endif /*ST_5301 */

#if defined(ST_5525)
#define USE_PIO
#define USE_DISP_GDMA   /* to be developped in stapigat */
#define USE_VMIX
#define USE_LAYER

#endif /*ST_5525 */

#if defined(ST_5105)|| defined(ST_5188)|| defined(ST_5107)
#define USE_PIO
#define USE_DISP_GDMA   /* to be developped in stapigat */
#define USE_VMIX
#define USE_LAYER
#endif /*ST_5105-5188-5107 */

#if defined(ST_7020)
#define USE_INTMR
#define USE_DISP_GAM
#if !defined(mb290) && !defined(mb295) /* 7020 stem db573 */
#define USE_I2C_FRONT
#define USE_STEM_RESET
#endif /* 7020 stem*/
#endif /* ST_7020 */

#if defined(ST_7710)
#define USE_PIO
#define USE_CLKRV
#define USE_TTX
#define USE_DISP_GAM
#endif /* ST_7710  */

#if defined(ST_7100)
#define USE_PIO
#define USE_CLKRV
#define USE_TTX
#define USE_DISP_VIDEO
#endif /* ST_7100  */

#if defined(ST_7109)
#define USE_PIO
#define USE_CLKRV
#define USE_TTX
#define USE_DISP_VIDEO
#endif /* ST_7109  */

#if defined(ST_7200)
#define USE_PIO
#define USE_CLKRV
/*#define USE_TTX*/
#define USE_DISP_GAM
#endif /* ST_7200  */

/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library you need
 *#########################################################################*/

#if defined(mb231)

#elif defined(mb282b)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb275)

#elif defined(mb275_64)

#elif defined(mb295)
#define USE_I2C
#define USE_STDENC_I2C

#elif defined (mb376) || defined(espresso)

#elif defined(mb290)

#elif defined(mb314) || defined(mb361) || defined(mb382)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG
/* overrides startup.c default to allow placing section def_bss into EXTERNAL */
#ifdef UNIFIED_MEMORY
#define CACHED_SIZE 0x0200000    /* 2 MBytes */
#endif

#elif defined(mb317a) || defined(mb317b)
#elif defined(mb390)  || defined (mb400)|| defined (mb457)|| defined (mb436)

#elif defined(mb391)

#elif defined(mb428)

#ifdef STVMIX_USE_UART
#define USE_UART
#define USE_UART_AS_STDIO
#endif

#elif defined(mb411) || defined(mb519)
#if !defined(ST_OSLINUX)
#define USE_I2C
#endif

#else
#error ERROR:no DVD_PLATFORM defined
#endif

/* ***********************************************************************
 *                        library inter dependencies
 * ***********************************************************************/

/* USE_SCART_SWITCH_CONFIG, USE_STEM_RESET needs USE_I2C */
#if (defined (USE_SCART_SWITCH_CONFIG) || defined (USE_STDENC_I2C) || defined (USE_STEM_RESET))&& !defined (USE_I2C)
#define USE_I2C
#endif

/* TRACE_UART needs USE_UART */
#if defined (TRACE_UART) && !defined (USE_UART)
#define USE_UART
#endif

/* STUART, STPWM, STCFG, STI2C needs PIO */
#if (defined (USE_PWM) || defined (USE_UART) || defined (USE_STCFG) || defined (USE_I2C)) && !defined (USE_PIO)
#define USE_PIO
#endif

/* STGPDMA, STINTMR, STI2C, STCLKRV need STEVT */
#if (defined (USE_GPDMA) || defined (USE_INTMR) || defined (USE_I2C) || defined (USE_CLKRV)) && !defined (USE_EVT)
#if !defined(ST_OSLINUX)
#define USE_EVT
#endif
#endif

/* STVTG, STVBI, STVOUT need STDENC */
#if (defined (USE_VTG) || defined (USE_VOUT) || defined (USE_VBI)) && !defined (USE_DENC)
#define USE_DENC
#endif

#endif /* #ifndef __TESTCFG_H */


