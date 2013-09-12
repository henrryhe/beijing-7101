/*****************************************************************************
 *
 *  Module      : staudlx_core_fops.h
 *  Date        : 02-07-2005
 *  Author      : WALKERM
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005 
 *****************************************************************************/


#ifndef STAUDLX_CORE_FOPS_H
#define STAUDLX_CORE_FOPS_H


int     staudlx_core_open   (struct inode*, struct file*);
ssize_t staudlx_core_read   (struct file*,        char*, size_t, loff_t*);
ssize_t staudlx_core_write  (struct file*,  const char*, size_t, loff_t*);
int     staudlx_core_flush  (struct file*);
loff_t  staudlx_core_lseek  (struct file*,  loff_t, int);
int     staudlx_core_readdir(struct file*,  void*, filldir_t);
int     staudlx_core_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
unsigned int     staudlx_core_poll   (struct file*,  struct poll_table_struct*);
int     staudlx_core_mmap   (struct file*,  struct vm_area_struct*);
int     staudlx_core_fsync  (struct inode*, struct dentry*, int);
int     staudlx_core_fasync (int,           struct file*,   int);
ssize_t staudlx_core_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t staudlx_core_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     staudlx_core_release(struct inode*, struct file*);  /* close */


#endif
