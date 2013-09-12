/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
13 May 2003        Add support for 7020stem                         HSdLM
10 Jun 2003        Add support for 5528                             HSdLM
21 Jan 2004        Add support for espresso                         AC
12 Jul 2004        Add support on mb391/7710                        TA
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

/* All chips */

#ifdef ST_OSLINUX
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/vmix/default.mac"



#else
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.mac"

#define USE_TBX
#define DUMP_REGISTERS
#endif

#define USE_EVT
#define USE_TESTTOOL
#if !defined(ST_OSLINUX)
#define USE_AVMEM
#endif
#define USE_DENC
#define USE_VTGSIM
#define USE_VTG
#define USE_VOUT
#define USE_VMIX
#define USE_LAYER
#define USE_API_TOOL

#if !defined(ST_5100) && !defined(ST_5105) && !defined(ST_5301) && !defined(ST_5188) && !defined(ST_5525) && !defined(ST_5107)\
 && !defined(ST_5162)
#define USE_DISP_VIDEO
#endif


#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) ||defined (ST_TP3)
#define USE_PIO
#define USE_PWM
#endif /* ST_5510 ST_5512 ST_5508 ST_5518*/

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
#define USE_STCFG
#define USE_PIO
#define USE_PWM
#endif /* ST_5514 ST_5516 ST_5517 */

#if defined(ST_5528)
#if !defined(ST_OSLINUX)
#define USE_PIO
#define USE_PWM
#endif
#endif /* ST_5528 */

#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
#define USE_PIO
#define USE_CLKRV
#endif /* ST_7710 - ST_7100 */

#if defined(ST_7200)
#define USE_CLKRV
#endif /* ST_7200 */

#if defined(ST_7015) || defined(ST_GX1)
#define USE_INTMR
#endif /* ST_7015 ST_GX1 */

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

#elif defined(mb314) || defined(mb361) || defined(mb382) || defined (espresso)
#if !defined(ST_OSLINUX)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG
#endif /* not ST_OSLINUX */

#elif defined(mb376)

#elif defined(mb317a) || defined(mb317b)

#elif defined (mb390)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG
#define USE_I2C_FRONT
#define USE_STV6413
#if defined(ST_5301)
#ifdef STVMIX_USE_UART
#define USE_UART
#define USE_UART_AS_STDIO
#endif
#endif

#elif defined(mb391)

#elif defined(mb400)

#elif defined(mb436)

#elif defined(mb634)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG


#elif defined(SAT5107)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb428)

#elif defined(maly3s)

#ifdef STVMIX_USE_UART
#define USE_UART
#define USE_UART_AS_STDIO
#endif

#elif defined(mb457)

#elif defined(mb411)

#elif defined(mb519)

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
#if !defined(ST_OSLINUX)  /* Not for Linux */
#define USE_UART
#endif
#endif

/* STUART, STPWM, STCFG, STI2C needs PIO */
#if (defined (USE_PWM) || defined (USE_UART) || defined (USE_STCFG) || defined (USE_I2C)) && !defined (USE_PIO)
#if !defined(ST_OSLINUX)
#define USE_PIO
#endif
#endif

/* STVTG, STGPDMA, STINTMR, STI2C, STCLKRV need STEVT */
#if (defined (USE_VTG) || defined (USE_GPDMA) || defined (USE_INTMR) || defined (USE_I2C) || defined (USE_CLKRV)) && !defined (USE_EVT)
#define USE_EVT


#endif

/* STVTG, STVBI, STVOUT need STDENC */
#if (defined (USE_VTG) || defined (USE_VOUT) || defined (USE_VBI)) && !defined (USE_DENC)
#define USE_DENC
#endif


#endif /* #ifndef __TESTCFG_H */




