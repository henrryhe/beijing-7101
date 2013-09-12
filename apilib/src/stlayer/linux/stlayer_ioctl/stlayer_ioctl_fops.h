/*****************************************************************************
 *
 *  Module      : stlayer_ioctl
 *  Date        : 13-11-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef stlayer_ioctl_FOPS_H
#define stlayer_ioctl_FOPS_H

int     stlayer_ioctl_open   (struct inode*, struct file*);
ssize_t stlayer_ioctl_read   (struct file*,        char*, size_t, loff_t*);
ssize_t stlayer_ioctl_write  (struct file*,  const char*, size_t, loff_t*);
int     stlayer_ioctl_flush  (struct file*);
loff_t  stlayer_ioctl_lseek  (struct file*,  loff_t, int);
int     stlayer_ioctl_readdir(struct file*,  void*, filldir_t);
int     stlayer_ioctl_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
unsigned int     stlayer_ioctl_poll   (struct file*,  struct poll_table_struct*);
int     stlayer_ioctl_fsync  (struct inode*, struct dentry*, int);
int     stlayer_ioctl_fasync (int,           struct file*,   int);
ssize_t stlayer_ioctl_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t stlayer_ioctl_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     stlayer_ioctl_release(struct inode*, struct file*);  /* close */


#endif
