/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
 *
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H

#ifdef ST_OSLINUX
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/vin/default.com"
#else  /*ST_OSLINUX*/
#define TESTTOOL_INPUT_FILE_NAME  "../../../scripts/default.com"
#endif  /*ST_OSLINUX*/
/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

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

#if defined(ST_5510)
/*#define USE_UART*/
/*#define TRACE_UART*/
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_PWM
#define USE_CLKRV
#define USE_EVT
#define USE_DENC
#define USE_VTG
#define USE_LAYER
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
/*#define USE_VIN*/
/*#define USE_AUDIO*/
/*#define USE_PTI*/     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_5512)
/*#define USE_UART */
/*#define TRACE_UART*/
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_PWM
#define USE_CLKRV
#define USE_EVT
#define USE_DENC
#define USE_VTG
#define USE_LAYER
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
/*#define USE_VIN*/
/*#define USE_AUDIO*/
/*#define USE_PTI*/     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_5508)
/*#define USE_UART */
/*#define TRACE_UART*/
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_PWM
#define USE_CLKRV
#define USE_EVT
#define USE_DENC
#define USE_VTG
#define USE_LAYER
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
/*#define USE_VIN*/
/*#define USE_AUDIO*/
/*#define USE_PTI*/     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_5518)
/*#define USE_UART*/
/*#define TRACE_UART*/
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_PWM
#define USE_CLKRV
#define USE_EVT
#define USE_DENC
#define USE_VTG
#define USE_LAYER
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
/*#define USE_VIN*/
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_5514)
  #if defined (mb290)
  #define USE_INTMR
  #define DUMP_REGISTERS
  #define USE_API_TOOL
  #define USE_TESTTOOL
  #define USE_VIDEO
  #define USE_VIN
  #define USE_VMIX
  #define USE_VOUT
  #define USE_VIN
  #define USE_PTI
  #endif
#define USE_GPDMA
/*#define USE_STCFG*/ /* needed for video injection  */
/*#define USE_UART*/
/*#define TRACE_UART*/
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_PWM
#define USE_CLKRV
#define USE_EVT
#define USE_DENC
#define USE_VTG
#define USE_LAYER
/*#define USE_VMIX*/
/*#define USE_VOUT*/
/*#define USE_VIDEO*/
/*#define USE_VIN*/
/*#define USE_AUDIO*/
/*#define USE_PTI*/     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_7015)
#define USE_INTMR
/*#define USE_UART*/
#define TRACE_UART
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_PWM
#define USE_CLKRV
#define USE_EVT
#define USE_DENC
#define USE_VTG
#define USE_LAYER
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
#define USE_VIDEO_2
#define USE_VIN
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */
#define DUMP_REGISTERS
#define USE_API_TOOL
#define USE_TESTTOOL

#elif defined(ST_7020)
#define USE_INTMR
/*#define USE_UART*/
#define TRACE_UART
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_PWM
#define USE_CLKRV
#define USE_EVT
#define USE_DENC
#define USE_VTG
#define USE_LAYER
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
#define USE_VIDEO_2
#define USE_VIN
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */
#define DUMP_REGISTERS
#define USE_API_TOOL
#define USE_TESTTOOL

#elif defined(ST_GX1) || defined(ST_NGX1)
#define USE_INTMR
/*#define USE_UART */
/*#define TRACE_UART*/
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_DENC
#define USE_VTG
#define USE_LAYER
#define USE_EVT
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
#define USE_VIDEO_2
#define USE_VIN
#define DUMP_REGISTERS
#define USE_API_TOOL
#define USE_TESTTOOL
#elif defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
/*#define USE_UART  */
/*#define TRACE_UART*/
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_DENC
#define USE_VTG
#define USE_CLKRV
#define USE_LAYER
#define USE_EVT
#define USE_VMIX
#define USE_VOUT
#define USE_VIDEO
#define USE_VIN
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */
#if defined(ST_7100) || defined(ST_7109)
/*#define USE_DISP_VIDEO*/
/*#define USE_DISPVID */
/*#define DUMP_REGISTERS*/
#endif /*ST_7100||ST_7109*/
#define USE_API_TOOL
#define USE_TESTTOOL
#if !defined(STUB_INJECT)
#define USE_FDMA
#endif  /* STUB_INJECT */

#else
#error ERROR:no DVD_FRONTEND defined
#endif

/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library you need
 *#########################################################################*/

#if defined(mb231)

#elif defined(mb382) /* Case 7020 STEM on mb382 board */
#define USE_STEM_RESET
#define USE_TSMUX
#define USE_I2C
#define USE_I2C_FRONT
#define USE_STV6413
#define USE_SCART_SWITCH_CONFIG
#define USE_EXTVIN

#elif defined(mb282b)
#define USE_I2C
#define USE_STV6410
/*#define USE_HDD*/

#elif defined(mb275)
/*#define USE_HDD*/

#elif defined(mb275_64)
/*#define USE_HDD*/

#elif defined(mb295)
#define USE_I2C
#define USE_EXTVIN  /* available for mb295 */

#elif defined(mb290)
#define USE_I2C
#define USE_EXTVIN

#elif defined(mb391)
#define USE_I2C
#define USE_I2C_FRONT
#define USE_EXTVIN

#elif defined(mb411)
#if !defined(ST_OSLINUX)
#define USE_I2C
#define USE_EXTVIN
#endif /*!ST_OSLINUX*/

#elif defined(mb314)
#define USE_I2C
#define USE_STV6410

#elif defined(mb317) || defined(mb317a) || defined(mb317b)
#define USE_SAA7114
#define INIT_TERM_VIDEO
#else
#error ERROR:no DVD_PLATFORM defined
#endif


#ifdef ST_OSLINUX
#define USE_REG

#if defined(USE_UART)
#undef USE_UART
#endif /*USE_UART*/

#if defined(USE_TBX)
#undef USE_TBX
#endif /*USE_TBX*/

#if defined(USE_AVMEM)
#undef USE_AVMEM
#endif /*USE_AVMEM*/

#endif  /*ST_OSLINUX*/

#endif /* #ifndef __TESTCFG_H */

