/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

/* All chips */
#ifdef ST_OSLINUX
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/hdmi/default.mac"
#else
#define USE_TBX
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.mac"
#endif

#define USE_DENC
#define USE_EVT
#define USE_VTG
#define USE_VOUT
#define USE_VMIX
#define USE_API_TOOL
#define USE_VIDEO
#define USE_VIDEO_2
#define USE_HDMI
#define USE_PIO

#if defined(ST_7710)||defined(ST_7100)||defined(ST_7109)|| defined(ST_7200)
#if defined (ST_7710) || defined(ST_OSLINUX)
#define USE_PTI
#endif /* ST_OSLINUX */
#define USE_AUDIO
#define USE_TESTTOOL
#endif

#if defined (mb411)|| defined(mb519)
#define USE_LXLOADER
#endif

#define USE_CLKRV
#define USE_DISP_VIDEO
#define USE_LAYER

#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109)|| defined (ST_7200)
#if !defined(ST_OSLINUX)
#ifndef ST_7200
#define USE_PWM
#else
#define USE_VTGSIM
#endif
/*#ifndef STPTI_UNAVAILABLE
#define USE_CLKRV
#endif                   */
#define USE_FDMA
#define USE_DISP_GAM
#define USE_AVMEM
#endif

#else
#error ERROR:no DVD_FRONTEND defined
#endif

/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library you need
 *#########################################################################*/


#if defined(mb391) || defined(mb411)|| defined (mb519)
#define USE_I2C
#else
#error ERROR:no DVD_PLATFORM defined
#endif


/* ***********************************************************************
 *                        library inter dependencies
 * ***********************************************************************/

/* USE_SCART_SWITCH_CONFIG, USE_STEM_RESET needs USE_I2C */
#if defined (USE_SCART_SWITCH_CONFIG) && !defined (USE_I2C)
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

