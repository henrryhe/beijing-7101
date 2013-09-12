/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
17 Apr 2002        Add support for STi5516/STi7020                  HSdLM
11 Oct 2002        Add support for STi5517                          HSdLM
12 May 2003        Add support for STi5528, remove stavmem dep.     HSdLM
21 Aug 2003        Add 5100/mb390 support                           HSdLM
05 Jul 2004        Add 7710/mb391 support                            AT
14 Mar 2005        Add 5301 support                                 ICA

*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

/* All chips */
#ifdef ST_OSLINUX
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/vtg/default.mac"
#else
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.mac"

#define USE_TBX
#define DUMP_REGISTERS /* not implemented yet. To be done */
#endif

#define USE_EVT
#define USE_VOUTSIM
#define USE_TESTTOOL
#define USE_DENC
#define USE_VTG
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

#elif defined(ST_5528)
#if !defined(ST_OSLINUX)
#define USE_PIO
#define USE_PWM
#endif

#if !defined(ST_OSLINUX) /* not implemented yet. To be done */
#define USE_DISP_GAM
#endif

#elif defined(ST_5100)
#define USE_PIO
#define USE_PWM
#define USE_DISP_GDMA

#elif defined(ST_5301)
/*#define USE_PIO
#define USE_PWM*/
#define USE_DISP_GDMA

#elif defined(ST_5105)
#define USE_PIO
#define USE_DISP_GDMA

#elif defined(ST_5188)|| defined(ST_5107) || defined(ST_5162)
/*#define USE_PIO*/
#define USE_DISP_GDMA

#elif defined(ST_5525)
/*#define USE_PIO*/
#define USE_DISP_GDMA


#elif defined(ST_GX1)
#define USE_INTMR
#define USE_DISP_GAM

#elif defined(ST_TP3)
#define USE_PIO
#define USE_PWM

#elif defined(ST_7710)
#define USE_PIO
#define USE_DISP_GAM
#define USE_CLKRV

#elif defined(ST_7100)
#if !defined(ST_OSLINUX) /* not implemented yet. To be done */
#ifndef ST_OSWINCE
#define USE_PIO
#endif /* ST_OSWINCE*/
#define USE_AVMEM
#endif
#define USE_CLKRV
#define USE_DISP_VIDEO
#define USE_LAYER

#elif defined(ST_7109)
/*#define USE_PIO
#define USE_DISP_GAM*/
#if !defined(ST_OSLINUX)
#define USE_AVMEM
#endif  /** ST_OSLINUX **/
#define USE_CLKRV
#define USE_DISP_VIDEO
#define USE_LAYER

#elif defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105)
#define USE_CLKRV
#define USE_DISP_GAM

#else
#error ERROR:no DVD_FRONTEND defined
#endif

#if defined(ST_7015)
#define USE_INTMR
#define USE_DISP_GAM
#endif

#if defined(ST_7020)
#define USE_INTMR
#define USE_DISP_GAM
#if !defined(mb290) && !defined(mb295) /* 7020 stem db573 */
#define USE_I2C_FRONT
#define USE_STEM_RESET
#endif /* 7020 stem*/
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
#define USE_I2C
#define USE_STDENC_I2C

#elif defined(mb290)

#elif defined(mb314) || defined(mb361) || defined(mb382)|| defined(espresso) || defined(mb400)||defined(mb634)
#if !defined(ST_OSLINUX)
	#define USE_I2C
	#define USE_SCART_SWITCH_CONFIG
#endif

#elif defined(mb317a) || defined(mb317b)

#elif defined(mb376)

#elif defined(mb390)
 /* HSdLM 21-08-03 11:43AM: assume no need to drive SCART switch (S/W requirement) */
#elif defined(mb391)

#elif defined(mb411)

#elif defined(mb457)

#elif defined(mb428)

#elif defined(mb436)||defined (DTT5107)

#elif defined(mb634)

#elif defined(mb519)
#elif defined(mb618)
#elif defined(mb680)
#else
#error ERROR:no DVD_PLATFORM defined
#endif


/* ***********************************************************************
 *                        library inter dependencies
 * ***********************************************************************/

/* USE_SCART_SWITCH_CONFIG, USE_STEM_RESET needs USE_I2C */
#if (defined (USE_SCART_SWITCH_CONFIG) || defined (USE_STDENC_I2C) || defined (USE_STEM_RESET)) && !defined (USE_I2C)
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
#define USE_EVT
#endif

/* STVTG, STVBI, STVOUT need STDENC */
#if (defined (USE_VTG) || defined (USE_VOUT) || defined (USE_VBI)) && !defined (USE_DENC)
#define USE_DENC
#endif

#endif /* #ifndef __TESTCFG_H */
