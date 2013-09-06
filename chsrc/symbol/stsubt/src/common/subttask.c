/******************************************************************************\
 *
 * File Name   : subtask.c
 *
 * Description : functions to create/delete tasks for stsubt decoder
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Alessandro Innocenti - October 1999
 *
\******************************************************************************/

#include <stdlib.h> /* For NULL */

#include <stddefs.h>
#include "subttask.h"

STSUBT_Task_t *SubtTaskCreate (
                       void (*Function)(void* Param), 
                       void* Param,
                       size_t StackSize, 
                       int Priority,
                       const char* Name, 
                       task_flags_t flags,
                       ST_Partition_t *TaskPartition)
{
    STSUBT_Task_t *SubtTask;
    int            result;

    SubtTask = memory_allocate (TaskPartition, sizeof(STSUBT_Task_t));
    if (SubtTask == NULL) 
    {
        return (NULL);
    }
    SubtTask->Partition = TaskPartition;

    SubtTask->stack = memory_allocate (TaskPartition, StackSize);
    if (SubtTask->stack == NULL) 
    {
        memory_deallocate(TaskPartition, SubtTask);
        return (NULL);
    }
    
    result = task_init(Function, Param, SubtTask->stack, StackSize,
                      &SubtTask->task, &SubtTask->tdesc, Priority, Name, flags);
    if (result != 0) 
    {
        memory_deallocate(TaskPartition, SubtTask->stack);
        memory_deallocate(TaskPartition, SubtTask);
        return (NULL);
    }

    return (SubtTask);
}

ST_ErrorCode_t SubtTaskDelete (STSUBT_Task_t *SubtTask)
{
    ST_Partition_t *Partition = SubtTask->Partition;

    if (task_delete(&SubtTask->task) != 0)
    {
        return (!ST_NO_ERROR);
    }

    memory_deallocate (Partition, SubtTask->stack);
    memory_deallocate (Partition, SubtTask);

    return 0;
}

