/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
23 May 2002        Adapted for STVID                                AN
14 May 2003        Add support for 7020stem                         HSdLM
16 August 2004     Add support for 7710                             WA
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

#ifdef ST_OSLINUX
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/cc/default.txt"
#else
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.txt"

#define USE_EVT
#define USE_TBX
#define USE_PIO
/*#define DUMP_REGISTERS*/
/*#define USE_PWM*/
/*#define USE_CLKRV*/
#define USE_UART
#define USE_PROCESS_TOOL
#endif

#define USE_TESTTOOL
#define USE_API_TOOL
#define USE_DENC
#define USE_EVT
#define USE_VTG
#define USE_LAYER
#define USE_VBI
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
#define USE_VIDEO_2



#if !defined(ST_OSLINUX)
#define USE_AVMEM
#endif /* ST_OSLINUX */

#ifndef STPTI_UNAVAILABLE
#define USE_PTI     /* Transport (PTI / LINK / STPTI) */
#define USE_CLKRV
#endif

/*#define DUMP_REGISTERS*/
/*#define TRACE_UART */

#if defined(ST_5514)
#define USE_STCFG
#define USE_GPDMA
#endif /* #if ST_5514 */

#if defined(ST_5516)
#define USE_STCFG
#endif /* #if ST_5516 */

#if defined(ST_5517)
#define USE_STCFG
#define USE_FDMA
#endif /* #if ST_5517 */

#if defined(ST_5528)
#define USE_STCFG
#define USE_GPDMA
#endif /* #if ST_5528 */

#if defined(ST_7015)
#define USE_INTMR
#endif /* 7015 */

#if defined(ST_7020)
#define USE_INTMR
#if !defined(mb290) && !defined(mb295) /* 7020 stem db573 */
#define USE_I2C_FRONT
#define USE_STEM_RESET
#endif /* 7020 stem*/
#endif /* ST_7020 */

#if defined(ST_5100)
#define USE_PIO         /* HSdLM 21-08-03 9:22AM: to be confirmed */
#define USE_PWM         /* HSdLM 21-08-03 9:22AM: to be confirmed */
#define USE_DISP_GDMA   /* HSdLM 21-08-03 9:22AM: to be developped in stapigat */
#define USE_FDMA        /* WA    01-06-04 3:09PM: to be developped in stapigat */
#endif /* 5100 */

#if defined (ST_5301)||defined (ST_5105)||defined (ST_5107)
#define USE_DISP_GDMA
#define USE_FDMA
#endif /*5301*/

#if defined(ST_7710)
#define USE_PIO          /* WA 16-08-04 10:22AM: to be confirmed */
#define USE_PWM          /* WA 16-08-04 10:22AM: to be confirmed */
#define USE_FDMA         /* WA 16-08-04 10:22AM: to be confirmed */
/*#define USE_INTMR*/
#define USE_I2C_FRONT
#define USE_CLKRV
#endif /* ST_7710 */

#if defined(ST_7100)|| defined(ST_7109)
#if !defined(ST_OSLINUX)
#define USE_I2C_FRONT
#endif
#define USE_FDMA
#define USE_CLKRV
#endif /*7100*/

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
#define USE_TSMUX

#elif defined(mb295)
#define USE_I2C
#define USE_STDENC_I2C

#elif defined(mb314) || defined(mb361) || defined(mb382)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb376) || defined(espresso)
#define USE_I2C

#elif defined(mb390)
#if defined(ST_5100)
#define USE_I2C
#endif

#elif defined(mb391)
#define USE_I2C
#define USE_MERGE

#elif defined(mb411)
#if !defined(ST_OSLINUX)
/*#define USE_I2C
#define USE_MERGE*/
#endif  /* ST_OSLINUX */

#elif defined(mb400)

#elif defined(mb436)

#else
#error ERROR:no DVD_PLATFORM defined
#endif

#endif /* #ifndef __TESTCFG_H */
