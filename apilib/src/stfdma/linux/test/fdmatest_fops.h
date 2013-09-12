/*****************************************************************************
 *
 *  Module      : fdmatest
 *  Date        : 05-04-2005
 *  Author      : WALKERM
 *  Description :
 *
 *****************************************************************************/


#ifndef fdmatest_FOPS_H
#define fdmatest_FOPS_H


int     fdmatest_open   (struct inode*, struct file*);
ssize_t fdmatest_read   (struct file*,        char*, size_t, loff_t*);
ssize_t fdmatest_write  (struct file*,  const char*, size_t, loff_t*);
int     fdmatest_flush  (struct file*);
loff_t  fdmatest_lseek  (struct file*,  loff_t, int);
int     fdmatest_readdir(struct file*,  void*, filldir_t);
int     fdmatest_ioctl  (struct inode*, struct file*, unsigned int, unsigned long);
unsigned int     fdmatest_poll   (struct file*,  struct poll_table_struct*);
int     fdmatest_mmap   (struct file*,  struct vm_area_struct*);
int     fdmatest_fsync  (struct inode*, struct dentry*, int);
int     fdmatest_fasync (int,           struct file*,   int);
ssize_t fdmatest_readv  (struct file*,  const struct iovec*, unsigned long, loff_t*);
ssize_t fdmatest_writev (struct file*,  const struct iovec*, unsigned long, loff_t*);
int     fdmatest_release(struct inode*, struct file*);  /* close */


#endif
