/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
06 Fev 2002        Created                                          TM
 *
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

#define USE_BLIT

#define USE_TESTTOOL
#ifndef ST_OSLINUX
#define USE_TBX
#endif

#define USE_EVT

#ifndef ST_OSLINUX
#define USE_AVMEM
#endif


#define USE_API_TOOL

#ifdef STBLIT_ENABLE_BASIC_TRACE
#define STBLIT_ENABLE_UART_TRACE
#endif

#ifdef STBLIT_ENABLE_UART_TRACE
#define USE_UART
#define TRACE_UART
#endif

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/


#if defined(ST_5514) && !defined(mb290)
#define USE_GPDMA
#endif

#if defined(ST_5517) || defined(ST_5100) || defined (ST_5105) || defined (ST_5188) || defined (ST_5107)
#define USE_FDMA
#endif

#if defined(ST_7015) || defined(ST_GX1) || defined(ST_NGX1)
#define USE_INTMR
#endif

#if defined(ST_5528) || defined(ST_7710) || defined(ST_7100)
/*#define USE_GPDMA*/
#endif

#if defined(ST_7020)
#define USE_INTMR
#if !defined(mb290) && !defined(mb295) /* 7020 stem db573 */
#define USE_STEM_RESET
#define USE_I2C_FRONT
#endif /* 7020 stem*/
#endif /* ST_7020 */

/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library you need
 *#########################################################################*/

#if defined(mb231)

#elif defined(mb282b)

#elif defined(mb275)

#elif defined(mb275_64)

#elif defined(mb295)

#elif defined(mb290)

#elif defined(mb314)

#elif defined(mb361)

#elif defined(mb382)

#elif defined(mb376) || defined(espresso)

#elif defined(mb390)

#elif defined(mb391)

#elif defined(mb400)

#elif defined(maly3s)

#elif defined(mb436)

#elif defined(mb428)

#elif defined(mb457)

#elif defined(mb411)

#elif defined(mb519)

#elif defined(mb317) || defined(mb317a) || defined(mb317b)
/*#define USE_SAA7114*/
#elif defined(mediaref)
#else
#error ERROR:no DVD_PLATFORM defined
#endif


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

