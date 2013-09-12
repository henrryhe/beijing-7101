/*****************************************************************************
 *
 *  Module      : fdma
 *  Date        : 29-02-2005
 *  Author      : WALKERM
 *  Description :
 *
 *****************************************************************************/


#ifndef fdma_FOPS_H
#define fdma_FOPS_H


int     fdma_open   (struct inode*, struct file*);
ssize_t fdma_read   (struct file*,        char*, size_t, loff_t*);
ssize_t fdma_write  (struct file*,  const char*, size_t, loff_t*);
int     fdma_flush  (struct file*);
loff_t  fdma_lseek  (struct file*,  loff_t, int);
int     fdma_readdir(struct file*,  void*, filldir_t);
int     fdma_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
unsigned int     fdma_poll   (struct file*,  struct poll_table_struct*);
int     fdma_mmap   (struct file*,  struct vm_area_struct*);
int     fdma_fsync  (struct inode*, struct dentry*, int);
int     fdma_fasync (int,           struct file*,   int);
ssize_t fdma_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t fdma_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     fdma_release(struct inode*, struct file*);  /* close */


#endif
