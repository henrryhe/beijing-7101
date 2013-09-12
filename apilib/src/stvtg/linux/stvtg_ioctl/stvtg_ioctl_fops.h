/*****************************************************************************
 *
 *  Module      : stvtg_ioctl
 *  Date        : 15-10-2005
 *  Author      : ATROUSWA
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef stvtg_ioctl_FOPS_H
#define stvtg_ioctl_FOPS_H


int     stvtg_ioctl_open   (struct inode*, struct file*);
ssize_t stvtg_ioctl_read   (struct file*,        char*, size_t, loff_t*);
ssize_t stvtg_ioctl_write  (struct file*,  const char*, size_t, loff_t*);
int     stvtg_ioctl_flush  (struct file*);
loff_t  stvtg_ioctl_lseek  (struct file*,  loff_t, int);
int     stvtg_ioctl_readdir(struct file*,  void*, filldir_t);
int     stvtg_ioctl_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
int     stvtg_ioctl_fsync  (struct inode*, struct dentry*, int);
int     stvtg_ioctl_fasync (int,           struct file*,   int);
ssize_t stvtg_ioctl_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t stvtg_ioctl_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     stvtg_ioctl_release(struct inode*, struct file*);  /* close */


#endif
