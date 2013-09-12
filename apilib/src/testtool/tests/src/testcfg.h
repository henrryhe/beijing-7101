/*****************************************************************************

File name   : TESTCFG.H

Description : module for specific configuration

COPYRIGHT (C) ST-Microelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
07 Sep 2001        Created                                          HSdLM
10 Apr 2002        Introduced for STTBX                             HSdLM
12 Dec 2002        Add support of 5517                              HSdLM
*****************************************************************************/

#ifndef __TESTCFG_H
#define __TESTCFG_H
#if defined(ST_OSLINUX)
#define TESTTOOL_INPUT_FILE_NAME  "./scripts/testtool/default.mac"
#else
#define TESTTOOL_INPUT_FILE_NAME "../../../scripts/metrics.mac"
#endif
#define TESTTOOL_TESTS

/*#########################################################################
 *                     Chip specific - library dependencies
 *                       Choose what library is needed
 *#########################################################################*/
#if !defined(ST_OSLINUX)
#define USE_TBX
#endif

#if !(defined(ST_5510)|| defined(ST_5512)|| defined(ST_5508)|| defined(ST_5518)|| defined(ST_5528)|| defined(ST_5100) \
   || defined(ST_5105)|| defined(ST_5301)|| defined(ST_5514)|| defined(ST_5516)|| defined(ST_5517)|| defined(ST_7015) \
   || defined(ST_7020)|| defined(ST_GX1) || defined(ST_7710)|| defined(ST_7100)|| defined(ST_7109)|| defined(ST_5525) \
   || defined(ST_5188)|| defined(ST_5107) || defined(ST_7200))
#error ERROR:no DVD_FRONTEND defined
#endif


/*#########################################################################
 *                     Board specific - library dependencies
 *                          Choose what library is needed
 *#########################################################################*/

#if !(defined(mb231) || defined(mb282b)|| defined(mb275) || defined(mb275_64)|| \
      defined(mb290) || defined(mb295) || defined(mb376) || defined(mb390)   || defined(mb400) || defined(mb457) || \
      defined(mb314) || defined(mb361) || defined(mb382) || defined(mb411)   || defined(mb391) || defined(mb428) || \
      defined(mb317a)|| defined(mb317b)|| defined(mb436) || defined(DTT5107) || defined(mb519) )
#error ERROR:no DVD_PLATFORM defined
#endif

#endif /* #ifndef __TESTCFG_H */
