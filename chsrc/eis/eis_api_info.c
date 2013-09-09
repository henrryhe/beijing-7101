/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_info.c
  * 描述: 	实现info相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-22
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "..\dbase\vdbase.h"
#include "eis_include\ipanel_base.h"
#include "eis_include\ipanel_sysapp.h"
#include "eis_api_info.h"
#include "..\util\ch_time.h" 
#include "chmid_cm_api.h"
#ifdef __EIS_API_DEBUG_ENABLE__
#define __EIS_API_INFO_DEBUG__
#endif
extern char                   str[60];
 extern BOX_INFO_STRUCT	*pstBoxInfo;    /* Box information from e2prom */
SHORT CH_GetLoaderVersion(void);
void EIS_LoadMacAddr(U8 *mac)
{
	U8 STB_Id[14];
	U8 STB_SN[14];
	int itemp;
	
	if(NULL == mac)
	{
		return;
	}
	
	CH_ReadSTBAndMACID((U8 *)STB_Id);
	 for (itemp=0;itemp<14;itemp++)
	{
		STB_SN[itemp] = STB_Id[13-itemp];
	}
	 
	mac[0]=STB_SN[8];
	mac[1]=STB_SN[9];
	mac[2]=STB_SN[10];
	mac[3]=STB_SN[11];
	mac[4]=STB_SN[12];
	mac[5]=STB_SN[13];
	
	 for (itemp=0;itemp<6;itemp++)
	{
		if(mac[itemp]!=0xff)
		{
			break;
		}
	}
	 
	 if(itemp==6)
 	{
		mac[0] = 0x00;
		mac[1] = 0x11;
		mac[2] = 0x22;
		mac[3] = 0x33;
		mac[4] = 0xff;
		mac[5] = 0x16;
 	}

}
/*
功能说明：
	获取系统的相关信息，以字符串的方式返回，一般是要在页面上作显示用的。Name
	参数为一枚举值（见下表），该值可能会增加，集成时或更新库文件时请以头文件为准。
参数说明：
	输入参数：
		name：希望获取的信息类型（见上表中的定义，使用时以头文件为准）
		buf：用来保存获取信息的buffer 的地址
		len：用来保存获取信息的buffer 的长度
	输出参数：无
	返 回：
		> 0:获取信息的实际长度;
		IPANEL_ERR:没有获取到指定信息
  */

INT32_T ipanel_porting_system_get_info ( IPANEL_STB_INFO_e name, CHAR_T *buf, INT32_T len)
{
	char cBuff[100];
	U8 soft1,soft11;
	U8 *soft2=NULL;
	 U8 STB_Id[14];
	U8 STB_SN[14],Mac[6];
	U8                      ProductModel;
	int                      ProYear;
	U8                      ProMonth;
	U8                      ProDay;
	U8                      ManuID;
	long int               STB_SNnum;
	U32    ret  = 0;
	eis_report ( "\n++>eis ipanel_porting_system_get_info <%d> time=%d ", name ,get_time_ms());
	if(NULL==buf)
	return IPANEL_ERR; 
	
	switch ( name )
	{
		case IPANEL_GET_TIME_ZONE:
			{
				buf[0]='8';
				len=1;
				return 1;
			}
			break;
		case IPANEL_GET_UTC_TIME:
			{
				static TDTTIME 	cur_time;
				static TDTDATE cur_date;	
				unsigned long utc_sec;
				char  time_str[30];
				int i,j;
				struct tm temptm;
				{
					GetCurrentDate(&cur_date);
					GetCurrentTime(&cur_time);
					if(cur_date.sYear<2000)
						return IPANEL_ERR;
					
					temptm.tm_sec = cur_time.ucSeconds;
					temptm.tm_min = cur_time.ucMinutes;
					temptm.tm_hour = cur_time.ucHours;
					temptm.tm_mday = cur_date.ucDay;
					temptm.tm_mon = (cur_date.ucMonth-1);
					temptm.tm_year = cur_date.sYear-1900;
					temptm.tm_wday = 0;
					temptm.tm_yday = 0;
					temptm.tm_isdst = 0;
					eis_report("\ndata: %d:%d:%d ,time: %d:%d:%d",
						cur_date.sYear,cur_date.ucMonth,cur_date.ucDay,
						temptm.tm_hour,temptm.tm_min,temptm.tm_sec);
					utc_sec= ( unsigned long )mktime(&temptm)-8*3600;
					memset(time_str,0,30);
					for(i=0; i<30; i++)
					{
						if(utc_sec>=10)
						{
							time_str[i]=utc_sec%10+'0';
							utc_sec=utc_sec/10;
						}
						else
						{
							time_str[i]=utc_sec%10+'0';
							break;
						}
					}
					//eis_report("\n time_str=%s ",time_str);
					for(j=0;i>=0;i--)
					{
						buf[j]=time_str[i];
						j++;
					}
					eis_report("\n buf=%s ",buf);
					return (j+1);
				}
			}
			break;
		case IPANEL_OC_SERVICE_ID: /*开始OC 打开时对应的service_id,DVB2.0启动流程之所必须*/
			
			break;
		case IPANEL_BOOT_TYPE: 					/*系统启动方式：取值为warm、cold */
			{
				strcpy ( buf, "cold");
				len=strlen("cold");
				eis_report ( "\n  IPANEL_BOOT_TYPE <%s>,len=%d ",buf, len);
			
				return len;
			}
			break;
		case IPANEL_BOOT_STRING:	 			/*启动参数串*/
			eis_report ( "\n++>eis ipanel_porting_system_get_info <IPANEL_BOOT_STRING>" );
			sprintf ( buf, "BOOTURL=http://10.3.46.77" );
			len=strlen(buf);
			return len;
			break;
			
		case IPANEL_STANDBY_FLAG: /* 系统启动时，获取是否需要进入待机的待机标志位，1表示启动后进待机状态，0表示启动后不进待机状态。*/
			{
				U8 STBSoftVersionMajor, STBSoftVersionMinor;
				GetSTBVersion(&STBSoftVersionMajor,&STBSoftVersionMinor);
				if((pstBoxInfo->abiBoxPoweredState==1)&&(pstBoxInfo->upgrade==1))
				{
					buf[0]='1';
					len=1;
					eis_report("\n pstBoxInfo->abiBoxPoweredState=%d, 开机要进入待机",pstBoxInfo->abiBoxPoweredState);
				}
				else
				{
					buf[0]='0';
					len=1;
					//eis_report("\n 开机不待机");
					pstBoxInfo->abiBoxPoweredState=0;
				}
				//pstBoxInfo->upgrade=0; move 195 line 
				CH_NVMUpdate(idNvmBoxInfoId); 
				return IPANEL_OK; 
			}
			break;
		case IPANEL_SEARCH_FLAG	: /* 系统启动时，获取是否需要搜台的标志位，1表示搜索，0表示不搜索。*/
			{
				U8 STBSoftVersionMajor, STBSoftVersionMinor;
				GetSTBVersion(&STBSoftVersionMajor,&STBSoftVersionMinor);

				if((pstBoxInfo->serach_tag==1)&&(pstBoxInfo->ucSoftwareVerMajor==STBSoftVersionMajor)&&(pstBoxInfo->ucSoftwareVerMinor==STBSoftVersionMinor))
				{
					buf[0]='1';
					len=1;
				}
				else
				{
					buf[0]='0';
					len=1;
				}
				pstBoxInfo->serach_tag=0;
				CH_NVMUpdate(idNvmBoxInfoId); 
				return IPANEL_OK; 
			}
			break;
		case IPANEL_HARDWARE_VERSION:   //  获取机顶盒硬件版本号：  格式为：HH-XX-XX
			{
				short HardwareID;
				//char Hardware_str[10]={"HH-01-"};
				HardwareID=CH_GetHardwareID();
				do_report(0,"\nHardwareID====%d",HardwareID);
				sprintf(buf,"CH-01-%02d",HardwareID);
				len=strlen(buf);
				//eis_report ( "\n++>eis ipanel_porting_system_get_info end  time=%d ",ipanel_porting_time_ms());

				return len;
			}
			break;
		case IPANEL_SOFTWARE_VERSION:

			{
			  GetSTBVersion(&soft1,&soft11);
			  sprintf(buf,"CS-%02d-%02d",soft1,soft11);
				len=strlen(buf);
				return len;
			}
			  break;
		case IPANEL_PRODUCT_SERIAL:  			/*产品序列号*/
			 {		 
			 	int itemp;
			 	CH_ReadSTBAndMACID((U8 *)STB_Id);

				  for (itemp=0;itemp<14;itemp++)
				{
					STB_SN[itemp] = STB_Id[13-itemp];
				}
				STB_SNnum=STB_SN[5]|(STB_SN[4]<<8)|(STB_SN[3]&0xF)<<16;
				ManuID=STB_SN[7]=0x01;
				ProductModel=STB_SN[6];
				ProYear=((STB_SN[1]&0xf)<<3)|((STB_SN[2]&0xe0)>>5);
				ProYear=ProYear;
				ProMonth=((STB_SN[2]&0x1e)>>1)&0xF;
				ProDay=((STB_SN[2]&0x1)<<4)|((STB_SN[3]>>4)&0xf);
				sprintf(buf,"%02d%02d%02d%02d%02d%07d",ManuID,ProductModel,ProYear,ProMonth,ProDay,STB_SNnum);
				len=strlen(buf);
				return len;
			}
			break;
		case IPANEL_PRODUCT_MAC_ADDR:  		/*网卡MAC 地址*/
			{
				EIS_LoadMacAddr(Mac);
				do_report(0,"%02x-%02x-%02x-%02x-%02x-%02x",Mac[0],Mac[1],Mac[2],Mac[3],Mac[4],Mac[5]);
				sprintf(buf,"%02x-%02x-%02x-%02x-%02x-%02x",Mac[0],Mac[1],Mac[2],Mac[3],Mac[4],Mac[5]);
				len=strlen(buf);
				eis_report ( "\n++>eis IPANEL_PRODUCT_MAC_ADDR" );
				return len;
			}
			break;		
		case IPANEL_CM_STATUS:
			buf[0] = '1' ;
			buf[1] = 0 ;
			return 0;
			break;
		case IPANEL_LOADER_VERSION:
			{    
				short LoaderwareID;
			       LoaderwareID=CH_GetLoaderVersion();
			       sprintf(buf,"CL-%02d-%02d",LoaderwareID/256,LoaderwareID%256);
			       len=strlen(buf);
				  return len;
			}
			break;

		case IPANEL_UPGRADE_FLAG:
			{
				if(1 == pstBoxInfo->upgrade)
				{
					buf[0]='1';
					len=1;
					eis_report("\n pstBoxInfo->upgrade = %d,升级成功,开机进入频道号最小\n", pstBoxInfo->upgrade);
				}
				else
				{
					buf[0]='0';
					len=1;
					eis_report("\n pstBoxInfo->upgrade = %d,未升级,开机进入原频道号\n", pstBoxInfo->upgrade);
				}
				pstBoxInfo->upgrade=0; 
				CH_NVMUpdate(idNvmBoxInfoId); 
				return IPANEL_OK; 
			}
			break;
	}

	return ret; 
}


/*
功能说明：
	如果将iPanel Middleware 的内部数据(字库，UI 之类)独立存储在 Flash 内存中,
	则必须实现该函数；内部数据由 iPanel 在发布iPanel Middleware 库文件的同时提
	供, 由 STB 集成商独立存放进 Flash 内存中，然后通过正确实现该函数，告诉
	iPanel Middleware 相关数据的存储信息。
 参数说明:
	输出参数：
		address：返回iPanel Middleware 请求数据的存储起始地址；
		size：返回iPanel Middleware 请求数据的长度。
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */

extern   ST_Partition_t* CHSysPartition ;
INT32_T ipanel_porting_get_outside_dat_info(CHAR_T **address, INT32_T *size, IPANEL_RESOURCE_TYPE_e type )
{
#if 1
	static CHAR_T * UI_buff=NULL;
	if(UI_buff==NULL)
	{
		UI_buff=(CHAR_T *)memory_allocate ( CHSysPartition,UI_SIZE);
		if(UI_buff==NULL)
			return IPANEL_ERR ; 
		memcpy((U8 *)UI_buff,(U8 *)UI_ADDRESS,UI_SIZE);
	}

	switch(type)
	{
		case IPANEL_RESOUCE_UI:
			{
				*address = (CHAR_T *)UI_buff;
				*size = UI_SIZE;
				do_report(0,"\n ipanel_porting_get_outside_dat_info out ui addr 0x%x , size %d ",*address,*size);
			}
			break;
		default:
			return IPANEL_ERR ; 
	}
#endif
	return IPANEL_OK ; 
}

#if SUMA_SECURITY
/* 把szStr前面(左边)的 szSet 字符串去调,并把字符串前移到
  * 头地址开始的地方*/
int CH_LeftStrTrim(char* p,int n)
{
	
	int len = 0;
	int i;
	int lenn;

	len = strlen(p);
	lenn = len - n;

	if( n < len )
	{
		for(i = 0;i <lenn;i++)
		{
			p[i] = p[ n + i];
		}
		for(i = lenn;i <len;i++)
		{
			p[i] = '\0';
		}

	}
	

}

int CH_RightStrTrim(char* p,int n)
{
	
	int len = 0;
	int i;

	len = strlen(p);

	if( n < len )
	{
		for(i = (len -n);i < len;i++)
		{
			p[i] = '\0';
		}

	}
	

}
void TRIM_LEFT(char* str)
{	
	while (0 < strlen(str) &&(str[0] == '\r' ||str[0] == '\n' ||str[0] == '\t' ||str[0] == ' '))
	{
		CH_LeftStrTrim(str,1);
	}
}


void TRIM_RIGHT(char* str)
{
	int len = strlen(str);

	while (0 < strlen(str) &&\
		   (str[len - 1] == '\r' ||\
		    str[len - 1] == '\n' ||\
		    str[len - 1] == '\t' ||\
		    str[len - 1] == ' '))\
	{
		CH_RightStrTrim(str,1);
	}
}

BOOL ChangeCharToValue(char* c_string,int* rpi_val)
{
	int i_len = strlen(c_string);
	int i_val = 0;
	int i_tempval;
	int iLoop;

	
#if 0
	for(iLoop = 0;iLoop<i_len;iLoop++)
	{
		if(c_string[iLoop] > '0' && c_string[iLoop]< '9')
{
			i_tempval = c_string[iLoop] - 48;
			i_val = i_val*10 + i_tempval;
		}
		else
		{
			return false;
		}
	}
#else
	i_val = atoi(c_string);
	#endif
	*rpi_val = i_val;
	return true;



}
#if 1 	/*sqzow20100624*/
INT32_T ipanel_start_other_app(const CHAR_T *name)
{
	char* startpos = NULL;
	char* endpos = NULL;

	char* val_startpos = NULL;
	char* val_endpos = NULL;

	char* c_tempstring = NULL;
	int     i_templen;
	int 	i_ParseResultNum = 0;/*解析成功的个数 sqzow20100626*/
	int     i_fre,i_sym,i_qam,i_audiopid;
	
	do_report(0,"ipanel_start_other_app %s",name);
	
	if(strstr(name,"randomnumber") == NULL ||strstr(name,"end") == NULL)
	{
		return IPANEL_ERR;
	}

	startpos = strstr(name,":");
	

	while(startpos != NULL)
	{
		endpos = strstr(startpos,";");
		if(endpos != NULL)
		{
		/*****************FRE********************************/
			val_startpos = strstr(startpos,"frequency");
			val_endpos = strstr(startpos,",");
			if(val_startpos == NULL ||val_endpos == NULL)
			{
				return IPANEL_ERR;
			}
			val_startpos = strstr(val_startpos,"=");
			if(val_startpos != NULL)
			{
				i_templen = val_endpos - val_startpos;
				if(i_templen == 1)/*只有预留的\0,返回错误*/
				{
					return IPANEL_ERR;
				}
				
				c_tempstring = malloc(i_templen);
				if(c_tempstring != NULL)
				{
					memset(c_tempstring,'\0',i_templen);
					memcpy(c_tempstring,(val_startpos+1),i_templen-1);
					TRIM_LEFT(c_tempstring);
					TRIM_RIGHT(c_tempstring);
					if(ChangeCharToValue(c_tempstring,&i_fre) == false)
					{
						do_report(0,"频率字符错误");
					}
					free(c_tempstring);
				}
				else
				{
					do_report(0,"malloc failed");
					return IPANEL_ERR;
				}
			}
			else
			{
				return IPANEL_ERR;
			}

			/****************SYM*************************/
			startpos = val_endpos+1;

			val_startpos = strstr(startpos,"symbolrate");
			val_endpos = strstr(startpos,",");
			if(val_startpos == NULL ||val_endpos == NULL)
			{
				return IPANEL_ERR;
			}
			val_startpos = strstr(val_startpos,"=");

			if(val_startpos != NULL)
			{
				i_templen = val_endpos - val_startpos;
				if(i_templen == 1)/*只有预留的\0,返回错误*/
				{
					return IPANEL_ERR;
				}
				
				c_tempstring = malloc(i_templen);
				if(c_tempstring != NULL)
				{
					memset(c_tempstring,'\0',i_templen);
					memcpy(c_tempstring,(val_startpos+1),i_templen-1);
					TRIM_LEFT(c_tempstring);
					TRIM_RIGHT(c_tempstring);
					if(ChangeCharToValue(c_tempstring,&i_sym) == false)
					{
						do_report(0,"符号率字符错误");
					}
					free(c_tempstring);
				}
				else
				{
					do_report(0,"malloc failed");
					return IPANEL_ERR;
				}

				
			}

			/****************QAM*************************/
			startpos = val_endpos+1;

			val_startpos = strstr(startpos,"modulation");
			val_endpos = strstr(startpos,",");
			if(val_startpos == NULL ||val_endpos == NULL)
			{
				return IPANEL_ERR;
			}
			val_startpos = strstr(val_startpos,"=");

			if(val_startpos != NULL)
			{
				i_templen = val_endpos - val_startpos;
				if(i_templen == 1)/*只有预留的\0,返回错误*/
				{
					return IPANEL_ERR;
				}

				c_tempstring = malloc(i_templen);
				if(c_tempstring != NULL)
				{
					memset(c_tempstring,'\0',i_templen);
					memcpy(c_tempstring,(val_startpos+1),i_templen-1);
					TRIM_LEFT(c_tempstring);
					TRIM_RIGHT(c_tempstring);
					if(ChangeCharToValue(c_tempstring,&i_qam) == false)
					{
						do_report(0,"QAM字符错误");
					}
					free(c_tempstring);
				}
				else
				{
					do_report(0,"malloc failed");
					return IPANEL_ERR;
				}

				
			}
			/******************audiopid**********************/
			startpos = val_endpos+1;

			val_startpos = strstr(startpos,"audiopid");
			val_endpos = strstr(startpos,";");
			if(val_startpos == NULL ||val_endpos == NULL)
			{
				return IPANEL_ERR;
			}
			val_startpos = strstr(val_startpos,"=");

			if(val_startpos != NULL)
			{
				i_templen = val_endpos - val_startpos;
				if(i_templen == 1)/*只有预留的\0,返回错误*/
				{
					return IPANEL_ERR;
				}
				
				c_tempstring = malloc(i_templen);
				if(c_tempstring != NULL)
				{
					memset(c_tempstring,'\0',i_templen);
					memcpy(c_tempstring,(val_startpos+1),i_templen-1);
					TRIM_LEFT(c_tempstring);
					TRIM_RIGHT(c_tempstring);
					if(ChangeCharToValue(c_tempstring,&i_audiopid) == false)
					{
						do_report(0,"QAM字符错误");
					}
					free(c_tempstring);
				}
				else
				{
					do_report(0,"malloc failed");
					return IPANEL_ERR;
				}
			}
			/*添加TS包获取记录*/
			if(i_ParseResultNum == 0)
			{
				CH_ResetTsParameter();
			}
			CH_AddTsCapturePara(i_fre, i_sym,i_qam, i_audiopid);
			i_ParseResultNum++;
			
		}
		else
		{
			return IPANEL_ERR;
		}
		
		startpos = strstr(val_endpos,"frequency");
		
	}

	CH_SaveTsCapturePara();
	return IPANEL_OK;
}
#endif
#endif


/*--eof----------------------------------------------------------------------------------------------------------*/

