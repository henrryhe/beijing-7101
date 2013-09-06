/*--------------------------------------------------------------------
file_name:snmp.c
description:
	SNMP管理程序，向SNMP代理程序发送snmp报文并根据返回报文判断代理程序
	所在的模块是否运行正常。此处主要用于内置cable modem模块是否上线，
	其中SNMP数据报文的编码格式是DER编码格式。目前支持单个OID查询。
author:wufei
date:2008-8-8
---------------------------------------------------------------------*/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "CHDRV_NET.h"
#include "CHMID_TCPIP_api.h"
#include "CHMID_CM_api.h"

/*-----------------底层函数的宏定义---------------------------------*/
#define MALLOC(a)		CHDRV_NET_MemAlloc(a)
#define FREE(p)			CHDRV_NET_MemFree(p)

typedef  CHDRV_NET_SEMAPHORE  SNMP_SEMOPHARE;
#define snmp_semaphore_signal(pSemaphore) CHDRV_NET_SemaphoreSignal(pSemaphore)
#define snmp_semaphore_wait(pSemaphore) CHDRV_NET_SemaphoreWait(pSemaphore,-1)
#define snmp_semaphore_create(count,pSemaphore) CHDRV_NET_SemaphoreCreate(count,pSemaphore)
						
#define snmp_printf		CHDRV_NET_Print
/*---------------------------全局变量------------------------------*/
static SNMP_SEMOPHARE gSnmp_Get = NULL;
static CHMID_CM_ONLINE_STATUS_e gCM_Status = CHMID_CM_OFFLINE;
static u32_t gCM_CheckCycleTime = 5000;
static u8_t gCM_Opened = 1;
/*----------------------------------------------------------------*/
#define DST_ADDR		"192.168.100.1"
#define DST_PORT		161

#define SNMP_VER		1
#define SNMP_COMMUNITY	"public"
#define SNMP_STATUS_OBJ_ID		"1.3.6.1.2.1.10.127.1.2.2.1.1.2"	/*CM status*/
#define SNMP_MAC_OBJ_ID "1.3.6.1.4.1.18671.1.1.3.1.16.0"	/*CM MAC address*/
#define SNMP_GET_SN_OBJ_ID    "1.3.6.1.4.1.18671.1.1.2.1.2.5.0"/*GET S/N number*/
#define SNMP_SET_DOWN_FREQ  "1.3.6.1.4.1.18671.1.1.2.2.2.2.0"/*SET down frequency*/
#define SNMP_CM_REBOOT	  "1.3.6.1.2.1.69.1.1.3.0" /*Reboot cm*/
#define SNMP_GET_UP_FREQ	"1.3.6.1.2.1.10.127.1.1.2.1.2.4"/*Get up freq*/
#define SNMP_GET_DOWN_FREQ "1.3.6.1.2.1.10.127.1.1.1.1.2.3"/*Get down freq*/

#define SEND_BUF_LEN		128
static unsigned char sendbuf[SEND_BUF_LEN];
static CHDRV_NET_Config_t netCfgTemp;

typedef enum
{
	SNMP_GET,
	SNMP_GET_NEXT,
	SNMP_GET_RESP,
	SNMP_SET_REQ,
	SNMP_TRAP
}SNMP_PDU_TYPE_t;

typedef struct
{
	unsigned char version;
	char * community_type;
	SNMP_PDU_TYPE_t pdu_type;
	unsigned char req_id;
	int err_status;
	int err_index;
	char * oid;
	void * value;
}snmp_header_t;
typedef enum
{
	GET_ONLINE_STATUS,
	GET_MAC_STATUS,
	GET_SN_NUMBER,
	SET_DOWN_FRE,
	GET_UP_FREQ,
	GET_DOWN_FREQ,
	REBOOT_CM
}SNMP_CM_STATUS_TYPE;


static void Snmp_manulSetIp(IP_GET_MODEL GetIpMod);

/*-------------------------------------------------------------
构造OID数据头
--------------------------------------------------------------*/
int make_OID_header(const char * oid,unsigned char ** oid_header)
{
	unsigned char * buf = NULL;
	int buf_len = 0,node_num = 0,i = 0,loop = 0;
	int str_len = strlen(oid);
	char oid_buf[11] = {0};
	int oid_element[128];
	unsigned char code_buf[128];
	int buf_index = 0,oid_element_index = 0;

	/*传入参数错误判断*/
	if(oid == NULL || str_len == 0)
	{
		return -1;
	}

	memset(oid_buf,0,11);
	memset(oid_element,0,128);
	/*计算出节点数*/
	for(loop = 0;loop <= str_len;loop++)
	{
		if(oid[loop] == '.' || oid[loop] == '\0')
		{
			memset(oid_buf,0,11);
			node_num++;			
			if(node_num == 1)/*第一次出现*/
			{
				memcpy(oid_buf,&oid[0],loop+1);/**/
				oid_buf[loop + 1] = '\0';
				oid_element[0] = atoi(oid_buf);
			}
			else if((1< node_num) && (node_num < 128))
			{/*提取两次。之间的数字*/
				for(i = 1;i <= 10;i++)
				{
					if(oid[loop-i] == '.')
					{
						memcpy(oid_buf,&oid[loop-i+1],i-1);
						oid_buf[i] = '\0';
						oid_element[++oid_element_index] = atoi(oid_buf);
						break;
					}
				}
			}
		}
	}
	/*计算oid_element在编码后的长度,并将编码后的OID放入缓存code_buf中*/
	buf_index = 0;
	for(loop = 2;loop <= oid_element_index;loop++)
	{
		if((0 <= oid_element[loop]) && (oid_element[loop] <= 0xff))
		{
			code_buf[buf_index] = (unsigned char)(oid_element[loop]&0xff);
			buf_index++;
		}
		else if((0xff < oid_element[loop]) && (oid_element[loop] <= 0xffff))
		{
			code_buf[buf_index] = 0x81/*(unsigned char)((oid_element[loop]>>8)&0xff)*/;
			code_buf[++buf_index] = 0x91/*(unsigned char)(oid_element[loop]&0xff)*/;
			code_buf[++buf_index] = 0x6f;
			buf_index++;
		}
		else if((0xffff < oid_element[loop]) && (oid_element[loop] <= 0xffffff))
		{
			code_buf[buf_index] = (unsigned char)((oid_element[loop]>>16)&0xff);
			code_buf[++buf_index] = (unsigned char)((oid_element[loop]>>8)&0xff);
			code_buf[++buf_index] = (unsigned char)(oid_element[loop]&0xff);
			buf_index++;
		}
		else if((0xffffff < oid_element[loop]) && (oid_element[loop] <= 0xffffffff))
		{
			code_buf[buf_index] = (unsigned char)((oid_element[loop]>>24)&0xff);
			code_buf[buf_index] = (unsigned char)((oid_element[loop]>>16)&0xff);
			code_buf[++buf_index] = (unsigned char)((oid_element[loop]>>8)&0xff);
			code_buf[++buf_index] = (unsigned char)(oid_element[loop]&0xff);
			buf_index++;
		}
	}
	buf_len = buf_index + 3;/*0x06头占用1BYTE*/
	buf = (unsigned char *)MALLOC(buf_len);
	if(buf == NULL)
	{
		snmp_printf("ERROR:malloc memory fail!\n");
		return -1;
	}
	memset(buf,0,buf_len);
	buf[0] = 0x06;
	buf[1] = buf_index + 1;/*第一第二个OID占用一个BYTE*/
	/*第一和第二个oid需转换成ASN.1格式*/
	buf[2] = 40*oid_element[0] + oid_element[1];
	for(loop = 3;loop < buf_len; loop++)
	{
		buf[loop] = code_buf[loop - 3];
	}
	/*memcpy(&buf[3],code_buf,buf_index);*/
	
	*oid_header = buf;

	return buf_len;

}
/*----------------------------------------------------------------
构造整型数据头
暂不支持负数
-----------------------------------------------------------------*/
int make_INTEGER_header(unsigned int INTEGER,unsigned char ** integer_header)
{
	int integer_len = 0,buf_len = 0,loop = 0;
	unsigned char * buf = NULL;

	if((0x00U <= INTEGER) && (INTEGER <= 0xffU))
	{
		integer_len = 1;
	}
	else if((0xffU < INTEGER) && (INTEGER <= 0xffffU))
	{
		integer_len = 2;
	}
	else if((0xffffU < INTEGER) && (INTEGER <= 0xffffffU))
	{
		integer_len = 3;
	}
	else if((0xffffffU < INTEGER) && (INTEGER <= 0xffffffffU))
	{
		integer_len = 4;
	}
	else
	{
		return -1;/*暂不支持负数*/
	}

	buf_len = integer_len + 2;/*0x02 INTEGER头和integer_len本身占用1BYTE*/
	buf = (unsigned char *)MALLOC(buf_len);
	if(buf == NULL)
	{
		return -1;
	}
	memset(buf,0,buf_len);
	/*向缓存中填充头数据*/
	buf[0] = 0x02;/*INTEGER 头*/
	buf[1] = (unsigned char )integer_len;/*数据长度*/
	for(loop = integer_len;loop > 0;loop--)
	{
		buf[integer_len - loop + 2] = (INTEGER>>((loop-1)*8))&0xff;
	}

	*integer_header = buf;
	return buf_len;
}
/*---------------------------------------------------------------------

构造OCTET_STRING数据头
----------------------------------------------------------------------*/
int make_OCTET_STRING_header(const char * string,unsigned char ** OS_header)
{
	int str_len = strlen(string);
	int buf_len,str_len_len = 0;
	unsigned char * buf = NULL;

	if(string == NULL || str_len == 0)
	{
		return -1;
	}
	/*字符串长度是否大于255*/
	if(str_len < 256)
	{
		str_len_len = 1;
	}
	else
	{
		str_len_len = 2;
	}
	buf_len = 1/*0x04头*/ + str_len_len + str_len;
	/*分配内存*/
	buf = (unsigned char *)MALLOC(buf_len);
	if(buf == NULL)
	{
		snmp_printf("ERROR:malloc memory fail!\n");
		return -1;
	}
	memset(buf,0,buf_len);

	buf[0] = 0x04;
	if(str_len_len == 1)
	{
		buf[1] = str_len&0xff;
		memcpy(&buf[2],string,str_len);
	}
	else
	{
		buf[1] = str_len&0xff00;
		buf[2] = str_len&0xff;
		memcpy(&buf[2],string,str_len);
	}
	
	*OS_header = buf;
	return buf_len;
}

/*--------------------------------------------------------------
构造NULL头
---------------------------------------------------------------*/
int make_VALUE_header(const char * string,unsigned char ** header)
{
	int str_len = strlen(string);
	unsigned char * buf = NULL;

	if(string == NULL || str_len == 0)
	{
		return -1;
	}
	if(strcmp(string,"NULL") == 0)
	{
	buf = (unsigned char *)MALLOC(2);
	if(buf == NULL)
	{
		return -1;
	}
	memset(buf,0,2);
	/*NULL头的DER编码格式固定为0x05 0x00*/
	buf[0] = 0x05;
	buf[1] = 0x00;

	*header = buf;
	return 2;
}
	else
	{
		unsigned int iValue = atoi(string);
		return make_INTEGER_header(iValue,header);
	}
}
/*----------------------------------------------------------------
对PDU类型头进行编码
----------------------------------------------------------------*/
int make_PDU_TYPE_header(SNMP_PDU_TYPE_t pdu_type,unsigned char ** header)
{
	unsigned char * buf = NULL;

	buf = (unsigned char *)MALLOC(1);
	if(NULL == buf)
	{
		return -1;
	}
	buf[0] = (unsigned char)pdu_type|0xa0;
	/*buf[1] = 0x1b;*/
	*header = buf;
	return 1;
}
/*----------------------------------------------------------------
*
* 解析整形数据
*----------------------------------------------------------------*/
u32_t parse_INTEGER_header(u8_t * buf)
{
	u8_t integer_len = buf[1];
	u8_t loop = 0;
	u32_t ui_Ret = 0;
	
	if(buf[0] != 2)/*TYPE==2*/
	{
		snmp_printf("\nparse_INTEGER_header error:data type[%02x] is not 2\n",buf[0]);
		return 0xfffffffful;
	}
	for(loop = integer_len;loop > 0;loop--)
	{
		ui_Ret += ((u32_t)buf[loop+1])<<((integer_len-loop)*8);
	}
	return ui_Ret;
}

char * parse_OCTET_STRING_header(unsigned char * buf)
{
	return 0;
}
/*-----------------------------------------------------------
DES:组装及其发送SNMP数据报文包，此函数仅支持一组OID，因此没有对返回
	报文进行详细解析，仅取返回报文中最后一个字节值作为状态值
RETURN:返回12为上线否则为下线
-----------------------------------------------------------*/
int SNMP_GetCMStatus(snmp_header_t * head,SNMP_CM_STATUS_TYPE status_type,unsigned char * * buffer)
{
	int fd = -1,iErr = 0;
	int ret = 0;
	CHMID_CM_ONLINE_STATUS_e uCmStatus = CHMID_CM_OFFLINE;
	unsigned char * pVersionHeader = NULL,\
				 * pCommHeader = NULL,\
				 * pPDU_typeHeader = NULL,\
				 * pReqIdHeader = NULL,\
				 * pErrStatusHeader = NULL,\
				 * pErrIndexHeader = NULL,\
				 * pOidHeader = NULL,\
				 * pValueHeader = NULL;
	int VersionHeader_len = 0,\
		CommHeader_len = 0,\
		PDU_typeHeader_len = 0,\
		ReqIdHeader_len = 0,\
		ErrStatusHeader_len = 0,\
		ErrIndexHeader_len = 0,\
		OidHeader_len = 0,\
		ValueHeader_len = 0;
	int cursor = 0,buf_len = 0;/*组合后的编码长度*/
	int addr_len = sizeof(CHMID_TCPIP_SockAddr_t);
	int i = 0;
	int i_a = 0,i_b = 0,i_c = 0,i_d = 0;
	CHMID_TCPIP_SockAddr_t dst_addr;
	
	if(gSnmp_Get == NULL)
	{
		snmp_semaphore_create(1,&gSnmp_Get);
	}
	snmp_semaphore_wait(gSnmp_Get);
	if(status_type != GET_ONLINE_STATUS)
	{
		/*保存当前网络配置信息*/
		CHMID_TCPIP_GetNetConfig(&netCfgTemp);
		/*设置IP配置为192.168.100.X段的IP地址*/		
		Snmp_manulSetIp(MANUAL);
	}
	/*对每个头进行编码*/
	VersionHeader_len = make_INTEGER_header(head->version,&pVersionHeader);
	CommHeader_len = make_OCTET_STRING_header(head->community_type,&pCommHeader);
	PDU_typeHeader_len = make_PDU_TYPE_header(head->pdu_type,&pPDU_typeHeader);
	ReqIdHeader_len = make_INTEGER_header(head->req_id,&pReqIdHeader);
	ErrStatusHeader_len = make_INTEGER_header(head->err_status,&pErrStatusHeader);
	ErrIndexHeader_len = make_INTEGER_header(head->err_index,&pErrIndexHeader);
	OidHeader_len = make_OID_header(head->oid,&pOidHeader);
	ValueHeader_len = make_VALUE_header((char *)head->value,&pValueHeader);
	
	if(VersionHeader_len < 0 ||CommHeader_len < 0
		||PDU_typeHeader_len < 0 || ReqIdHeader_len<0
		||ErrStatusHeader_len < 0||ErrIndexHeader_len< 0
		||OidHeader_len < 0||ValueHeader_len<0)
	{
		/*return -1;*/
		iErr =  -1;
		goto done;
	}
	
	/*编码后的头长度计算*/
	buf_len = ValueHeader_len + OidHeader_len;
	i_a = buf_len;
	if(buf_len <= 0xff)
		buf_len += 1;
	else 
		buf_len += 2;
	buf_len++;/*0x30*/
	i_b = buf_len;
	if(buf_len <= 0xff)
		buf_len += 1;
	else 
		buf_len += 2;
	buf_len++;/*0x30*/
	buf_len+=ErrIndexHeader_len;
	buf_len+=ErrStatusHeader_len;
	buf_len+=ReqIdHeader_len;
	i_c = buf_len;
	if(buf_len <= 0xff)
		buf_len += 1;
	else 
		buf_len += 2;
	buf_len+=PDU_typeHeader_len;
	buf_len+=CommHeader_len;
	buf_len+=VersionHeader_len;
	i_d = buf_len;
	if(buf_len <= 0xff)
		buf_len += 1;
	else 
		buf_len += 2;
	buf_len++;/*0x30*/

	if((buf_len+2) > SEND_BUF_LEN)
	{
		snmp_printf("The head is too long!!\n");
		iErr = -1;
		goto done;
	}
	/*拼接数据*/
	memset(sendbuf,0,SEND_BUF_LEN);
	cursor = 0;
	sendbuf[cursor] = 0x30;
	cursor++;
	if(i_d < 0xff)
	{
		sendbuf[cursor] = i_d;
		cursor++;
	}
	else
	{
		sendbuf[cursor] = (i_d>>8)&0xff;
		sendbuf[cursor+1] = i_d&0xff;
		cursor+=2;
	}
	memcpy(&sendbuf[cursor],pVersionHeader,VersionHeader_len);
	cursor+=VersionHeader_len;
	memcpy(&sendbuf[cursor],pCommHeader,CommHeader_len);
	cursor+=CommHeader_len;
	memcpy(&sendbuf[cursor],pPDU_typeHeader,PDU_typeHeader_len);
	cursor+=PDU_typeHeader_len;
	if(i_c < 0xff)
	{
		sendbuf[cursor] = i_c;
		cursor++;
	}
	else
	{
		sendbuf[cursor] = (i_c>>8)&0xff;
		sendbuf[cursor+1] = i_c&0xff;
		cursor+=2;
	}
	memcpy(&sendbuf[cursor],pReqIdHeader,ReqIdHeader_len);
	cursor+=ReqIdHeader_len;
	memcpy(&sendbuf[cursor],pErrStatusHeader,ErrStatusHeader_len);
	cursor+=ErrStatusHeader_len;
	memcpy(&sendbuf[cursor],pErrIndexHeader,ErrIndexHeader_len);
	cursor+=ErrIndexHeader_len;
	sendbuf[cursor] = 0x30;
	cursor++;
	if(i_b < 0xff)
	{
		sendbuf[cursor] = i_b;
		cursor++;
	}
	else
	{
		sendbuf[cursor] = (i_b>>8)&0xff;
		sendbuf[cursor+1] = i_b&0xff;
		cursor+=2;
	}
	sendbuf[cursor] = 0x30;
	cursor++;
	if(i_a < 0xff)
	{
		sendbuf[cursor] = i_a;
		cursor++;
	}
	else
	{
		sendbuf[cursor] = (i_a>>8)&0xff;
		sendbuf[cursor+1] = i_a&0xff;
		cursor+=2;
	}
	memcpy(&sendbuf[cursor],pOidHeader,OidHeader_len);
	cursor+=OidHeader_len;
	memcpy(&sendbuf[cursor],pValueHeader,ValueHeader_len);
	cursor+=ValueHeader_len;

	/**/
	dst_addr.us_Port = CHMID_TCPIP_HTONS(DST_PORT);
	dst_addr.us_Family = AF_INET;
	dst_addr.uc_Len = addr_len;
	CHMID_TCPIP_InetAton(DST_ADDR,&(dst_addr.ui_IP));
	/**/
	fd = CHMID_TCPIP_Socket(AF_INET,SOCK_DGRAM,0);
	if(fd < 0)
	{
		snmp_printf("ERROR:Create socket fail!\n");
		iErr =  -1;
		goto done;
	}
#ifdef m081201__
{
		snmp_printf("<<<send>>>\n");
		for(i = 0;i < SEND_BUF_LEN;i++)
		{
			if(i%16 == 0 && i != 0)
			{
				snmp_printf("\n");
			}	
			snmp_printf("%02x ",sendbuf[i]);

		}
		snmp_printf("\n");
}
#endif
	ret = CHMID_TCPIP_SendTo(fd,sendbuf,SEND_BUF_LEN,0,&dst_addr,addr_len);
	if(ret < ++cursor)
	{
		snmp_printf("ERROR:send buffer fail!need send[%d],real send[%d]\n",cursor,ret);
		iErr =  -1;
		goto done;
	}
	memset(sendbuf,0,SEND_BUF_LEN);
	ret = CHMID_TCPIP_RecvFrom(fd,sendbuf,SEND_BUF_LEN,0,(struct sockaddr *)(&dst_addr),&addr_len);
	if(ret <= 0)
	{
		snmp_printf("ERROR:recv buffer fail!ret = [%d]\n",ret);
		iErr =  -1;
		goto done;
	}
	else
	{
#ifdef m081201__
		snmp_printf("<<<recv>>>\n");
		for(i = 0;i < SEND_BUF_LEN;i++)
		{
			if(i%16 == 0 && i != 0)
			{
				snmp_printf("\n");
			}	
			snmp_printf("%02x ",sendbuf[i]);

		}
		snmp_printf("\n");
#endif
	}
done:
	if(fd >= 0)
	{
		CHMID_TCPIP_Close(fd);
	}
	FREE(pVersionHeader);
	FREE(pCommHeader);
	FREE(pPDU_typeHeader );
	FREE(pReqIdHeader );
	FREE(pErrStatusHeader );
	FREE(pErrIndexHeader );
	FREE(pOidHeader );
	FREE(pValueHeader );
	/*恢复原来的IP配置*/
	if(status_type != GET_ONLINE_STATUS)
	{
		Snmp_manulSetIp(AUTO_DHCP);
	}
	if(iErr == 0 && status_type == GET_ONLINE_STATUS)/*获取在线状态*/
	{
		iErr = sendbuf[ret-1];
	}
	else if(iErr == 0 && status_type == GET_MAC_STATUS)/*获取MAC地址*/
	{
		memcpy(*buffer,&sendbuf[46],6);
	}
	else if(iErr == 0 && status_type == GET_SN_NUMBER)/*获取S/N值*/
	{
		memcpy(*buffer,&sendbuf[47],sendbuf[46]);
		snmp_printf("\nSN NUMBER:%s\n",*buffer);
	}
	else if(iErr == 0 && status_type == GET_UP_FREQ)/*获取上行频点值*/
	{
		u32_t up_freq = parse_INTEGER_header(&sendbuf[43]);
		if(buffer == NULL)
		{
			snmp_printf("\nInvalid params!!\n");
			iErr = -1;
		}
		else
		{
			sprintf(*buffer,"%d",up_freq);
			snmp_printf("\nGET_UP_FREQ:%s\n",*buffer);
		}
	}
	else if(iErr == 0 && status_type == GET_DOWN_FREQ)/*获取下行频点值*/
	{
		u32_t down_freq = parse_INTEGER_header(&sendbuf[43]);
		if(buffer == NULL)
		{
			snmp_printf("\nInvalid params!!\n");
			iErr = -1;
		}
		else
		{
			sprintf(*buffer,"%d",down_freq);
			snmp_printf("\nGET_DOWN_FREQ:%s\n",*buffer);
		}
	}
	snmp_semaphore_signal(gSnmp_Get);
	return iErr;
}
/*---------------------------------------------------------------
DES:主入口,在此函数中设置snmp包头参数值
RETURN:CM上线返回1否则返回0
---------------------------------------------------------------*/
CHMID_CM_ONLINE_STATUS_e SNMP_CM_CheckCMStatus(void)
{
	int iStatus = 0;
	snmp_header_t head;
	CHMID_CM_RESULT_e RetStatus = CHMID_CM_INITED;
	static unsigned char req_id = 1;
	
	head.version = SNMP_VER - 1;
	head.community_type = SNMP_COMMUNITY;
	head.pdu_type = SNMP_GET;
	head.req_id = req_id;
	head.err_index = 0;
	head.err_status = 0;
	head.oid = SNMP_STATUS_OBJ_ID;
	head.value = "NULL";
	iStatus = SNMP_GetCMStatus(&head,GET_ONLINE_STATUS,NULL);
	if(iStatus < 0){
		snmp_printf("SNMP_GetCMStatus ERROR!return value = [%d]\n",iStatus);
		RetStatus = CHMID_CM_CHECK_FAIL;
	}else if(iStatus >= 0 && iStatus < 4){
		snmp_printf("CM is OFFLINE!CM status = [%d]\n",iStatus);
		RetStatus = CHMID_CM_INITED;
	}else if(iStatus >= 4 && iStatus <= 5){
		snmp_printf("CM is OFFLINE!CM status = [%d]\n",iStatus);
		RetStatus = CHMID_CM_DOWN_FREQ_LOCKED;
	}else if( iStatus == 6){
		snmp_printf("CM is OFFLINE!CM status = [%d]\n",iStatus);
		RetStatus = CHMID_CM_UP_FREQ_LOCKED;
	}else if(iStatus > 6 && iStatus < 12){
		snmp_printf("CM is OFFLINE!CM status = [%d]\n",iStatus);
		RetStatus = CHMID_CM_OFFLINE;
	}else if(iStatus == 12){
		snmp_printf("CM is ONLINE!CM status = [%d]\n",iStatus);
		RetStatus = CHMID_CM_ONLINE;
	}
	req_id++;
	if(req_id == 0xff)
	{
		req_id = 1;
	}
	return RetStatus;
}
/*
当CM不在线时候，手动给机顶盒设置IP，以便与CM进行通信
*/
static void Snmp_manulSetIp(IP_GET_MODEL GetIpMod)
{
#if 0
	CHDRV_NET_Config_t netCfg;
	u16_t netStatus = 0;
	u16_t waitLoop = 20;

	if(CHMID_CM_CheckCMStatus() == CHMID_CM_ONLINE)
		return;
	CHMID_TCPIP_GetNetConfig(&netCfg);
	if(GetIpMod == AUTO_DHCP && memcmp(&netCfg,&netCfgTemp,sizeof(CHDRV_NET_Config_t))!= 0)
	{
		memcpy(&netCfg,&netCfgTemp,sizeof(CHDRV_NET_Config_t));
		CHMID_TCPIP_NetConfigRenew(netCfg);
		snmp_printf("SNMP SET AUTO_MODE out\n");
		return;
	}
	else if(!(netCfgTemp.ipaddress[0]==192	\
		&&netCfgTemp.ipaddress[1]==168	\
		&&netCfgTemp.ipaddress[2]==100	\
		))
	{
		netCfg.ipmodel = MANUAL;
		netCfg.ipaddress[0] = 192;
		netCfg.ipaddress[1] = 168;
		netCfg.ipaddress[2] = 100;
		netCfg.ipaddress[3] = 100;
		netCfg.netmask[0] = 255;
		netCfg.netmask[1] = 255;
		netCfg.netmask[2] = 255;
		netCfg.netmask[3] = 0;
		netCfg.gateway[0] = 192;
		netCfg.gateway[1] = 168;
		netCfg.gateway[2] = 100;
		netCfg.gateway[3] = 1;
		
		CHMID_TCPIP_NetConfigRenew(netCfg);
		do{
			netStatus = CHMID_TCPIP_GetNetStatus();
			if(netStatus&CHMID_TCPIP_USABLE){
				break;
			}else{
				CHDRV_NET_ThreadDelay(500);
			}
		}while(waitLoop--);
		snmp_printf("SNMP SET MANUL_MODE out\n");
		return;
	}
	else
		return ;
#endif
}
void Snmp_CM_Process(void * arg)
{
	SNMP_CM_STATUS_TYPE cmStatus = CHMID_CM_OFFLINE;
	s32_t i_loop = 0;
	
	while(1)
	{
		/*关闭监控功能*/
		if(!gCM_Opened)
		{
			CHDRV_NET_ThreadDelay(1000);
			continue;
		}
		cmStatus = SNMP_CM_CheckCMStatus();
		/*如果下线则多检测几次*/
		if(cmStatus != CHMID_CM_ONLINE){
			for(i_loop = 0;i_loop<1;i_loop++){
				cmStatus = SNMP_CM_CheckCMStatus();
				if(cmStatus == CHMID_CM_ONLINE)	
					break;
				else
					CHDRV_NET_ThreadDelay(500);
			}
		}
		/*CM上线后更新一次IP*/
		if(cmStatus == CHMID_CM_ONLINE && gCM_Status != CHMID_CM_ONLINE){
			/*得到当前网络配置*/
			CHMID_TCPIP_GetNetConfig(&netCfgTemp);
			/*重新自动获取IP*/
			#if 0
			if((netCfgTemp.ipmodel == AUTO_DHCP)){
				CHMID_TCPIP_NetConfigRenew(netCfgTemp);
			}
			#endif
		}
		/*更新CM状态值*/
		gCM_Status = cmStatus;
		CHDRV_NET_ThreadDelay(gCM_CheckCycleTime);
	}
}
/*
返回0表示获取正确
*/
CHMID_CM_RESULT_e CHMID_CM_GetCMMac(u8_t * mac)
{
	int iStatus = 0;
	snmp_header_t head;
	static unsigned char req_id = 1;
	int iCounter = 100;
	CHMID_CM_ONLINE_STATUS_e uCmStatus = CHMID_CM_OFFLINE;
	
	if(mac == NULL)
		return -1;
	head.version = SNMP_VER - 1;
	head.community_type = SNMP_COMMUNITY;
	head.pdu_type = SNMP_GET;
	head.req_id = req_id;
	head.err_index = 0;
	head.err_status = 0;
	head.oid = SNMP_MAC_OBJ_ID;
	head.value = "NULL";

	iStatus = SNMP_GetCMStatus(&head,GET_MAC_STATUS,&mac);
	if(iStatus == 0)
		snmp_printf("SNMP_GetCMStatus success!return value = [%d]\n",iStatus);
	else 
		snmp_printf("SNMP_GetCMStatus ERROR!return value = [%d]\n",iStatus);

	req_id++;
	if(req_id == 0xff)
	{
		req_id = 1;
	}
	if(iStatus == 0)
	{
		return CHMID_CM_OK;
	}
	else
	{
		return CHMID_CM_FAIL;
	}
}
/*
返回0表示获取正确
*/
CHMID_CM_RESULT_e CHMID_CM_GetCMSNNumber(u8_t * SN_NO)
{
	int iStatus = 0;
	snmp_header_t head;
	static unsigned char req_id = 1;
	CHMID_CM_ONLINE_STATUS_e uCmStatus = CHMID_CM_OFFLINE;
	
	if(SN_NO == NULL)
		return CHMID_CM_INVALID_PARAM;
	head.version = SNMP_VER - 1;
	head.community_type = SNMP_COMMUNITY;
	head.pdu_type = SNMP_GET;
	head.req_id = req_id;
	head.err_index = 0;
	head.err_status = 0;
	head.oid = SNMP_GET_SN_OBJ_ID;
	head.value = "NULL";

	iStatus = SNMP_GetCMStatus(&head,GET_SN_NUMBER,&SN_NO);
	if(iStatus == 0)
		snmp_printf("SNMP_GetCMStatus success!return value = [%d]\n",iStatus);
	else 
		snmp_printf("SNMP_GetCMStatus ERROR!return value = [%d]\n",iStatus);

	req_id++;
	if(req_id == 0xff)
	{
		req_id = 1;
	}
	if(iStatus == 0)
	{
		return CHMID_CM_OK;
	}
	else
	{
		return CHMID_CM_FAIL;
	}
}
/*
返回0表示获取正确
*/
CHMID_CM_RESULT_e CHMID_CM_GetUPFreq(u8_t * uc_UPFreq)
{
	int iStatus = 0;
	snmp_header_t head;
	static unsigned char req_id = 1;
	
	if(uc_UPFreq == NULL)
		return CHMID_CM_INVALID_PARAM;
	head.version = SNMP_VER - 1;
	head.community_type = SNMP_COMMUNITY;
	head.pdu_type = SNMP_GET;
	head.req_id = req_id;
	head.err_index = 0;
	head.err_status = 0;
	head.oid = SNMP_GET_UP_FREQ;
	head.value = "NULL";

	iStatus = SNMP_GetCMStatus(&head,GET_UP_FREQ,&uc_UPFreq);
	if(iStatus == 0)
		snmp_printf("SNMP_GetCMStatus success!return value = [%d]\n",iStatus);
	else 
		snmp_printf("SNMP_GetCMStatus ERROR!return value = [%d]\n",iStatus);

	req_id++;
	if(req_id == 0xff)
	{
		req_id = 1;
	}
	if(iStatus == 0)
	{
		return CHMID_CM_OK;
	}
	else
	{
		return CHMID_CM_FAIL;
	}
}
/*
返回0表示获取正确
*/
CHMID_CM_RESULT_e CHMID_CM_GetDownFreq(u8_t * uc_DownFreq)
{
	int iStatus = 0;
	snmp_header_t head;
	static unsigned char req_id = 1;
	
	if(uc_DownFreq == NULL)
		return CHMID_CM_INVALID_PARAM;
	head.version = SNMP_VER - 1;
	head.community_type = SNMP_COMMUNITY;
	head.pdu_type = SNMP_GET;
	head.req_id = req_id;
	head.err_index = 0;
	head.err_status = 0;
	head.oid = SNMP_GET_DOWN_FREQ;
	head.value = "NULL";

	iStatus = SNMP_GetCMStatus(&head,GET_DOWN_FREQ,&uc_DownFreq);
	if(iStatus == 0)
		snmp_printf("SNMP_GetCMStatus success!return value = [%d]\n",iStatus);
	else 
		snmp_printf("SNMP_GetCMStatus ERROR!return value = [%d]\n",iStatus);

	req_id++;
	if(req_id == 0xff)
	{
		req_id = 1;
	}
	if(iStatus == 0)
	{
		return CHMID_CM_OK;
	}
	else
	{
		return CHMID_CM_FAIL;
	}
}
CHMID_CM_RESULT_e CHMID_CM_Reboot(void)
{
	int iStatus = 0;
	snmp_header_t head;
	static unsigned char req_id = 1;
	CHMID_CM_ONLINE_STATUS_e uCmStatus = CHMID_CM_OFFLINE;

	head.version = SNMP_VER - 1;
	head.community_type = SNMP_COMMUNITY;
	head.pdu_type = SNMP_SET_REQ;
	head.req_id = req_id;
	head.err_index = 0;
	head.err_status = 0;
	head.oid = SNMP_CM_REBOOT;
	head.value = "1";

	iStatus = SNMP_GetCMStatus(&head,REBOOT_CM,NULL);
	if(iStatus == 0)
		snmp_printf("SNMP_GetCMStatus success!return value = [%d]\n",iStatus);
	else 
		snmp_printf("SNMP_GetCMStatus ERROR!return value = [%d]\n",iStatus);

	req_id++;
	if(req_id == 0xff)
	{
		req_id = 1;
	}
	if(iStatus == 0)
	{
		return CHMID_CM_OK;
	}
	else
	{
		return CHMID_CM_OK;/*重起时网络终端导致错误*/
	}
}
/*
返回0表示获取正确
*/
CHMID_CM_RESULT_e CHMID_CM_SetDownFreq(u8_t * Freq)
{
	int iStatus = 0;
	snmp_header_t head;
	static unsigned char req_id = 1;
	CHMID_CM_ONLINE_STATUS_e uCmStatus = CHMID_CM_OFFLINE;
	
	if(Freq == NULL)
		return CHMID_CM_INVALID_PARAM;
	head.version = SNMP_VER - 1;
	head.community_type = SNMP_COMMUNITY;
	head.pdu_type = SNMP_SET_REQ;
	head.req_id = req_id;
	head.err_index = 0;
	head.err_status = 0;
	head.oid = SNMP_SET_DOWN_FREQ;
	head.value = Freq;

	iStatus = SNMP_GetCMStatus(&head,SET_DOWN_FRE,NULL);
	if(iStatus == 0)
		snmp_printf("SNMP_GetCMStatus success!return value = [%d]\n",iStatus);
	else 
		snmp_printf("SNMP_GetCMStatus ERROR!return value = [%d]\n",iStatus);

	req_id++;
	if(req_id == 0xff)
	{
		req_id = 1;
	}
	if(iStatus == 0)
	{
		 CHMID_CM_Reboot();
		return CHMID_CM_OK;
	}
	else
	{
		return CHMID_CM_FAIL;
	}
}
CHMID_CM_ONLINE_STATUS_e CHMID_CM_CheckCMStatus(void)
{
	return gCM_Status;
}

CHMID_CM_RESULT_e CHMID_CM_SetCheckCycleTime(u32_t ui_time_s)
{
	if(ui_time_s == 0)
		return CHMID_CM_INVALID_PARAM;
	
	gCM_CheckCycleTime = ui_time_s * 1000;
	return CHMID_CM_OK;
}

u32_t CHMID_CM_GetCheckCycleTime(void)
{
	
	return gCM_CheckCycleTime/1000;
}

CHMID_CM_RESULT_e CHMID_CM_GetVersion(CHMID_CM_Version_t * pVersion)
{
	pVersion->i_InterfaceMainVer = 0;
	pVersion->i_InterfaceSubVer = 2;
	pVersion->i_ModuleMainVer = 0;
	pVersion->i_ModuleSubVer = 2;
	return CHMID_CM_OK;
}
CHMID_CM_RESULT_e CHMID_CM_Open(void)
{
	gCM_Opened = 1;
	return CHMID_CM_OK;
}
CHMID_CM_RESULT_e CHMID_CM_Close(void)
{
	gCM_Opened = 0;
	return CHMID_CM_OK;
}
CHMID_CM_RESULT_e CHMID_CM_Init(void)
{
	s32_t stackSize = 8192;
	s32_t priority = 6;
	s8_t * name = "CM_process";
	CHMID_CM_Version_t cmVersion ;
	CHDRV_NET_THREAD taskHandle = NULL;
	

	/**/taskHandle = CHDRV_NET_ThreadCreate((void(*)(void *))Snmp_CM_Process, \
			NULL,		\
			stackSize,	\
			priority, 		\
			(s8_t *)name,		\
			0);
	if(taskHandle == NULL)
	{
		snmp_printf("\r\nCHMID_CM INIT FAIL\r\n");
		return CHMID_CM_FAIL;
	}
	CHMID_TCPIP_GetNetConfig(&netCfgTemp);
	
	CHMID_CM_GetVersion(&cmVersion);
	snmp_printf("*******************************************\r\n");
	snmp_printf("      CHMID_CM INIT OK!\r\n");
	snmp_printf("      (v%d.%d.%d.%d)\r\n",cmVersion.i_InterfaceMainVer,	\
		cmVersion.i_InterfaceSubVer,cmVersion.i_ModuleMainVer,cmVersion.i_ModuleSubVer);
	snmp_printf("      Built time: %s,%s\r\n",__DATE__,__TIME__);
	snmp_printf("*******************************************\r\n");

	return CHMID_CM_OK;
}

/*----------------------------------EOF--------------------------------------*/
