/************************************************************************
File Name   : stdevice.h

Description : System-wide devices header file.
              This file defines the complete system hardware & software
              configuration. It does this by including a board-specific
              configuration file and a software specific configuration file.

Copyright (C) 2000 STMicroelectronics

Reference   :

************************************************************************/

#ifndef __STDEVICE_H
#define __STDEVICE_H

/*
 * Select target evaluation platform.
 * At least one of these must be set at compile time. If the
 * user has not set the environment variables then the build
 * system will select mb231.
 */

/* Same board, different memory configuration */
#if defined(mb5518um)
	#define mb5518
#endif

/* Same board, different chip cuts and memory configuration */
#if defined(mb314_21) || defined(mb314_um) || defined(mb314_21_um)
    #define mb314
#endif

#if defined(mb231)        /* MB231 Evaluation Platform for STi5510 */
#include "mb231.h"
#elif defined(mb282)     /* MB282 Evaluation Platform for STi5512 */
#include "mb282.h"
#elif defined(mb282b)     /* MB282 Evaluation Platform for STi5512 */
#include "mb282b.h"
#elif defined(mb275) || defined(mb275_64) /* MB275 Evaluation Platform for STi5508 */
#include "mb275.h"
#elif defined(mb193) /* MB193 Evaluation Platform for ST20TP3 */
#include "mb193.h"
#elif defined(mb295) || defined(MB295)     /* MB295 Evaluation Platform for ST20TP3+7015 */
#include "mb295.h"
#elif defined(mb290)  /* MB290 Evaluation Platform for STi5514+7020 */
#include "mb290.h"
#elif defined(mb5518) /* MB5518 Evaluation Platform for STi5518 */
#include "mb5518.h"
#elif defined(mb317a) || defined(mb317b) /* MB317 Evaluation Platform for ST40GX1 */
#include "mb317.h"
#elif defined(mb314)  /* MB314 Evaluation Platform for STi5514 */
#include "mb314.h"
#elif defined(mediaref)
    #if defined(ST_5514)
        #include "mb314.h"
    #elif defined(ST_GX1) || defined(ST_NGX1)
        #include "mb317.h"
    #endif
#elif defined(mb361)  /* MB361 Evaluation Platform for STi5516/17 */
#include "mb361.h"
#elif defined(mb382)  /* MB382 Evaluation Platform for STi5517 */
#include "mb382.h"
#elif defined(mb376)  /* MB376 Evaluation Platform for STi5528 */
#include "mb376.h"
#elif defined(espresso)  /* Espresso Evaluation Platform for STi5528 */
#include "espresso.h"
#elif defined(mb390)  /* MB390 Evaluation Platform for STi5100/5101/5301 */
#include "mb390.h"
#elif defined(mb391)  /* MB391 Evaluation Platform for STi7710 */
#include "mb391.h"
#elif defined(mb394)  /* MB394 DTTi5516 Brick Board */
#include "mb394.h"
#elif defined(mb400)  /* MB400 Evaluation Platform for STi5105 */
#include "mb400.h"
#elif defined(mb385)  /* MB385-Champagne Evaluation Platform for STm5700 */
#include "mb385.h"
#elif defined(walkiry)/* Walkiry Evaluation Platform for STm5700 */
#include "walkiry.h"
#elif defined(mb411)  /* MB411 Evaluation Platform for STi7100/STx7109 */
#include "mb411.h"
#elif defined(mb426)  /* MB426-Traviata Evaluation Platform for STm8010 */
#include "mb426.h"
#elif defined(mb421)  /* MB421 Evaluation Platform for STm8010 */
#include "mb421.h"
#elif defined(mb395)  /* MB395 Evaluation Platform for STi5100 */
#include "mb395.h"
#elif defined(maly3s) /* MALY3S Reference Platform for STi5105 */
#include "maly3s.h"
#elif defined(mb428)  /* MB428 Evaluation Platform for STx5525 */
#include "mb428.h"
#elif defined(mb457)  /* MB457 Evaluation Platform for STx5188 */
#include "mb457.h"
#elif defined(mb436)  /* MB436 Evaluation Platform for STx5107 */
#include "mb436.h"
#elif defined(DTT5107) /* DTT5107 Evaluation Platform for STx5107 */
#include "DTT5107.h"
#elif defined(CAB5107) /* CAB5107 Evaluation Platform for STx5107 */
#include "CAB5107.h"
#elif defined(SAT5107) /* SAT5107 Evaluation Platform for STx5107 */
#include "SAT5107.h"
#elif defined(mb519)  /* MB519 Evaluation Platform for STx7200 */
#include "mb519.h"
#else
/*For GNBvd36334-->Request enhancement for user platform*/
/*If the user wants to use their own customised platform, they can add a file named "stdevice_user.h"
in the include directory. If the user specifies an unrecognised platform in DVD_PLATFORM &
does not put a stdevice_user.h file, then they will get a "#include file "stdevice_user.h" wouldn't open"
error during compilation*/
/*#include "stdevice_user.h" This file is to be added by the user for their customised platform*/
#include "cocoref_v2_7109.h"
#endif


/* Select target processor device.
 * At least one of these must be set at compile time.
 */

#if defined(ST_5510)      /* STi5510 device */
#include "sti5510.h"
#elif defined(ST_5512)    /* STi5512 device */
#include "sti5512.h"
#elif defined(ST_5508)    /* STi5508 device */
#include "sti5508.h"
#elif defined(ST_5518)    /* STi5518 device */
#include "sti5518.h"
#elif defined(ST_5580)    /* STi5580 device */
#include "sti5580.h"
#elif defined(ST_TP3)     /* ST20TP3 device */
#include "st20tp3.h"
#elif defined(ST_GX1)     /* ST40GX1 device */
#include "st40gx1.h"
#elif defined(ST_NGX1)    /* ST40NGX1 device */
#include "st40ngx1.h"
#elif defined(ST_STB1)    /* ST40STB1 device */
#include "st40stb1.h"
#elif defined(ST_7750)    /* ST407750 device */
#include "st407750.h"
#elif defined(ST_5514)    /* STi5514 device */
#include "sti5514.h"
#elif defined(ST_5516)    /* STi5516 device */
#include "sti5516.h"
#elif defined(ST_5517)    /* STi5517 device */
#include "sti5517.h"
#elif defined(ST_5528)    /* STi5528 device */
#include "sti5528.h"
#elif defined(ST_5100)    /* STi5100 device */
#include "sti5100.h"
#elif defined(ST_5101)    /* STi5101 device */
#include "sti5101.h"
#elif defined(ST_7710)    /* STi7710 device */
#include "sti7710.h"
#elif defined(ST_5105)    /* STi5105 device */
#include "sti5105.h"
#elif defined(ST_5700)    /* STm5700 device */
#include "stm5700.h"
#elif defined(ST_7100)    /* STi7100 device */
#include "sti7100.h"
#elif defined(ST_8010)    /* STm8010 device */
#include "stm8010.h"
#elif defined(ST_5301)    /* STi5301 device */
#include "sti5301.h"
#elif defined(ST_5525)    /* STx5525 device */
#include "sti5525.h"
#elif defined(ST_7109)    /* STx7109 device */
#include "sti7109.h"
#elif defined(ST_5188)    /* STx5188 device */
#include "sti5188.h"
#elif defined(ST_5107)    /* STx5107 device */
#include "sti5107.h"
#elif defined(ST_7200)    /* STx7200 device */
#include "sti7200.h"
#else
#error No target CPU selected by environment (variable DVD_FRONTEND)
#endif

/*
 * Select backend decoder device.
 * This is optional and may be omitted from the build.
 */
#if defined(ST_7015)      /* STi7015 device */
#include "sti7015.h"
#endif

#if defined(ST_7020)      /* STi7020 device */
#include "sti7020.h"
#endif

#if defined(ST_4629)      /* STi4629 device */
#include "sti4629.h"
#endif


/*
 * Software configuration
 */

/*#include "swconfig.h"*/

#endif /* __STDEVICE_H */

/* End of stdevice.h */
