/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description :
 *
 *****************************************************************************/


#ifndef stpti4_ioctl_FOPS_H
#define stpti4_ioctl_FOPS_H


int     stpti4_ioctl_open   (struct inode*, struct file*);
int     stpti4_ioctl_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
int     stpti4_ioctl_mmap   (struct file *filp, struct vm_area_struct *vma);
int     stpti4_ioctl_release(struct inode*, struct file*);  /* close */


#endif
