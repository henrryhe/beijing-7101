/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
17 May 2002                                                         TM
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H


#define USE_TESTTOOL
#ifndef ST_OSLINUX
#define USE_TBX
#define USE_AVMEM
#endif
#define USE_API_TOOL

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library you need
 *#########################################################################*/

#if defined(ST_5510)


#elif defined(ST_5512)


#elif defined(ST_5508)


#elif defined(ST_5518)


#elif defined(ST_5514)
#define USE_GPDMA

#elif defined(ST_5516)

#elif defined(ST_7015) || defined(ST_7020)

#elif defined(ST_GX1) || defined(ST_NGX1)

#elif defined(ST_5528)

#elif defined(ST_5100)

#elif defined(ST_5105)|| defined(ST_5107)

#elif defined(ST_5188)

#elif defined(ST_5301)

#elif defined(ST_7710)|| defined(ST_7100)|| defined(ST_7109)

#else
#error ERROR:no DVD_FRONTEND defined
#endif


/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library you need
 *#########################################################################*/

#if defined(mb231)

#elif defined(mb282b)
/*#define USE_I2C*/
/*#define USE_STV6410*/
/*#define USE_HDD*/

#elif defined(mb275)
/*#define USE_HDD*/

#elif defined(mb275_64)
/*#define USE_HDD*/

#elif defined(mb295)
/*#define USE_I2C*/

#elif defined(mb314)
/*#define USE_I2C*/
/*#define USE_STV6410*/

#elif defined(mb361)
/*#define USE_I2C*/
/*#define USE_STV6410*/

#elif defined(mb317) || defined(mb317a) || defined(mb317b)
/*#define USE_SAA7114*/

#elif defined(espresso)

#elif defined(mb382)

#elif defined(mb390)

#elif defined(mb391)

#elif defined(mb411)

#elif defined(mb400)

#elif defined(mb457)

#elif defined(mb436)

#else
#error ERROR:no DVD_PLATFORM defined
#endif

/* STGPDMA, STINTMR, STCLKRV need STEVT */
#if (defined (USE_GPDMA) || defined (USE_INTMR) || defined (USE_CLKRV)) && !defined (USE_EVT)
#define USE_EVT
#endif



#endif /* #ifndef __TESTCFG_H */

