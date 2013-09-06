#include "br_file.h"
#include "graphcom.h"
#if 1
graph_file   g_graphicFile;

/*
从文件的当前位置开始中读取size*count个字节的数据
buf 存放读入数据的指针
size 每个数据单位的字节数
count 读入的数据单位个数
*/
int chz_fread(void *buf, int size, int count, graph_file *fp)
{
 
	int buf_long = EOF;
	if(fp == NULL)
		return 0;
	buf_long = CH_MIN(size * count, (fp->obj_size  + fp->linked_obj_addr- fp->tmpbuf));
	if(buf_long > 0)
	{
		memcpy(buf, fp->tmpbuf, buf_long);
		fp->tmpbuf += buf_long;
	}
	return buf_long;
 
}

/*
打开一个文件
filename 文件名称
*/
graph_file *chz_fopen(char *filename, char *mode, int ri_Len)
{
#if 0
	return 
	 	(graph_file *)chz_find_obj_id(filename);
#else
   g_graphicFile.linked_obj_addr = g_graphicFile.tmpbuf=(U8*)filename;
if(ri_Len != 0)
   g_graphicFile.obj_size = ri_Len;
   return &g_graphicFile;
#endif
}

/*
设定文件操作指示器位置
p 文件指针
offset 相对于origin规定的偏移位置量
origin 指针移动的起始位置，可设置为以下三种情况：     
SEEK_SET 文件开始位置              
SEEK_CUR 文件当前位置               
SEEK_END 文件结束位置
*/
int chz_fseek(graph_file *fp, long offset, int origin)
{
 	int len;
	if(fp == NULL)
		return 0;
	switch(origin)
	{
		case SEEK_SET:
			fp->tmpbuf = fp->linked_obj_addr + offset;
			break;
		case SEEK_CUR:
			fp->tmpbuf = fp->tmpbuf + offset;
			break;
		case SEEK_END:
			fp->tmpbuf = fp->linked_obj_addr + fp->obj_size + offset;
			break;
		default:
			break;
			
	}
	return (fp->tmpbuf - fp->linked_obj_addr);
 }

int chz_fclose(graph_file *fp)
{
	if(fp == NULL)
		return -1;
	fp->tmpbuf = fp->linked_obj_addr;
	return 0;
}
int chz_close(int handle)
{
	return 
		chz_fclose((graph_file *)handle);
}

/*重新设置一个已打开文件的操作模式*/
int chz_setmode(int handle, int amode)
{
	return 0;
}


/*得到一个与UNIX文件句柄handle等价的流式文件指针*/
graph_file *chz_fdopen(int handle, char *mode)
{
	return (graph_file *)handle;
}


/*对指定文件fp指定操作缓冲区*/
int chz_setvbuf(graph_file *fp, char *buf, int type, size_t size)
{
	return 0;
}


/*以各种方式打开文件*/
int chz_open(const char *path,int mode,int ri_len)
{
	graph_file *fp;
	fp = chz_fopen((char*)path, NULL,ri_len);
	if(fp)
		return (int)fp;
	else
		return -1;
}


/*从文件中读出一个字符*/
int chz_getc(graph_file *fp)
{
	char c = EOF;
	if(fp && fp->tmpbuf < fp->linked_obj_addr + fp->obj_size)
	{
		c = *fp->tmpbuf;
		 fp->tmpbuf++;
	}
	 return (int)c;
}

/*将字符c放到文件流fp的首部*/
int chz_ungetc(int c, graph_file *fp)
{
	if(fp)
	{
		*fp->linked_obj_addr = (char)(c & 0xff);
		return c;
	}
	return EOF;
}
#else
int close(int handle)
{
return 0;
}
int setmode(int handle, int amode)
{
return 0;
}
#endif

