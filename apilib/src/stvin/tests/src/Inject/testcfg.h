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

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

#if defined(ST_5510)
#define USE_UART
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
#define USE_VIDEO_2
/*#define USE_VIN*/
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_5512)
#define USE_UART
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
#define USE_VIDEO_2
/*#define USE_VIN*/
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_5508)
#define USE_UART
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
#define USE_VIDEO_2
/*#define USE_VIN*/
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_5518)
#define USE_UART
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
#define USE_VIDEO_2
/*#define USE_VIN*/
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_5514)
#define USE_GPDMA
#define USE_STCFG   /* Needed to something on the output */
#define USE_UART
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
#define USE_VIDEO_2
/*#define USE_VIN*/
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_7015) || defined(ST_7020)
#define USE_INTMR
#define USE_UART
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
#define USE_VIDEO_2
/* #define USE_VIN */
/*#define USE_AUDIO*/
#define USE_PTI     /* ------- Transport (PTI / LINK / STPTI ) ------ */

#elif defined(ST_GX1)
#define USE_UART
/*#define TRACE_UART*/
#define USE_TBX
#define USE_PIO
#define USE_AVMEM
#define USE_DENC
#define USE_VTG
#define USE_LAYER
#define USE_EVT
#define USE_VMIX    /* not yet ported on ST40GX1 */
#define USE_VOUT    /* not yet ported on ST40GX1 */
/* #define USE_VIDEO */  /* not yet ported on ST40GX1 */
/*#define USE_VIN*/     /* not yet ported on ST40GX1 */

#else
#error ERROR:no DVD_FRONTEND defined
#endif


/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library you need
 *#########################################################################*/

#if defined(mb231)

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
#define USE_STDENC_I2C
/* #define USE_EXTVIN */ /* available for mb295 */

#elif defined(mb314)
#define USE_I2C
#define USE_STV6410

#elif defined(mb317)
/*#define USE_EXTVIN*/  /* available for mb317 */

#else
#error ERROR:no DVD_PLATFORM defined
#endif

#endif /* #ifndef __TESTCFG_H */
