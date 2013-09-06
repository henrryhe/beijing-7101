#ifndef _BR_FILE__H_
#define _BR_FILE__H_
#include "stdio.h"

typedef struct _linkedobj_struct
{
	unsigned long int linked_obj_id;
	unsigned char* linked_obj_addr;/*当前文件的首地址*/
	unsigned char* tmpbuf;        /*当前的文件指针buffer*/
	unsigned char obj_type;
	unsigned char cur_version;/*这个才是版本号*/
	unsigned char section;/*实际上是obj section number， 就是表示第几个SECTION*/
	unsigned char last_section;/*最后一个section number */
	unsigned long int CRC32;/*crc 校验*/
	unsigned long int obj_size; /*当前文件的大小*/
	unsigned char compressed_flag;
	unsigned long int compressed_size;
	int received;
	struct _linkedobj_struct * next;
}z_linkedobj_struct;

#if 0
#define graph_read fread
#define graph_open fopen
#define graph_seek fseek
#define graph_close fclose
#define graph_file	FILE
#define graph_setmode setmode
#define graph_fdopen	fdopen
#define graph_setvbuf	setvbuf
#define graph_bopen	open
#define graph_getc		getc
#define graph_bclose	close
#define graph_ungetc	ungetc
#else
#define graph_read chz_fread
#define graph_open chz_fopen
#define graph_seek chz_fseek
#define graph_close chz_fclose
#define graph_file	z_linkedobj_struct
#define graph_setmode chz_setmode
#define graph_fdopen	chz_fdopen
#define graph_setvbuf	chz_setvbuf
#define graph_bopen	chz_open
#define graph_getc		chz_getc
#define graph_bclose	chz_close
#define graph_ungetc	chz_ungetc


/*
从文件的当前位置开始中读取size*count个字节的数据
buf 存放读入数据的指针
size 每个数据单位的字节数
count 读入的数据单位个数
*/
int chz_fread(void *buf, int size, int count, graph_file *fp);

/*
打开一个文件
filename 文件名称
*/
graph_file *chz_fopen(char *filename, char *mode,int ri_len);

/*
设定文件操作指示器位置
p 文件指针
offset 相对于origin规定的偏移位置量
origin 指针移动的起始位置，可设置为以下三种情况：     
SEEK_SET 文件开始位置              
SEEK_CUR 文件当前位置               
SEEK_END 文件结束位置
*/
int chz_fseek(graph_file *fp, long offset, int origin);

int chz_fclose(graph_file *fp);
int chz_close(int handle);

/*重新设置一个已打开文件的操作模式*/
int chz_setmode(int handle, int amode);


/*得到一个与UNIX文件句柄handle等价的流式文件指针*/
graph_file *chz_fdopen(int handle, char *mode);


/*对指定文件fp指定操作缓冲区*/
int chz_setvbuf(graph_file *fp, char *buf, int type, size_t size);


/*以各种方式打开文件*/
int chz_open(const char *path, int mode,int ri_len);


/*从文件中读出一个字符*/
int chz_getc(graph_file *fp);

/*将字符c放到文件流fp的首部*/
int chz_ungetc(int c, graph_file *fp);
#endif
#endif


