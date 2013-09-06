/******************************************************************************\
 *
 * File Name   : subtask.h
 *
 * Description : functions to allocate and create tasks for stsubt decoder
 *
 * Copyright 1998, 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Alessandro Innocenti - October 1999
 *
\******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SUBTTASK_H
#define __SUBTTASK_H

#include <stddefs.h>
#include <stlite.h>

/* structure to collect information regarding a task */

typedef struct {
  task_t   task;
  tdesc_t  tdesc;
  void	  *stack;
  ST_Partition_t *Partition;
} STSUBT_Task_t;

STSUBT_Task_t * SubtTaskCreate (
                        void (*Function)(void* Param), 
                        void* Param,
                        size_t StackSize, 
                        int Priority,
                        const char* Name, 
                        task_flags_t flags,
                        ST_Partition_t *TaskPartition);

ST_ErrorCode_t SubtTaskDelete (STSUBT_Task_t *SubtTask);

#endif  /* #ifndef __SUBTTASK_H */

