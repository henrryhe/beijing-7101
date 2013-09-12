/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
23 May 2002        Adapted for STVID                                AN
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/
#ifdef ST_OSLINUX
#define USE_REG                         /* for poke (not in Testtool) */

#if defined(STVID_USE_SPLITTED_SCRIPTS)
#define TESTTOOL_INPUT_FILE_NAME  "./sscripts/vid/default.com"
#else
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/vid/default.com"
#endif

#else
#if defined(STVID_USE_SPLITTED_SCRIPTS)
#define TESTTOOL_INPUT_FILE_NAME  "../../../sscripts/default.com"
#else
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.com"
#endif
#define USE_PROCESS_TOOL
#define USE_TBX

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5301) && !defined(ST_5188) && !defined(ST_5525) \
 && !defined(ST_7200) && !defined(ST_5162)
/* No pwm on STi5105, STi5301 and 5162 platforms */
#define USE_PWM
#endif

#define USE_AVMEM

#endif  /* ST_OSLINUX */

#ifdef USE_COLOR_TRACES
/* Print STTBX message in red using ANSI Escape code */
/* Need a terminal supporting escape codes           */
#define STTBX_ErrorPrint(x)  \
{ \
    STTBX_Print(("\033[31m")); \
    STTBX_Print(x); \
    STTBX_Print(("\033[0m")); \
}
#else
#define STTBX_ErrorPrint(x)  STTBX_Print(x)
#endif

#define USE_PIO

/* Increase max number of symbols usable in testtool */
#define TESTTOOL_MAX_SYMBOLS    3000

/* Flags temporary removed for first video snap on STi7710  */
/* Should be enabled again asap.                            */
#if !defined(STPTI_UNAVAILABLE) && (!defined(NATIVE_CORE))
#if defined(ST_5188) || defined(ST_8010)
#define USE_DEMUX
#else
#define USE_PTI     /* Transport (PTI / LINK / STPTI) */
#endif /* ST_5188 || ST_8010 */
#endif /* STPTI_UNAVAILABLE */

#ifndef STCLKRV_UNAVAILABLE
#define USE_CLKRV
#endif /* STCLKRV_UNAVAILABLE */

#define USE_TESTTOOL
#define USE_API_TOOL
/* To allow memory injection without PTI, add #define STPTI_UNAVAILABLE in video test files */

#define USE_EVT
#define USE_DENC

#if defined(STUB_VTG)
    #define USE_VTGSTUB
#else
    #define USE_VTG
#endif

#define USE_LAYER
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
#define USE_VIDEO_2
#ifdef VALID_TOOLS
#define USE_VALID_TRACE
#else
#ifndef NATIVE_CORE
/* can't use DUMP_REGISTER with VSOC (compilation issue) */
#define DUMP_REGISTERS
#endif
#endif /* VALID_TOOLS */

#if !defined(ST_OSLINUX)
#ifdef STVID_TRACE_BUFFER_UART
#if (STVID_TRACE_BUFFER_UART==1)
#define USE_UART
#define TRACE_UART
#elif (STVID_TRACE_BUFFER_UART==2)
#define USE_UART
#define TRACE_UART
#define INTRUSIVE_TRACES
#endif
#endif

#if defined(STVID_USE_UART_AS_STDIO)
#if !defined(USE_UART)
#define USE_UART
#endif
#define USE_UART_AS_STDIO
#endif
#endif  /* ST_OSLINUX */

#if defined(ST_5512)
#endif /* 5512 */

#if defined(ST_5514)
#define USE_GPDMA
#define USE_STCFG
#define USE_TSMUX
#endif /* 5514 */

#if defined(ST_5516) || defined(ST_5517)
#define USE_STCFG
#endif /* 5516 || 5517 */

#if defined(ST_7015) || defined(ST_7020)
#define USE_INTMR
#endif /* 7015 */

#if defined(ST_5528)
#if !defined(ST_OSLINUX)
#define USE_GPDMA
#if defined(ST_7020) && defined(mb376)
#define USE_STCFG
#endif
#endif
#endif

#if defined(ST_51xx_Device)|| defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
#if !defined(STUB_INJECT)
#define USE_FDMA
#endif  /* STUB_INJECT */
#endif

#if defined(ST_7020) && (defined(mb382) || defined(mb314) || defined(mb376))
#define USE_STEM_RESET
#define USE_EXTVIN
#define USE_VIN
#endif

#if defined(ST_7020) && defined(mb382)
#define USE_TSMUX
#endif

#ifdef STVID_USE_STBLIT_TEST_MACROS
#define USE_BLIT
#endif

/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library you need
 *#########################################################################*/

#if defined(mb231)

#elif defined(mb282b)
#define USE_I2C
#define USE_STV6410
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb275) || defined(mb275_64)

#elif defined(mb290)
#define USE_I2C
#define USE_EXTVIN
#define USE_VIN

#elif defined(mb390)|| defined(mb428)
/* Not yet available for STi5301(first snapshot) */

#if defined(ST_5100)
#define USE_I2C
#define USE_I2C_FRONT
#define USE_STV6413
#define USE_SCART_SWITCH_CONFIG
#endif /* ST_5100 */

#elif defined(mb400)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb436)

#elif defined(mb634)
#define USE_I2C
#define USE_SCART_SWITCH_CONFIG


#elif defined(maly3s)

#elif defined(mb457)

#elif defined(SAT5107)
#define USE_I2C
#define USE_I2C_BACK
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb391)
#define USE_I2C
#define USE_I2C_FRONT
#define USE_EXTVIN
#define USE_VIN

#elif defined(mb295)
#define USE_I2C
#define USE_STDENC_I2C
#define USE_EXTVIN
#define USE_VIN

#elif defined(mb314)
#define USE_I2C
#define USE_I2C_FRONT
#define USE_STV6410
#define USE_SCART_SWITCH_CONFIG

#elif defined(mb317a) || defined(mb317b)
/*#define USE_EXTVIN*/  /* available for mb317 */

#elif defined(mb361) || defined(mb382)
#define USE_I2C
#define USE_I2C_FRONT
#define USE_STV6413
#define USE_SCART_SWITCH_CONFIG

#elif defined (mb376) || defined(espresso)
#define USE_I2C_FRONT
#define USE_I2C
/* #define USE_VIN */

#elif defined (mb411)
#define USE_LXLOADER

#elif defined (mb519)
#define USE_LXLOADER

#else
#error ERROR:no DVD_PLATFORM defined
#endif

#ifdef ST_OSWINCE
#undef USE_PIO   // PIO managed in console IO
#undef USE_PWM	  // link to PIO Mng
#endif

#endif /* #ifndef __TESTCFG_H */
