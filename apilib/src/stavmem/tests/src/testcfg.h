/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
10 Apr 2002        Introduced for STAVMEM                           HSdLM
20 Nov 2002        Add 5517 support                                 HSdLM
02 Apr 2003        Add mb376/5528 support                           HSdLM
21 Aug 2003        Add mb390/5100 support                           HSdLM
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

/* All chips */
#if !defined(ST_OSLINUX)
#define USE_TBX
#endif /* ! ST_OSLINUX */

#define USE_TESTTOOL
#define USE_API_TOOL
/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library is needed
 *#########################################################################*/

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518)

#elif defined(ST_5514) || defined(ST_5528) || defined(espresso)

#ifdef STAVMEM_MEM_ACCESS_GPDMA
#define USE_EVT
#define USE_GPDMA
#endif /* #ifdef STAVMEM_MEM_ACCESS_GPDMA */

#elif defined(ST_5516)

#elif defined(ST_5517)|| defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301) \
   || defined(ST_7109)|| defined(ST_5525)|| defined(ST_5188)|| defined(ST_5107) || defined(ST_7200)
#ifdef STAVMEM_MEM_ACCESS_FDMA
#define USE_FDMA
#endif /* #ifdef STAVMEM_MEM_ACCESS_FDMA */
#endif

#if defined(ST_7020)
#if !defined(mb290) && !defined(mb295) /* 7020 stem db573 */
#define USE_I2C_FRONT
#define USE_STEM_RESET
#endif /* 7020 stem*/
#endif

#if defined(ST_GX1)
#endif

/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library is needed
 *#########################################################################*/

#if defined(mb231) || defined(mb282b) || defined(mb275) || defined(mb275_64)

#elif defined(mb295) || defined(mb290) || defined(mb391)

#elif defined(mb314)|| defined(mb361)|| defined(mb382)|| defined(mb376)|| defined(mb390)  || defined(espresso)|| defined(mb400)\
   || defined(mb411)|| defined(mb428)|| defined(mb457)|| defined(mb436)|| defined(DTT5107)|| defined(maly3s) || defined(mb519)

#elif defined(mb317a) || defined(mb317b)

#elif defined(espresso)

#else
#error ERROR:no DVD_PLATFORM defined
#endif

/* ***********************************************************************
 *                        library inter dependencies
 * ***********************************************************************/

/* USE_SCART_SWITCH_CONFIG, USE_STDENC_I2C, USE_STEM_RESET needs USE_I2C */
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
