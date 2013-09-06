/******************************************************************************\
 *
 * File Name   : subtconf.h
 *  
 * Description : Software configuration header for STSUBT
 *  
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Sept 1999
 *  
\******************************************************************************/

#ifndef __SUBTCONF_H
#define __SUBTCONF_H

#include <stlite.h>

/* ----------------------------------------- */
/* --- Default defines for decoder tasks --- */
/* ----------------------------------------- */

/* Stack size and task priority can be externally set. */

#ifndef STSUBT_TASK_PRIORITY
/* #define STSUBT_TASK_PRIORITY         (MAX_USER_PRIORITY)  */
#define STSUBT_TASK_PRIORITY            (10) /*zxg change12*/
#endif

/* In case report messaging is enabled then an extra stack size 
 * is required (toolbox output string).
 */

#ifndef STSUBT_STACK_SIZE
#define STSUBT_STACK_SIZE               (4 * 1024)
#endif

/* --- Defines for Filter task --- */

#define FILTER_STACK_SIZE               (STSUBT_STACK_SIZE)
#define FILTER_TASK_PRIORITY            (STSUBT_TASK_PRIORITY)


/* --- Defines for Processor task --- */

#define PROCESSOR_STACK_SIZE     	(STSUBT_STACK_SIZE)
#define PROCESSOR_TASK_PRIORITY  	(STSUBT_TASK_PRIORITY)


/* --- Defines for Display Engine task --- */

#define ENGINE_STACK_SIZE        	(STSUBT_STACK_SIZE)
#define ENGINE_TASK_PRIORITY     	(STSUBT_TASK_PRIORITY)

/* --- Defines for Display Timer task --- */

#define TIMER_STACK_SIZE         	(STSUBT_STACK_SIZE)
#define TIMER_TASK_PRIORITY      	(STSUBT_TASK_PRIORITY)

/* -- Defines for pes extraction task (link only)--- */

#define EXTRACTPES_STACK_SIZE               (STSUBT_STACK_SIZE)
#define EXTRACTPES_TASK_PRIORITY            (STSUBT_TASK_PRIORITY+1)

/* ------------------------------------------ */
/* --- Default sizes of decoder resources --- */
/* ------------------------------------------ */
 
/* --- Define size of Coded Data Buffer --- */
 
#define CODED_DATA_BUFFER_SIZE          (24 * 1024)
 
/* --- Define size of Composition Buffer --- */
 
#define COMPOSITION_BUFFER_SIZE         (20 * 1024)
#define COMPOSITION_BUFFER_SIZE_FS      (20 * 1024)
 
/* --- Define max visible regions allocable into Comp. Buffer --- */
 
#define COMPOSITION_BUFFER_MAX_VIS_REGIONS  (50)
 
/* --- Define size of Pixel Buffer --- */
 
#define PIXEL_BUFFER_SIZE              	(80 * 1024)  
#define PIXEL_BUFFER_SIZE_FS           	(210 * 1024) 
 
/* --- Define number of PCS that can be stored --- */
/* --- in PCS Buffer and size of its items     --- */
 
#define PCS_BUFFER_NUM_ITEMS            10 
#define PCS_BUFFER_ITEM_SIZE    	sizeof(PCS_t)

/* --- Define size of Filter Buffer --- */

#define FILTER_BUFFER_SIZE      	((64 * 1024) + 4)

/* -- Define size of pes extraction task (link only) --- */
#define EXTRACT_PES_TEMP_BUFFER_SIZE (8*1024)  /* should match the size of pti_buffer */
 
/* --- Define size of Segment Buffer --- */

#define CI_WORKING_BUFFER_SIZE      	(CODED_DATA_BUFFER_SIZE)

/* --- Define number of display items that can be  --- */
/* --- stored in the Display Buffer and their size --- */
 
#define DISPLAY_BUFFER_NUM_ITEMS        10 
#define DISPLAY_ITEM_SIZE       	sizeof(STSUBT_DisplayItem_t)

/* -------------------------------------------------------- */
/* --- Default sizes of decoder resources for CI support--- */
/* -------------------------------------------------------- */

/* --- Define size of CI Reply Buffer it can be externally set.--- */

#ifndef CI_REPLY_BUFFER_SIZE
#define CI_REPLY_BUFFER_SIZE        (1 * 1024)
#endif

/* --- Define size of CI Message Buffer it can be externally set.--- */

#ifndef CI_MESSAGE_BUFFER_SIZE
#define CI_MESSAGE_BUFFER_SIZE      (1 * 1024)
#endif


#endif  /* #ifndef __SUBTCONF_H */

