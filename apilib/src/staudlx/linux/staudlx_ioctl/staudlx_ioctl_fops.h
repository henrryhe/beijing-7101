/*****************************************************************************
 *
 *  Module      : staudlx_ioctl
 *  Date        : 11-07-2005
 *  Author      : TAYLORD
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005 
 *****************************************************************************/


#ifndef staudlx_ioctl_FOPS_H
#define staudlx_ioctl_FOPS_H


int     staudlx_ioctl_open   (struct inode*, struct file*);
ssize_t staudlx_ioctl_read   (struct file*,        char*, size_t, loff_t*);
ssize_t staudlx_ioctl_write  (struct file*,  const char*, size_t, loff_t*);
int     staudlx_ioctl_flush  (struct file*);
loff_t  staudlx_ioctl_lseek  (struct file*,  loff_t, int);
int     staudlx_ioctl_readdir(struct file*,  void*, filldir_t);
int     staudlx_ioctl_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
unsigned int     staudlx_ioctl_poll   (struct file*,  struct poll_table_struct*);
int     staudlx_ioctl_mmap   (struct file*,  struct vm_area_struct*);
int     staudlx_ioctl_fsync  (struct inode*, struct dentry*, int);
int     staudlx_ioctl_fasync (int,           struct file*,   int);
ssize_t staudlx_ioctl_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t staudlx_ioctl_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     staudlx_ioctl_release(struct inode*, struct file*);  /* close */


#endif
