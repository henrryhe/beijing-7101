/*****************************************************************************

File Name  : linuxwrap.c

Description: OS & File System Wrapper for STLinux

COPYRIGHT (C) 2006 STMicroelectronics

*****************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include <linuxwrapper.h>
#include "linuxwrap.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private functions prototypes --------------------------------------------- */

/* Global Variables --------------------------------------------------------- */

/* Public functions --------------------------------------------------------- */


/*******************************************************************************
Name         : task_create()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
task_t *task_create(void (*funcp)(void *param), void *param, size_t stack_size, int priority, const char *name, task_flags_t flags)
{
    task_t          task, *task_p;
    pthread_attr_t  attr;

    pthread_attr_init(&attr);
    if (priority >= 0)
    {
        struct sched_param sparam;

        if (priority < sched_get_priority_min(SCHED_RR))
            priority = sched_get_priority_min(SCHED_RR);

        if (priority > sched_get_priority_max(SCHED_RR))
            priority = sched_get_priority_max(SCHED_RR);

        sparam.sched_priority = priority;
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &sparam);
    }

    if (pthread_create(&task, &attr, (void*)funcp, param) != 0)
    {
        return NULL;
    }

    task_p = malloc(sizeof(task_t));
    if (task_p)
    {
        *task_p = task;
    }

    return task_p;
}

/*******************************************************************************
Name         : task_delete()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
int task_delete(task_t *taskp)
{
    pthread_join(*taskp, NULL);
    free(taskp);
    return 0;
}


/******************************************************************************
Name		: vfs_open

Description	: Warpper for porting from OSPLUS to LINUX

Parameters	: char* pathname	Path of the file
		      int   flags	    one of O_RDONLY, O_WRONLY, O_RDWR

******************************************************************************/
vfs_file_t  *vfs_open(const char *pathname, int flags)
{
	int fd;

	fd = open(pathname,flags);

	return (fd < 0) ? NULL : (vfs_file_t *)fd;
}

/* EOF ---------------------------------------------------------------------- */

