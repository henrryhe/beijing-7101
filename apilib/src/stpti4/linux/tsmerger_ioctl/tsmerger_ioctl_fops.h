/*****************************************************************************
 *
 *  Module      : tsmerger_ioctl
 *  Date        : 24-07-2005
 *  Author      : STIEGLITZP
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005 
 *****************************************************************************/


#ifndef tsmerger_ioctl_FOPS_H
#define tsmerger_ioctl_FOPS_H


int     tsmerger_ioctl_open   (struct inode*, struct file*);
ssize_t tsmerger_ioctl_read   (struct file*,        char*, size_t, loff_t*);
ssize_t tsmerger_ioctl_write  (struct file*,  const char*, size_t, loff_t*);
int     tsmerger_ioctl_flush  (struct file*);
loff_t  tsmerger_ioctl_lseek  (struct file*,  loff_t, int);
int     tsmerger_ioctl_readdir(struct file*,  void*, filldir_t);
int     tsmerger_ioctl_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
unsigned int     tsmerger_ioctl_poll   (struct file*,  struct poll_table_struct*);
int     tsmerger_ioctl_mmap   (struct file*,  struct vm_area_struct*);
int     tsmerger_ioctl_fsync  (struct inode*, struct dentry*, int);
int     tsmerger_ioctl_fasync (int,           struct file*,   int);
ssize_t tsmerger_ioctl_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t tsmerger_ioctl_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     tsmerger_ioctl_release(struct inode*, struct file*);  /* close */


#endif
