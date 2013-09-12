/*****************************************************************************
 *
 *  Module      : stvidtest_ioctl_fops.h
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description :
 *  Copyright   : STMicroelectronics (c)2006 
 *****************************************************************************/


#ifndef STVIDTEST_IOCTL_FOPS_H
#define STVIDTEST_IOCTL_FOPS_H

int     stvidtest_ioctl_open   (struct inode*, struct file*);
ssize_t stvidtest_ioctl_read   (struct file*,        char*, size_t, loff_t*);
ssize_t stvidtest_ioctl_write  (struct file*,  const char*, size_t, loff_t*);
int     stvidtest_ioctl_flush  (struct file*);
loff_t  stvidtest_ioctl_lseek  (struct file*,  loff_t, int);
int     stvidtest_ioctl_readdir(struct file*,  void*, filldir_t);
int     stvidtest_ioctl_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
unsigned int     stvidtest_ioctl_poll   (struct file*,  struct poll_table_struct*);
int     stvidtest_ioctl_fsync  (struct inode*, struct dentry*, int);
int     stvidtest_ioctl_fasync (int,           struct file*,   int);
ssize_t stvidtest_ioctl_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t stvidtest_ioctl_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     stvidtest_ioctl_release(struct inode*, struct file*);  /* close */

#endif  /* !STVIDTEST_IOCTL_FOPS_H */
