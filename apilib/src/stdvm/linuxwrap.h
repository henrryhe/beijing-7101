/*****************************************************************************

File Name  : linuxwrap.h

Description: OS & File System Wrapper for STLinux

COPYRIGHT (C) 2006 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LINUXWRAP_H
#define __LINUXWRAP_H

/* Includes --------------------------------------------------------------- */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <dirent.h>

/* Exported Constants ----------------------------------------------------- */

#define OSPLUS_O_RDONLY     O_RDONLY
#define OSPLUS_O_WRONLY     O_WRONLY
#define OSPLUS_O_RDWR       O_RDWR
#define OSPLUS_O_ACCMODE    O_ACCMODE
#define OSPLUS_O_CREAT      O_CREAT

#define OSPLUS_SEEK_SET     SEEK_SET
#define OSPLUS_SEEK_CUR     SEEK_CUR
#define OSPLUS_SEEK_END     SEEK_END

#define OSPLUS_SUCCESS      (0)
#define OSPLUS_FAILURE      (-1)

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* OS */
#define task_wait(_taskp_,_count_,_timeout_)        /* Nothing to Do */
#define message_create_queue                        message_create_queue_timeout

/* file access functions */
#define vfs_close(_fd_)                             close((int)_fd_)
#define vfs_read(_fd_,_buf_,_count_)                read((int)_fd_,_buf_,_count_)
#define vfs_write(_fd_,_buf_,_count_)               write((int)_fd_,_buf_,_count_)
#define vfs_mkdir(_pathname_)                       mkdir(_pathname_,S_IRWXU | S_IRWXG | S_IRWXO)
#define vfs_rmdir(_pathname_)                       rmdir(_pathname_)
#define vfs_stat(_filename_,_status_)               stat(_filename_,_status_)
#define vfs_fseek(_fd_,_offset_,_whence_)           ((lseek((int)_fd_,_offset_,_whence_) >= 0) ? OSPLUS_SUCCESS : OSPLUS_FAILURE)
#define vfs_ftell(_fd_)                             lseek((int)_fd_,0,SEEK_CUR)
#define vfs_link(_oldpath_,_newpath_)               link(_oldpath_,_newpath_)
#define vfs_unlink(_pathname_)                      unlink(_pathname_)
#define vfs_resize(_fd_,_size_)                     ftruncate((int)_fd_,_size_)

/* partition access functions */
#define vfs_fscntl(_pathname_,_fscntl_,_info_)      statfs(_pathname_,_info_)

#define FS_PARTITION_FREE_SIZE(_info_)              ((_info_).f_blocks * ((_info_).f_bsize/HDD_SECTOR_SIZE))

/* directory related functions */
#define vfs_opendir(_path_)                         opendir(_path_)
#define vfs_readdir(__dir__,__dirent__)             ((((__dirent__) = readdir(__dir__)) != NULL) ? 0 : 1)
#define vfs_closedir(__dir__)                       closedir(__dir__)

#define DIRENT_IS_FILE(__dirent__)                  ((__dirent__)->d_type == DT_REG)

/* cache releated functions not need for STLinux */
#define osplus_uncached_register(address,size)      /* not needed for STLinux */
#define osplus_uncached_unregister(address,size)    /* not needed for STLinux */

/* Exported Types --------------------------------------------------------- */
typedef void            vfs_file_t;
typedef struct stat     vfs_stat_t;
typedef struct statfs   vfs_info_t;

typedef DIR             vfs_dir_t;
typedef struct dirent   vfs_dirent_t;

/* Exported Functions ----------------------------------------------------- */

/* OS */
task_t *task_create(void (*funcp)(void *param), void *param, size_t stack_size, int priority, const char *name, task_flags_t flags);
int     task_delete(task_t *taskp);

/* FS */
vfs_file_t  *vfs_open(const char *pathname, int flags);

#endif /*__LINUXWRAP_H*/

/* EOF --------------------------------------------------------------------- */

