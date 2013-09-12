/*****************************************************************************
 *
 *  Module      : stvmix_ioctl
 *  Date        : 30-10-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef stvmix_ioctl_FOPS_H
#define stvmix_ioctl_FOPS_H


int     stvmix_ioctl_open   (struct inode*, struct file*);
ssize_t stvmix_ioctl_read   (struct file*,        char*, size_t, loff_t*);
ssize_t stvmix_ioctl_write  (struct file*,  const char*, size_t, loff_t*);
int     stvmix_ioctl_flush  (struct file*);
loff_t  stvmix_ioctl_lseek  (struct file*,  loff_t, int);
int     stvmix_ioctl_readdir(struct file*,  void*, filldir_t);
int     stvmix_ioctl_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
int     stvmix_ioctl_mmap   (struct file*,  struct vm_area_struct*);
int     stvmix_ioctl_fsync  (struct inode*, struct dentry*, int);
int     stvmix_ioctl_fasync (int,           struct file*,   int);
ssize_t stvmix_ioctl_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t stvmix_ioctl_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     stvmix_ioctl_release(struct inode*, struct file*);  /* close */


#endif
