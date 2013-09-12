/*******************************************************************************

File name   : genadd.h

Description : Constants and exported functions

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                      Name
----               ------------                                      ----
23 July 02         Created                                           BS
14 Oct 2002        Add support for 5517 and 5578                     HSdLM
14 May 2003        Add support for STi5528                           HSdLM
27 Aug 2003        Add support for STi5100                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __GENADD_H
#define __GENADD_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Macros and Defines -------------------------------------------- */

#ifdef REMOVE_GENERIC_ADDRESSES

#if defined(USE_AS_FRONTEND) && defined(USE_AS_BACKEND)
#error This file cannot be defined as both for FRONTEND & BACKEND
#endif

#ifdef USE_AS_FRONTEND

#ifdef STB_FRONTEND_TP3
#define USE_TP3_GENERIC_ADDRESSES
#endif /* STB_FRONTEND == TP3 */

#ifdef STB_FRONTEND_5508
#define USE_5508_GENERIC_ADDRESSES
#endif /* STB_FRONTEND == 5508 */

#ifdef STB_FRONTEND_5510
#define USE_5510_GENERIC_ADDRESSES
#endif /* STB_FRONTEND == 5510 */

#ifdef STB_FRONTEND_5512
#define USE_5512_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5512 */

#ifdef STB_FRONTEND_5518
#define USE_5518_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5518 */

#ifdef STB_FRONTEND_5578
#define USE_5578_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5578 */

#ifdef STB_FRONTEND_5580
#define USE_5580_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5580 */

#ifdef STB_FRONTEND_7015
#define USE_7015_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_7015 */

#ifdef STB_FRONTEND_GX1
#define USE_GX1_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_GX1 */

#ifdef STB_FRONTEND_5514
#define USE_5514_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5514 */

#ifdef STB_FRONTEND_5528
#define USE_5528_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5528 */

#ifdef STB_FRONTEND_5516
#define USE_5516_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5516 */

#ifdef STB_FRONTEND_5517
#define USE_5517_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5517 */

#ifdef STB_FRONTEND_5100
#define USE_5100_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_5100 */

#ifdef STB_FRONTEND_7020
#define USE_7020_GENERIC_ADDRESSES
#endif /* STB_FRONTEND_7020 */

#endif /* USE_AS_FRONTEND */


#ifdef USE_AS_BACKEND

#ifdef STB_BACKEND_TP3
#define USE_TP3_GENERIC_ADDRESSES
#endif /* STB_BACKEND_TP3 */

#ifdef STB_BACKEND_5508
#define USE_5508_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5508 */

#ifdef STB_BACKEND_5510
#define USE_5510_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5510 */

#ifdef STB_BACKEND_5512
#define USE_5512_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5512 */

#ifdef STB_BACKEND_5518
#define USE_5518_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5518 */

#ifdef STB_BACKEND_5578
#define USE_5578_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5578 */

#ifdef STB_BACKEND_5580
#define USE_5580_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5580 */

#ifdef STB_BACKEND_7015
#define USE_7015_GENERIC_ADDRESSES
#endif /* STB_BACKEND_7015 */

#ifdef STB_BACKEND_GX1
#define USE_GX1_GENERIC_ADDRESSES
#endif /* STB_BACKEND_GX1 */

#ifdef STB_BACKEND_5514
#define USE_5514_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5514 */

#ifdef STB_BACKEND_5528
#define USE_5528_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5528 */

#ifdef STB_BACKEND_5516
#define USE_5516_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5516 */

#ifdef STB_BACKEND_5517
#define USE_5517_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5517 */

#ifdef STB_BACKEND_5100
#define USE_5100_GENERIC_ADDRESSES
#endif /* STB_BACKEND_5100 */

#ifdef STB_BACKEND_7020
#define USE_7020_GENERIC_ADDRESSES
#endif /* STB_BACKEND_7020 */

#endif /* USE_AS_BACKEND */

#endif /* REMOVE_GENERIC_ADDRESSES */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __GENADD_H */
