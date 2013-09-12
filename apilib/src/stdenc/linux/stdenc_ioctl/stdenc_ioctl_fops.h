/*****************************************************************************
 *
 *  Module      : stdenc_ioctl
 *  Date        : 21-10-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef stdenc_ioctl_FOPS_H
#define stdenc_ioctl_FOPS_H


int     stdenc_ioctl_open   (struct inode*, struct file*);
ssize_t stdenc_ioctl_read   (struct file*,        char*, size_t, loff_t*);
ssize_t stdenc_ioctl_write  (struct file*,  const char*, size_t, loff_t*);
int     stdenc_ioctl_flush  (struct file*);
loff_t  stdenc_ioctl_lseek  (struct file*,  loff_t, int);
int     stdenc_ioctl_readdir(struct file*,  void*, filldir_t);
int     stdenc_ioctl_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
int     stdenc_ioctl_fsync  (struct inode*, struct dentry*, int);
int     stdenc_ioctl_fasync (int,           struct file*,   int);
ssize_t stdenc_ioctl_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t stdenc_ioctl_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     stdenc_ioctl_release(struct inode*, struct file*);  /* close */


#endif
