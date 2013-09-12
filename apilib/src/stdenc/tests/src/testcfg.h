/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
30 Oct 2002        Add 5517 support                                 HSdLM
06 May 2003        Add 7020 stem db573 support                      HSdLM
21 Aug 2003        Add 5100/mb390 support                            TA
02 Jul 2004        Add 7710/mb391 support                            TA
01 Dec 2004        Add 5105/mb400 support                            TA
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

/* All chips */

#ifdef ST_OSLINUX
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/denc/default.mac"
#else
#define USE_TBX
#define DUMP_REGISTERS /* not implemented yet. To be done */
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.mac"
#endif

#define USE_VTGSIM
#define USE_VOUTSIM
#define USE_TESTTOOL
#define USE_DENC
#define USE_API_TOOL
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518)
#define USE_PIO
#define USE_PWM
#define USE_DISP_OSD

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
#define USE_STCFG
#define USE_PIO
#define USE_PWM
#if !defined(ST_7020) /* mb290:5514+7020, 7020stem can be on mb314 (5514) or mb382 (5517) */
#define USE_DISP_OSD  /* if 7020 defined the display is done in */
#endif /* !defined(ST_7020) */

#elif defined(ST_7015) || defined(ST_5528) || defined(ST_7710)
#if !defined(ST_OSLINUX)
	#define USE_PIO
	#define USE_PWM
#endif

#if !defined(ST_OSLINUX) /* not implemented yet. To be done */
	#define USE_DISP_GAM
#endif

#elif defined(ST_7100)
#if !defined(ST_OSLINUX) /* not implemented yet. To be done */
    #define USE_PIO
    #define USE_AVMEM
#endif
#define USE_EVT
#define USE_LAYER
#define USE_DISP_VIDEO
#elif defined (ST_7109)
#if !defined(ST_OSLINUX)
#define USE_AVMEM
#endif      /* ST_OSLINUX */
#define USE_EVT
#define USE_LAYER
#define USE_DISP_VIDEO
#elif defined(ST_7200)
#if defined(ST_OSLINUX)
#define USE_REG
#endif
#define USE_DISP_GAM
#elif defined(ST_5100)
#define USE_PIO         /* HSdLM 21-08-03 9:22AM: to be confirmed */
#define USE_PWM         /* HSdLM 21-08-03 9:22AM: to be confirmed */
#define USE_DISP_GDMA   /* HSdLM 21-08-03 9:22AM: to be developped in stapigat */

#elif defined(ST_5105)
#define USE_PIO
#define USE_DISP_GDMA

#elif defined(ST_5188)
#define USE_PIO
#define USE_DISP_GDMA

#elif defined(ST_5107)||defined(ST_5162)
#define USE_DISP_GDMA

#elif defined(ST_5301)
/*#define USE_PIO*/
#define USE_DISP_GDMA

#elif defined(ST_5525)
/*#define USE_PIO*/
#define USE_DISP_GDMA

#endif

#if defined(ST_7020)
#define USE_DISP_GAM
#if !defined(mb290) && !defined(mb295) /* 7020 stem db573 */
#define USE_I2C_FRONT
#define USE_STEM_RESET
#endif /* 7020 stem*/
/* let default: merou, overscan is too big for NTSC square: makes 7020 GDP badly displayed */
/*#define GAM_BITMAP_NAME "overscan.gam"*/
#endif

#if defined(ST_GX1)
#define USE_DISP_GAM
#define GAM_BITMAP_NAME "overscan.gam"
#endif


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
#define USE_PIO         /* actually on ST20TP3 */
#define USE_PWM         /* actually on ST20TP3 */
#define USE_I2C         /* actually on ST20TP3, for DENC stv0119 */
#define USE_STDENC_I2C  /* for DENC stv0119 */

#elif defined(mb290)

#elif defined(mb314) || defined(mb361) || defined(mb382) || defined (espresso)
#if !defined(ST_OSLINUX)
	#define USE_I2C
	#define USE_SCART_SWITCH_CONFIG
#endif

#elif defined(mb317)

#elif defined(mb376)

#elif defined(mb390)

#elif defined(mb391)

#elif defined(mb400)

#elif defined(mb411)

#elif defined(mb519)

#elif defined(mb457)

#elif defined(mb428)

#elif defined(mb436)||defined (DTT5107)

#elif defined(mb634)
#else
#error ERROR:no DVD_PLATFORM defined
#endif

/* ***********************************************************************
 *                        library inter dependencies
 * ***********************************************************************/

/* USE_SCART_SWITCH_CONFIG needs USE_I2C */
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

/* STGPDMA, STINTMR, STCLKRV need STEVT */
#if (defined (USE_GPDMA) || defined (USE_INTMR) || defined (USE_CLKRV)) && !defined (USE_EVT)
#define USE_EVT
#endif
/* STI2C can do without STEVT when STI2C_MASTER_ONLY is set,  */


/* STVTG, STVBI, STVOUT need STDENC */
#if (defined (USE_VTG) || defined (USE_VOUT) || defined (USE_VBI)) && !defined (USE_DENC)
#define USE_DENC
#endif

#endif /* #ifndef __TESTCFG_H */
