/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
24 Apr 2002        Add support for STi5516/STi7020                  HSdLM
21 Jan 2003        Symplify & add mb290 support                     BS
12 May 2003        Add 7020 stem db573 support                      HSdLM
04 Jun 2003        Add support for STi5528                          HSdLM
21 Aug 2003        Add 5100/mb390 support                           HSdLM
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

/* All chips */

#ifdef ST_OSLINUX
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/vout/default.mac"
#else
#define USE_TBX
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.mac"
#endif


/*#define USE_POLL*/
#define USE_EVT
#define USE_TESTTOOL
#define USE_DENC
#define USE_VTG
#define USE_VOUT
#define USE_API_TOOL


#if defined (STVOUT_CEC_PIO_COMPARE)
#define USE_PWM
#endif

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518)
#define USE_PIO
#define USE_PWM
#define USE_DISP_OSD

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
#define USE_STCFG
#define USE_PIO
#define USE_PWM
#define USE_DISP_OSD

#elif defined(ST_5528)
#if !defined(ST_OSLINUX)
	#define USE_PIO
	#define USE_PWM
    #define USE_DISP_GAM
#endif

#elif defined(ST_5100)
#define USE_PIO         /* HSdLM 21-08-03 9:22AM: to be confirmed */
#define USE_PWM         /* HSdLM 21-08-03 9:22AM: to be confirmed */
#define USE_DISP_GDMA   /* HSdLM 21-08-03 9:22AM: to be developped in stapigat */

#elif defined (ST_5105)
#define USE_PIO
#define USE_DISP_GDMA

#elif defined (ST_5107) || defined(ST_5162)
/*#define USE_PIO*/
#define USE_DISP_GDMA

#elif defined (ST_5188)
#define USE_DISP_GDMA

#elif defined(ST_5301)
#define USE_PIO
/*#define USE_PWM*/
#define USE_DISP_GDMA

#elif defined (ST_5525)
#define USE_DISP_GDMA

#elif defined (ST_7710)
#define USE_PIO
#define USE_EVT
#define USE_DISP_GAM
#define USE_CLKRV
#elif defined (ST_7100)
#if !defined(ST_OSLINUX)
#define USE_EVT
#define USE_AVMEM
#endif  /** ST_OSLINUX **/
#define USE_PIO
#define USE_CLKRV
#define USE_DISP_VIDEO
#define USE_LAYER
#if defined(ST_OSLINUX)
#define USE_REG
#endif  /** ST_OSLINUX **/
#elif defined (ST_7109)
#if !defined(ST_OSLINUX)
#define USE_EVT
#define USE_AVMEM
#endif /** ST_OSLINUX **/
/*#define USE_DISP_GAM */
#define USE_PIO
#define USE_DISP_VIDEO
#define USE_LAYER
#define USE_CLKRV
#if defined(ST_OSLINUX)
#define USE_REG
#endif  /** ST_OSLINUX **/

#elif defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#if !defined(ST_OSLINUX)
#define USE_EVT
#define USE_I2C
#endif /** ST_OSLINUX **/
#define USE_PIO
#define USE_DISP_GAM
#define USE_CLKRV
#define USE_BOOT
#if defined(ST_OSLINUX)
#define USE_REG
#endif  /** ST_OSLINUX **/
#elif defined(ST_GX1)
#define USE_EVT
#define USE_INTMR
#define USE_DISP_GAM
#define GAM_BITMAP_NAME "overscan.gam"

#elif defined(ST_TP3)
#define USE_PIO
#define USE_PWM

#else
#error ERROR:no DVD_FRONTEND defined
#endif

#if defined(ST_7015)
#define USE_EVT
#define USE_INTMR
#define USE_DISP_GAM
#define GAM_BITMAP_NAME "overscan.gam"
#endif

#if defined(ST_7020)
#define USE_EVT
#define USE_INTMR
#define USE_DISP_GAM
#define GAM_BITMAP_NAME "overscan.gam"
#if !defined(mb290) && !defined(mb295) /* 7020 stem db573 */
#define USE_I2C_FRONT
#define USE_STEM_RESET
#endif /* 7020 stem*/
#endif

/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library you need
 *#########################################################################*/

#if defined(mb231) || defined(mb275) || defined(mb275_64)

#elif defined(mb282b) || defined(mb314) || defined(mb361) || defined(mb382)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG

#elif defined (espresso)
#ifndef ST_OSLINUX
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG
#endif /* not ST_OSLINUX */
#if defined(ST_OSLINUX)
#define USE_INTMR
#endif /* ST_OSLINUX */


#elif defined(mb290) || defined(mb317a) || defined(mb317b)

#elif defined(mb295)
#define USE_I2C
#define USE_STDENC_I2C

#elif defined(mb376) || defined(espresso)

#elif defined(mb390)
#define USE_SCART_SWITCH_CONFIG
#define USE_I2C_FRONT

#elif defined(mb400)
#define USE_I2C_FRONT

#elif defined(mb457)
#define USE_I2C_FRONT

#elif defined(mb428)
#define USE_I2C_FRONT

#elif defined(mb436)||defined (DTT5107)
#define USE_I2C_FRONT

#elif defined (mb634)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb391)||defined (mb411)
#define USE_I2C
#elif defined (mb519)|| defined (mb618)|| defined (mb680)
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
#if defined (STVOUT_TRACE) && !defined (USE_UART)
#define USE_UART
#endif
#if defined (STVOUT_TRACE) && !defined (TRACE_UART)
#define TRACE_UART
#endif

/* STUART, STPWM, STCFG, STI2C needs PIO */
#if (defined (USE_PWM) || defined (USE_UART) || defined (USE_STCFG) || defined (USE_I2C)) && !defined (USE_PIO)
#define USE_PIO
#endif

/* STVTG, STGPDMA, STINTMR, STCLKRV need STEVT */
#if (defined (USE_VTG) || defined (USE_GPDMA) || defined (USE_INTMR) || defined (USE_CLKRV)) && !defined (USE_EVT)
#if !defined(ST_OSLINUX)
#define USE_EVT
#endif
#endif
/* STI2C can do without STEVT when STI2C_MASTER_ONLY is set,  */

/* STVTG, STVBI, STVOUT need STDENC */
#if (defined (USE_VTG) || defined (USE_VOUT) || defined (USE_VBI)) && !defined (USE_DENC)
#define USE_DENC
#endif


#endif /* #ifndef __TESTCFG_H */
