/*----------------------------------------------------------
IP loader升级监控
-----------------------------------------------------------*/
#include <stdio.h>
#include "sockets.h"
#include "report.h"
#include "stlite.h"
#include "string.h"
#include "stddefs.h"
#include "appltype.h"
#include "symbol.h"
#include "..\dbase\vdbase.h"
#include "../usif/applIF.h"
#include "ste2p.h"


#define TFTP_RRQ 1   /*Read request (RRQ)*/
#define TFTP_WRQ 2   /*Write request (WRQ) */
#define TFTP_DATA 3  /*Data (DATA)*/
#define TFTP_ACK 4   /*Acknowledgment (ACK)*/
#define TFTP_ERROR 5 /*Error (ERROR)*/

#define TFTP_NETASCII 0
#define TFTP_OCTET 1
#define TFTP_WSTAT_FIRSTACK 0
#define TFTP_WSTAT_NEXTACK 1
#define TFTP_WSTAT_LASTACK 2
#define MAX_RETRY 20
#define TFTP_NOTEND_DATALEN 512+2+2

typedef int SOCKET;

char desthost[256] = "10.27.118.252";/*JS Service Addr*/ /*Local addr: 127.0.0.1*/
int filemode = TFTP_OCTET;
SOCKET sock = INVALID_SOCKET;

U32 TFTP_SDL_LEN = 0;/**/
U32 TFTP_TOTAL_LEN = 0;/*数据总长度*/

boolean tftp_cfg_serial_in_range = false;/*序列号是否满足条件标志*/

typedef struct 
{
	U8 manufacture_code;
	U8 stb_type;
	U8 hardware_version;
	U8 version_update_mode;
	U32 serial_range;
	U8 app_update_mode;
	U32 program_version;
	char program_filename[256];
	U32 program_file_crc32;
}tftp_config_file_struct;
tftp_config_file_struct tftp_config_file;

extern void do_report( report_severity_t   report_severity, char *format, ... );
extern STE2P_Handle_t 		hE2PDevice;		


/***************************************************************************
TFTP升级入口
前提条件:
	协议栈初始化
	本机IP地址成功获取
	本机DNS成功获取

返回值:
	-1: 无升级
	0: 如果有升级信息，则提示用户，由用户选择是否升级；
	1: 如果有升级信息不提示用户，立即进行升级；
	其他值: 非法,无升级;
***************************************************************************/
int ch_ldr_tftp_checkdld(void)
{
	U32 dns_ip = 0;
	int iloop = 0;
	int cfg_result = -1;

	/*将协议栈的初始化移到TFTP升级触发之后*/
	/*获取本机IP DNS地址*/

	/***************获取升级服务器的DNS IP地址********************/
	iloop = 0;
	while(iloop < 5)
	{
		dns_ip = ch_tftp_getdnsip();
		
		if(dns_ip < 100)
		{/*获取服务器IP失败,尝试3次*/
			do_report(0,"get dns ip address error[%d]!\r\n",iloop);
			task_delay(ST_GetClocksPerSecondLow()*2);
			iloop++;
			continue;
		} 
		else
		{/*获取DNS成功,显示并退出*/
			do_report(0,"loader.njcatv.net：[%d.%d.%d.%d]",
							dns_ip&0xFF,
							(dns_ip>>8)&0xFF,
							(dns_ip>>16)&0xFF,
							(dns_ip>>24)&0xFF);
			break;
		}
		
	}
	if(iloop >= 5)
	{/**获取服务器DNS超时,退出*/
		do_report(0,"Get service DNS IP address timeout\r\n");
		return -1;
	}

	/************************获取升级文件的配置信息**************************/
	iloop = 0;
	while(iloop < 3)
	{/**/
		if(-1 == ch_tftp_getconfig(dns_ip))
		{/*获取配置文件出错*/
			do_report(0,"get tftp config file error = [%d]\r\n", iloop);
			iloop++;
			continue;
		}
		else
		{/*获取配置文件成功，判断是否需要升级*/
			cfg_result = ch_tftp_CheckDldInfo();
			if(-1 == cfg_result)
			{/*配置文件中没有升级信息*/
				do_report(0,"No download information!\r\n");
				return -1;
			}
			else
			{
				break;
			}
		}
	}
	if(iloop >= 3)
	{/*获取文件超时，退出*/
		do_report(0,"get tftp config file timeout!\r\n");
		return -1;
	}
	
	return cfg_result;
	
}

/*生成ACK数据报*/
int makeack(unsigned short num,char *buffer,int size )
{
  int pos = 0;
  buffer[pos] = 0;
  pos++;
  buffer[pos] = TFTP_ACK;
  pos++;
  buffer[pos] = (char)(num>>8);
  pos++;
  buffer[pos] = (char)num;
  pos++;
  return pos;
}

/*生成REQ数据*/
int makereq(char type,int mode,char *filename,char *buffer,int size)
{
  int pos = 0;
  unsigned int i = 0;
  char s[32] = "";
  if(mode==TFTP_NETASCII)
    strcpy(s,"netascii");
  else
	strcpy(s,"octet");
  buffer[pos] = 0;
  pos++;
  buffer[pos] = type;
  pos++;
  for(i=0;i<strlen(filename);i++)
  {
    buffer[pos] = filename[i];
    pos++;
  }
  buffer[pos] = 0;
  pos++;
  for(i=0;i<strlen(s);i++)
  {
    buffer[pos] = s[i];
    pos++;
  }
  buffer[pos] = 0;
  pos++;
  return pos;
}

/*生成DATA数据*/
int makedata(int num,char *data,int datasize,char *buffer,int bufsize)
{
  int pos = 0;
  buffer[pos] = 0;
  pos++;
  buffer[pos] = TFTP_DATA;
  pos++;
  buffer[pos] = (char)(num>>8);
  pos++;
  buffer[pos] = (char)num;
  pos++;
  memcpy(&buffer[pos],data,datasize);
  pos = pos + datasize;
  return pos;

}


unsigned int ch_tftp_getdnsip( void )
{
	unsigned int DNS;
	SOCKET socket;
	struct sockaddr_in serv_addr;
	char recv_buf[1024];
	int length = 0,fromlen;
	int argp=1;
	int ip=0;
	unsigned int *temp;
	U8	dns_buf[]={0x47,0x23,0x01,0x00,0x00,0x01,0x00,0x00,
					0x00,0x00,0x00,0x00,0x06,0x6c,0x6f,0x61,
					0x64,0x65,0x72,0x06,0x6e,0x6a,0x63,0x61,
					0x74,0x76,0x03,0x6e,0x65,0x74,0x00,0x00,
					0x01,0x00,0x01};
	
	U8 que_num=0,ans_num=0;
	int i=0;
	
	DNS = CH_GetDNSByNum(0);
	if(DNS == 0)
	{
		do_report(0,"get local DNS error, Local DNS = 0\r\n");
		return 20;
	}
	socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket<0) 
	{
		do_report(0,"socket error!\r\n");
		return ip;
	}
	
	/*bzero(&serv_addr,sizeof(serv_addr));*/
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_len = sizeof(struct sockaddr);
	serv_addr.sin_addr.s_addr = DNS;
	serv_addr.sin_port = htons(53);

	ioctlsocket(socket,O_NONBLOCK,(void *)&argp);
	length=sendto(socket,(void *)dns_buf,35,0,((struct sockaddr*)&(serv_addr)),sizeof(struct sockaddr));
	if(length<=0)
	{
		do_report(0,"sendto error!\r\n");
		goto dns_exit;
	}
	
	memset(recv_buf,0,1024);
	
	while(1)
	{
		length = recvfrom(socket,recv_buf,1024,0,((struct sockaddr*) &(serv_addr)),&fromlen);
		if(length>50)
		{
			if(recv_buf[0]!=0x47 || recv_buf[1]!=0x23)
			{
				goto dns_exit;
			}
			if(recv_buf[2]!=0x85 && recv_buf[2]!=0x84)
			{
				goto dns_exit;
			}
			if(recv_buf[3]!=0x80 && recv_buf[3]!=0)
			{
				goto dns_exit;
			}
			
			que_num=recv_buf[5];
			ans_num=recv_buf[7];
			
			if( que_num==0 || ans_num==0 )
			{
				goto dns_exit;
			}
#if 0
			ip=(unsigned int )(recv_buf[47]<<24)|(recv_buf[48]<<16)|(recv_buf[49]<<8)|(recv_buf[50]);
#else
			ip=(unsigned int )(recv_buf[47])|(recv_buf[48]<<8)|(recv_buf[49]<<16)|(recv_buf[50]<<24);
#endif			
			do_report(0,"ip=0x%x \r\n",ip);
			goto dns_exit;
		}
		task_delay ( ST_GetClocksPerSecondLow() /3 );
		i++;
		if(i>3)
		{
			break;
		}
	}

dns_exit:
	 close(socket);	
	 return ip;
}



/*返回值: 读取文件的行数*/
int trip_string(char* config_buf)
{
	char config[50][100];
	char serial_range[50];
	int iloop=0,token_loop=0,jloop=0;
	char *token=NULL;
	char seps[] = "\r\n";
	char temp_cfg_buf[2048];
	char *test_cfg = NULL;
	char config_head[8][30] = {
		"manufacture_code:",
		"stb_type:",
		"hardware_version:",
		"version_update_mode:",
		"serial_range:",
		"app_update_mode:",
		"program_version:",
		"program_filename:"
	};
	/*do_report(0,"IP LOADER:config file content\n[\n%s\n]\n",config_buf);*/
	memcpy(temp_cfg_buf,config_buf,2048);
	memset(config,0,sizeof(config));
	token = strtok( temp_cfg_buf, seps );
	while(token!=NULL)
	{
		strncpy(config[token_loop],token,100);
		token = strtok( NULL, seps );
		token_loop++;
		if(token_loop>50)
		{
			break;
		}
	}
	token_loop--;

	/*get manufacture code*/
	for(iloop=0; iloop<token_loop; iloop++)
	{
		if(strncmp(config_head[0],(char*)config[iloop],17) == 0)
		{
			tftp_config_file.manufacture_code = atoi(&config[iloop][17]);	
			break;
		}
	}

	/*get stb_type*/
	for(iloop=0; iloop<token_loop; iloop++)
	{
		if(strncmp(config_head[1],(char*)config[iloop],9) == 0)
		{
			tftp_config_file.stb_type= atoi(&config[iloop][9]);
			break;
		}
	}

	/*get hardware_version*/
	for(iloop=0; iloop<token_loop; iloop++)
	{
		if(strncmp(config_head[2],(char*)config[iloop],17) == 0)
		{
			tftp_config_file.hardware_version= atoi(&config[iloop][17]);
			break;
		}
	}	

	/*get version_update_mode:*/
	for(iloop=0; iloop<token_loop; iloop++)
	{
		if(strncmp(config_head[3],(char*)config[iloop],20) == 0)
		{
			tftp_config_file.version_update_mode= atoi(&config[iloop][20]);
			break;
		}
	}	

	/*get serial_range:*/
	tftp_config_file.serial_range = 0;
	for(iloop=0; iloop<token_loop; iloop++)
	{
		if(strncmp(config_head[4],(char*)config[iloop],13) == 0)
		{
			tftp_config_file.serial_range= atoi(&config[iloop][13]);
			if(tftp_config_file.serial_range > 46)
			{/*缓存区最大能存46行的序列号范围值*/
				do_report(0,"TFTP:Serial Range Error[%d>46]!\r\n",tftp_config_file.serial_range);
				return -1;
			}
			do_report(0,"TFTP Serial_range Count=[%d]\r\n",tftp_config_file.serial_range);
			break;
		}
	}	
	/*************************************************/
	/******需要在此添加对序列号范围的判断*************/	
	jloop = iloop + 1;
	for(iloop=0; iloop<tftp_config_file.serial_range;iloop++)
	{
		strncpy(serial_range,(char*)config[jloop + iloop],36);
		if(0 == ch_tftp_CheckSerialNum(serial_range))
		{/*序列号在范围内*/
			tftp_cfg_serial_in_range = true;
			do_report(0,"current serial id is in range!\r\n");
			break;
		}
		else
		{/*序列号不在范围内*/
			do_report(0,"current serial id not in range!\r\n");
			continue;
		}
	}

	/*************************************************/

	/*get app_update_mode:*/
	for(iloop=0; iloop<token_loop; iloop++)
	{
		if(strncmp(config_head[5],(char*)config[iloop],16) == 0)
		{
			tftp_config_file.app_update_mode= atoi(&config[iloop][16]);
			break;
		}
	}	
	
	/*get program_version:*/
	for(iloop=0; iloop<token_loop; iloop++)
	{
		if(strncmp(config_head[6],(char*)config[iloop],16) == 0)
		{
			tftp_config_file.program_version = atoi(&config[iloop][16]);
			break;
		}
	}

	/*get program_filename:*/
	for(iloop=0; iloop<token_loop; iloop++)
	{
		if(strncmp(config_head[7],(char*)config[iloop],17) == 0)
		{
			strcpy(tftp_config_file.program_filename,&config[iloop][17]);
			break;
		}
	}
	
	return iloop;
}

/*检查升级配置信息*/
/*需要修改，仔细测试*/
int ch_tftp_getconfig(U32 dns_ip)
{
  char sendbuf[1024] = {0};
  char recvbuf[1024] = {0};
  char config_buffer[2048];
  struct sockaddr_in addr;
  struct sockaddr_in from;
  int fromlen = 0;
  int ret = 0, test_ret=0;
  int len = 0 ;
  fd_set  fdr;
  int retry = 0;
  struct  timeval timeout = {1,0};
  int stat = 0;
  int lastdata = 0;
  int flen = 0;

#ifdef JIANGSHU_NOCM 	
  char* cfg_filename= "changhong_dvbc6600bis_05.txt";/*设置配置文件名*//*smsc stack*/	
#else
  char* cfg_filename= "changhong_dvbc6600bic_03.txt";/*设置配置文件名*/
#endif

	/*打包成TFTP格式*/
	len = makereq(TFTP_RRQ,TFTP_OCTET,cfg_filename,sendbuf,sizeof(sendbuf));

	retry = 0;
	
	sock = socket(AF_INET,SOCK_DGRAM,0x0);
	if(-1 == sock)
	{
		do_report(0,"TFTP Create socket Error,Exit! \r\n");
		return -1;
	}

  addr.sin_family =AF_INET;
  from.sin_family =AF_INET;
  addr.sin_port = htons(69);
  addr.sin_addr.s_addr   = dns_ip/*inet_addr(desthost)*/;
  ret = sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr));
  while(true)
  {
    FD_ZERO(&fdr);/*将fdr初始化为空集NULL*/
    FD_SET(sock, &fdr);
	ret = select(sock+1, &fdr, NULL,NULL, &timeout);/*确定一个或多个socket的状态，如需要则等待*/
	if(SOCKET_ERROR==ret)
	{
      do_report(0,"Socket error \r\n");
      return -1;
	}
	else if(0==ret)
	{
      if(MAX_RETRY==retry)
	  {
	    do_report(0,"Time Out \r\n");
		close(sock);
		return -1;
	  }
      retry++;
	}
	else
	{
	  if (FD_ISSET(sock,&fdr))
	  {
        retry = 0;
	    fromlen = sizeof(struct sockaddr);
        ret = recvfrom(sock,recvbuf,sizeof(recvbuf),0,(struct sockaddr *)&from,&fromlen);
	    if(TFTP_ERROR==recvbuf[1])
		{
			do_report(0,"Error %d: %s \r\n",recvbuf[3],&recvbuf[4]);
			close(sock);
			return -1;
		}
	    if(0==stat)
		{
			addr.sin_port = from.sin_port ;
			stat = 1;
		}
        if(TFTP_DATA==recvbuf[1])
		{
          lastdata = recvbuf[2]*256 + recvbuf[3];
		  len = makeack(lastdata,sendbuf,sizeof(sendbuf));
          test_ret = sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr));
		  if(ret<TFTP_NOTEND_DATALEN)
		  {/*接收最后一个数据包*/
				memcpy(config_buffer + flen,&recvbuf[4],ret-4);
				flen = flen + ret -4;
				do_report(0,"total %d byte received\r\n",flen);
				/*****需要添加: 解析配置信息*****/
				trip_string(config_buffer);
				
				close(sock);
				return 0;
		  }
          else
		  {/*接收前面N-1个包*/
				memcpy(config_buffer + flen,&recvbuf[4],512);
				flen = flen + 512;
				do_report(0,"%d byte received\r\n",flen);
		  }
		}
	  }
	}
  }

  close(sock);
  return -1;
}

/*检查序列号*/
int ch_tftp_CheckSerialNum(char* serial_buffer)
{
	U32 current_serial_num = 0;
	U32 lower_serial_num = 0;
	U32 high_serial_num = 0;
	U8 Lower_STBID[8],Higher_STBID[8];
	U8 STBID[8];
	U32 stb_id=0;
	int iloop=0;
	
	if((serial_buffer[17] != '-')||(serial_buffer[35] != ';'))
	{
		do_report(0,"serial range error!\r\n");
		return -1;
	}
	memset(Lower_STBID,0,8);
	memset(Higher_STBID,0,8);

	Lower_STBID[0] = (serial_buffer[0]-0x30)*10 + (serial_buffer[1]-0x30);
	Lower_STBID[1] = (serial_buffer[2]-0x30)*10 + (serial_buffer[3]-0x30);
	Lower_STBID[2] = (serial_buffer[4]-0x30)*10 + (serial_buffer[5]-0x30);
	Lower_STBID[3] = (serial_buffer[6]-0x30)*10 + (serial_buffer[7]-0x30);
	Lower_STBID[4] = (serial_buffer[8]-0x30)*10 + (serial_buffer[9]-0x30);
	lower_serial_num = atoi(&serial_buffer[10]);

	Higher_STBID[0] = (serial_buffer[18]-0x30)*10 + (serial_buffer[19]-0x30);
	Higher_STBID[1] = (serial_buffer[20]-0x30)*10 + (serial_buffer[21]-0x30);
	Higher_STBID[2] = (serial_buffer[22]-0x30)*10 + (serial_buffer[23]-0x30);
	Higher_STBID[3] = (serial_buffer[24]-0x30)*10 + (serial_buffer[25]-0x30);
	Higher_STBID[4] = (serial_buffer[26]-0x30)*10 + (serial_buffer[27]-0x30);
	high_serial_num = atoi(&serial_buffer[28]);

	if((lower_serial_num == 0)&&(high_serial_num == 0))
	{/*如果起至序列号均为0,表示所有机器全升级*/
		return 0;
	}

	/*****获取本机序列号*******/
	CH_LDR_ReadSTBID(STBID,&stb_id);

	/*****对比本机序列号与配置文件中的序列号*****/
	for(iloop=0; iloop<3/*5*/; iloop++)/*暂时不比较年号和周号*/
	{
		if(STBID[iloop] != Lower_STBID[iloop])
		{/*如果不相等,返回错误*/
			do_report(0,"Lower_STBID error!\r\n");
			return -1;
		}
	}

	for(iloop=0; iloop<3/*5*/; iloop++)/*暂时不比较年号和周号*/
	{
		if(STBID[iloop] != Higher_STBID[iloop])
		{/*如果不相等,返回错误*/
			do_report(0,"Higher_STBID error!\r\n");
			return -1;
		}
	}

	if((stb_id < lower_serial_num)||(stb_id > high_serial_num))
	{/*本机机号不在区域内*/
		do_report(0,"error: Current STBID not in range!\r\n");
		return -1;
	}

	return 0;
}

/*检查配置文件是否有更新数据
-1: 无升级
0: 如果有升级信息，则提示用户，由用户选择是否升级；
1: 如果有升级信息不提示用户，立即进行升级；
其他值: 非法,无升级;
*/
int ch_tftp_CheckDldInfo(void)
{
	unsigned int StbVersion = 0;
	U8 STBID[8];
	U32 stb_id=0;

	/*****获取本机序列号*******/
	/*CH_LDR_ReadSTBID(STBID,&stb_id);*/
	CH_LDR_ReadMainSTBID(STBID);
	if(	tftp_config_file.manufacture_code != STBID[0])
	{/*检查厂商号*/
		do_report(0,"TFTP Check Manufacture_code error[%d]!=[%d]!\r\n",tftp_config_file.manufacture_code, STBID[0]);
		return -1;
	}

	if(	tftp_config_file.stb_type!= STBID[1])
	{/*检查厂商号*/
		do_report(0,"TFTP Check stb_type error!\r\n");
		return -1;
	}

	if(	tftp_config_file.hardware_version != HARDWARE_ID)
	{
		do_report(0,"TFTP:Check HARDWARE_ID ERROR!\r\n");
		return -1;
	}

/*	if(CH_LDR_ReadSTBConfig() == 0)*/
	{
		GetSTBVersion(&StbVersion);
		/*软件版本强制升级标志为0表示按照版本由低到高升级；为1表示不管版本强制升级；其他值非法。*/
		if(1 == tftp_config_file.version_update_mode)
		{/*为1表示不管版本强制升级*/
			if(/*hl_infor.ucSoftwareVerMinor*/StbVersion == tftp_config_file.program_version)
			{/*本机软件版本与配置信息中的版本相同，不进入升级*/
				do_report(0,"TFTP:The software version is same!\r\n");
				return -1;
			}
		}
		else if(0 == tftp_config_file.version_update_mode)
		{/*为0表示按照版本由低到高升级*/
			if(/*hl_infor.ucSoftwareVerMinor*/StbVersion >= tftp_config_file.program_version)
			{/*本机软件版本比配置信息中的版本高，不进入升级*/
				do_report(0,"TFTP:The current software version is Newer!\r\n");
				return -1;
			}
		}
	}

	if(tftp_cfg_serial_in_range == false)
	{
		do_report(0,"TFTP:Current serial id not in range\r\n");
		return -1;
	}

	/*主程序强制升级标志
	为0表示如果有升级信息，则提示用户，由用户选择是否升级；
	为1表示如果有升级信息不提示用户，就进行升级；
	其他值非法。*/
	if((0 != tftp_config_file.app_update_mode)&&(1 != tftp_config_file.app_update_mode))
	{
		return -1;
	}
	else
	{
		return(tftp_config_file.app_update_mode);
	}
}


#if 1
#define TFTP_DOWNLOAD_FLAG	5
#define TFTP_DOWNLOAD_AUTO	101
#define TFTP_DOWNLOAD_FORCE	201
#define  LDR_NVM_APPLICATION_MEM_START (8100)
extern semaphore_t  semNvmAccessLock;
/*app检测到tftp升级后设置标志*/
/*第五个字节*/
int CH_LDRV_NVMRead(U8 *pbBuffer, U32 wOffset, U32 wLength)
{
	U32  len,i;

	semaphore_wait ( &semNvmAccessLock );

	for ( i=0;i<100;i++)
	{
		if(( STE2P_Read(hE2PDevice,
						  wOffset + LDR_NVM_APPLICATION_MEM_START,
						  pbBuffer,
		 				  wLength, &len))!=ST_NO_ERROR  || len != wLength )
		{

		}
		else
		{
			   break;
		}
	}
	semaphore_signal ( &semNvmAccessLock );
	return 0;
}

int CH_LDRV_NVMWrite( U8 *pbBuffer,U32 wOffset,U32 wLength )
{

	int i;
	U32 len;
	ST_ErrorCode_t ReturnError;
	   semaphore_wait ( &semNvmAccessLock );

	for ( i = 0;i < 100;i++ )
	{
		ReturnError= STE2P_Write(hE2PDevice,
								  wOffset + LDR_NVM_APPLICATION_MEM_START,
								  pbBuffer,
								  wLength,
								  &len);
		
		if(ReturnError==ST_NO_ERROR && len == wLength)
		{
			   semaphore_signal ( &semNvmAccessLock );
			return 0;
		}
		do_report(0,"nvm write err:offs:%x,len:%x\n",wOffset,wLength);
	}
	   semaphore_signal ( &semNvmAccessLock );

	return -1;
}

void CH_TFTP_SetAutoDld(void)
{
	U8	tmp_nvm[22];
	U8 i,tmpcrc = 0;
	U8 force_download =TFTP_DOWNLOAD_AUTO;
	
    CH_LDRV_NVMRead(tmp_nvm,0,20);
	tmp_nvm[TFTP_DOWNLOAD_FLAG] = force_download ;
	for(i=0;i<18;i++)
	{
		tmpcrc += tmp_nvm[i];
	}
	tmp_nvm[19] = tmpcrc;
	CH_LDRV_NVMWrite(tmp_nvm,0,20);
}

/*Downloader检测到tftp升级后将原100标志置为200
以标志本次升级的方式，当保存文件失败时，先检测此标志进入相应的升级方式*/
/*第五个字节*/
void CH_TFTP_SetForceDld(void)
{
	U8	tmp_nvm[22];
	U8 i,tmpcrc = 0;
	U8 force_download = TFTP_DOWNLOAD_FORCE;
	
    CH_LDRV_NVMRead(tmp_nvm,0,20);
	tmp_nvm[TFTP_DOWNLOAD_FLAG] = force_download ;
	for(i=0;i<18;i++)
	{
		tmpcrc += tmp_nvm[i];
	}
	tmp_nvm[19] = tmpcrc;
	CH_LDRV_NVMWrite(tmp_nvm,0,20);
}

/*读取IP LOADER升级标志*/
U8 CH_TFTP_ReadIPDLD( void )
{
	U8	tmp_nvm[22];
	U8 i,tmpcrc = 0;

	CH_LDRV_NVMRead(tmp_nvm,0,20);
	for(i = 0;i < 18;i++)
	{
		tmpcrc += tmp_nvm[i];
	}
	if(tmpcrc != tmp_nvm[19])
	{
		return 0;
	}
	else
	{
		return tmp_nvm[TFTP_DOWNLOAD_FLAG];
	}
}

/*清除TFTP升级标志*/
void CH_TFTP_ClearIPDLD(void )
{
	U8	tmp_nvm[22];
	U8	i,tmpcrc = 0;

	CH_LDRV_NVMRead(tmp_nvm,0,20);
	tmp_nvm[TFTP_DOWNLOAD_FLAG] = 0 ;
	for(i = 0;i < 18;i++)
	{
		tmpcrc += tmp_nvm[i];
	}
	tmp_nvm[19] = tmpcrc;
	CH_LDRV_NVMWrite(tmp_nvm,0,20);
}

#endif


