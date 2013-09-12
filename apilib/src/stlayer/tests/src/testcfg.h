/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM

*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

/* All Chips / All boards */
#ifdef ST_OSLINUX
	#define TESTTOOL_INPUT_FILE_NAME  "./scripts/layer/layertest.txt"
#else
	#define USE_TBX
	#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/layertest.txt"
#endif

#define USE_TESTTOOL
#ifndef ST_OSLINUX
#define USE_AVMEM
#endif
#define USE_VTGSIM
#define USE_DENC
#define USE_VTG
#define USE_VOUT
#define USE_VMIX
#define USE_LAYER
#define USE_API_TOOL
#ifdef ENABLE_STBLIT_MACROS
#define USE_BLIT
#endif
#if defined(ST_5100) || defined(ST_5105)|| defined(ST_5301) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107) || defined(ST_5162)
#define USE_COMPO
#define USE_DISP
#endif

#define USE_EVT

#if defined(ST_5510)|| defined(ST_5512)|| defined(ST_5508)|| defined(ST_5518) ||defined (ST_TP3)
#define USE_PIO
#define USE_PWM
#endif

#if defined(ST_5514)
#define USE_GPDMA
#define USE_STCFG
#define USE_PIO
#define USE_PWM
#endif

#if defined(ST_5528)
#ifdef ST_OS21
/* Temporay removal of STGDPMA, because of STAVMEM doesn't support GPDMA under OS21 yet */
/*USE_GPDMA*/
#endif
#ifdef ST_OS20
#define USE_GPDMA
#endif
#if !defined(ST_OSLINUX)
#define USE_PIO
#define USE_PWM
#endif
#endif /* ST_5528 */

#if defined(ST_7710)
#define USE_FDMA
#define USE_PIO
#define USE_CLKRV
#endif

#if defined(ST_7100)
/*#define USE_FDMA*/
#if !defined(ST_OSWINCE)
#define USE_PIO
#define USE_CLKRV
#define USE_I2C
#endif
#endif

#if defined(ST_7109)
/*#define USE_FDMA*/
#define USE_PIO
#define USE_CLKRV
#define USE_I2C
#endif

#if defined(ST_7200)
#define USE_CLKRV
#endif

#if defined(ST_5516)
#define USE_STCFG
#define USE_PIO
#define USE_PWM
#ifdef UNIFIED_MEMORY
#define CACHED_SIZE 0x0200000    /* 2 MBytes */
#endif
#endif

#if defined(ST_5517)
#define USE_FDMA
#define USE_STCFG
#define USE_PIO
#define USE_PWM
#ifdef UNIFIED_MEMORY
#define CACHED_SIZE 0x0200000    /* 2 MBytes */
#endif
#endif


#if defined(ST_7015) || defined(ST_GX1)  || defined(ST_NGX1)
#define USE_INTMR
#endif

#if defined(ST_7020)
#define USE_INTMR
#if !defined(mb290) && !defined(mb295) /* 7020 stem db573 */
#define USE_I2C_FRONT
#define USE_STEM_RESET
#endif /* 7020 stem*/
#endif /* ST_7020 */


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

#elif defined(mb290)

#elif defined(mb295)
#define USE_I2C
#define USE_STDENC_I2C

#elif defined(mb314) || defined(mb361) || defined(mb382)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG

#elif defined(espresso)
#ifdef ST_OSLINUX
/*#define USE_INTMR*/
#endif /* ST_OSLINUX */

#elif defined(mb317a) || defined(mb317b)|| defined(mb376) || defined(espresso) || defined(mb391)

#elif defined(mb411)
#ifdef ST_OSLINUX
/*#define USE_INTMR*/
#endif /* ST_OSLINUX */

#elif defined(mb390)

#define USE_I2C
#define USE_SCART_SWITCH_CONFIG
#define USE_I2C_FRONT
#define USE_STV6413

#if defined(ST_5301)
#ifdef STLAYER_USE_UART
#define USE_UART
#define USE_UART_AS_STDIO
#endif
#endif

#elif defined(mb428)
#ifdef STLAYER_USE_UART
#define USE_UART
#define USE_UART_AS_STDIO
#endif



#elif defined(mb400)

#elif defined(mb457)

#elif defined(mb436)

#elif defined(mb634)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb519)

#elif defined(maly3s)

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
#define USE_EVT
#endif

/* STVTG, STVBI, STVOUT need STDENC */
#if (defined (USE_VTG) || defined (USE_VOUT) || defined (USE_VBI)) && !defined (USE_DENC)
#define USE_DENC
#endif

#endif /* #ifndef __TESTCFG_H */

/* end of testcfg.h */
