/********************************源文件头注释：开始******************/ 
/*版权所有: 四川长虹网络科技有限公司(2008) */
/*文件名:      ipanel_socket.c */
/*版本号:  */
/*作者: liuzhanfen */
/*创建日期: 2008.12.01 */
/*内容概述: 浏览器SOCKET相关模块定义*/
/*修改历史:  */
/**************************源文件头注释：结束************************/ 
#include "..\main\initterm.h"
#include "stddefs.h"
#include "stdarg.h"              	/* for argv argc  */
#define CH_Printf(x) sttbx_Print x
#include "eis_include\ipanel_socket.h"
#include "eis_include\ipanel_network.h"

#include "eis_include\ipanel_porting_event.h"
#define u32 U32
#if 0
#define ipanel_debug_socket/**/
#endif
//#include "..\chmid_dvm\ch_pvrgui.h"
#include "eis_api_globe.h"
#include "chmid_tcpip_api.h"
#include "chmid_cm_api.h"
#include "sockets.h"
typedef  CHMID_TCPIP_SockAddr_t sockaddr_in_t;
extern boolean EIS_DHCP_set;

extern void task_time_sleep(unsigned int ms);

typedef int STATUS ;

#define MAX_SOCKETS (16)

union   ORDERBYTE
{
	unsigned int 	dwords;
	struct
	{
		unsigned char	byte0;
		unsigned char	byte1;
		unsigned char	byte2;
		unsigned char	byte3;
	}bytes;
};

static unsigned int  CHARTOINT(unsigned char *ipAddr)
{
	union  ORDERBYTE  IPAddr ;

	IPAddr.bytes.byte0  = ipAddr[3];
	IPAddr.bytes.byte1  = ipAddr[2];
	IPAddr.bytes.byte2  = ipAddr[1] ;
	IPAddr.bytes.byte3  = ipAddr[0] ;

	return (IPAddr.dwords) ;
}

typedef struct _SOCKPARAM_tag
{
	INT32_T sockid ; 
	INT32_T socktype ; 
	INT32_T sockport ; 
	IPANEL_IPADDR   sockip ;	
}SOCKPARAM;

typedef struct _SOCKMANAGER_tag
{
	INT32_T n_sockidle ; 
	SOCKPARAM sockparam[MAX_SOCKETS] ;	
}SOCKMANAGER ;

static SOCKMANAGER *ipanel_mgr = NULL ; 
static SOCKMANAGER  ipanelMgr ;

static SOCKMANAGER  *CH_ipanel_register_new()
{
	SOCKMANAGER *me ;  
	int i;
	
	me = (SOCKMANAGER*) &ipanelMgr;
	memset(me,0,sizeof(SOCKMANAGER));
	
	for (i=0;i<MAX_SOCKETS;i++) 
	{
		me->sockparam[i].sockid = -1;
		me->sockparam[i].socktype = -1;
		me->sockparam[i].sockport = -1; 
	}
	
	me->n_sockidle = MAX_SOCKETS; 
 
	return me;
}
#define CH_Print
#ifdef CH_Print
static char ipanel_changhong_buff[1024];
#endif

static U8 socket_num=0;
int CH_GetEisSocketNum()
{
	return socket_num;
}

CH_ipanel_SocketSumAdd()
{
	if(socket_num<254)
	socket_num++;
}

CH_ipanel_SocketReduce()
{
	if(socket_num>0)
	socket_num--;
}

void CH_ipanel_Print( const char *fmt, ... )
{
	#ifdef 	CH_Print
	va_list Argument;
	static semaphore_t *p_ipanel_Print_seg=NULL; /*对打印接口增加保护防止函数访问重入*/
	if(p_ipanel_Print_seg==NULL)
	{
		p_ipanel_Print_seg = semaphore_create_fifo(1);
		if(p_ipanel_Print_seg==NULL)
			return ;
	}
	semaphore_wait(p_ipanel_Print_seg);
	va_start( Argument, fmt );
	vsprintf( ipanel_changhong_buff, fmt, Argument );
	va_end( Argument );

	CH_Printf((" %s" ,ipanel_changhong_buff ));
	semaphore_signal ( p_ipanel_Print_seg );
      #endif	
}

static void CH_ipanel_register_delete(SOCKMANAGER *me)
{
	int i;
    	for (i=0;i<MAX_SOCKETS;i++) 
	{
        	if (me->sockparam[i].sockid != -1) 
		{
			CHMID_TCPIP_Close(me->sockparam[i].sockid);
        	}
    	}
}

static int CH_ipanel_register_register(SOCKMANAGER *me, INT32_T s)
{
	int i, indx=-1;

	for (i=0;i<MAX_SOCKETS;i++) 
	{
		if (me->sockparam[i].sockid == -1) 
		{
			me->sockparam[i].sockid = s;
			me->n_sockidle --;
			indx = i ;
			break;
		}
	}

	return indx;	
}

static int CH_ipanel_register_unregister(SOCKMANAGER *me, INT32_T s)
{
	int i, indx=-1;
	
	for (i=0;i<MAX_SOCKETS;i++) 
	{
		if (me->sockparam[i].sockid == s)
		{
			me->sockparam[i].sockid = -1;
			me->n_sockidle ++;
			indx = i;
			break;
		}
	}
	
	return indx;	
}

static int CH_ipanel_register_set_param(SOCKMANAGER *me,SOCKPARAM param)
{
	int i, indx=-1;
	
	for (i=0;i<MAX_SOCKETS;i++) 
	{
		if (me->sockparam[i].sockid == param.sockid)
		{
			indx = i;
			break;
		}
	}

	if(indx == MAX_SOCKETS)
	{
		eis_report("[CH_ipanel_register_set_param] set param failed!\n");
		return -1;
	}

	me->sockparam[indx].sockport = param.sockport ;
	me->sockparam[indx].socktype = param.socktype ;
	me->sockparam[indx].sockip.version    = param.sockip.version ; 
	me->sockparam[indx].sockip.padding1     = param.sockip.padding1 ; 
	me->sockparam[indx].sockip.padding2     = param.sockip.padding2 ; 
	me->sockparam[indx].sockip.padding3     = param.sockip.padding3 ; 
	me->sockparam[indx].sockip.addr.ipv4     = param.sockip.addr.ipv4 ; 
	me->sockparam[indx].sockip.addr.ipv6     = param.sockip.addr.ipv6 ; 

	return 0;	
}

static int CH_ipanel_register_get_param(SOCKMANAGER *me, SOCKPARAM *param)
{
	int i, indx=-1;
	
	for (i=0;i<MAX_SOCKETS;i++) 
	{
		if (me->sockparam[i].sockid == param->sockid)
		{
			indx = i;
			break;
		}
	}

	if(indx == MAX_SOCKETS)
	{
		eis_report("[CH_ipanel_register_get_param] get param failed!\n");
		return -1;
	}

	param->sockport = me->sockparam[indx].sockport;
	param->socktype = me->sockparam[indx].socktype;
	param->sockip.version      =  me->sockparam[indx].sockip.version    ; 
	param->sockip .padding1     =  me->sockparam[indx].sockip.padding1    ; 
	param->sockip.padding2       =  me->sockparam[indx].sockip.padding2    ; 
	param->sockip.padding3       =  me->sockparam[indx].sockip.padding3     ; 
	param->sockip.addr.ipv4     =  me->sockparam[indx].sockip.addr.ipv4    ; 
	param->sockip.addr.ipv6     =  me->sockparam[indx].sockip.addr.ipv6    ; 
	
	return 0;
}

INT32_T CH_ipanel_get_socktype(SOCKMANAGER *me, INT32_T s)
{
	int i, indx=-1;
	
	for (i=0;i<MAX_SOCKETS;i++) 
	{
		if (me->sockparam[i].sockid == s)
		{
			indx = i;
			break;
		}
	}

	if(indx == MAX_SOCKETS)
	{
		eis_report("[CH_ipanel_get_socktype] get sock type failed!\n");
		return -1;
	}	

	return me->sockparam[indx].socktype ; 
}

INT32_T CH_ipanel_network_init(void)
{
	#ifdef ipanel_debug_socket
	eis_report( "\n CH_ipanel_network_init");
	#endif
	
	ipanel_mgr = CH_ipanel_register_new();
	if( NULL == ipanel_mgr )
	{
		eis_report("\n CH_ipanel_network_initnetwork init failed! ");
		return false ; 
	}

	return true;
}

VOID CH_ipanel_network_exit(VOID)
{
	#ifdef ipanel_debug_socket
	eis_report( "\n CH_ipanel_network_exit");
	#endif

	CH_ipanel_register_delete(ipanel_mgr);
	ipanel_mgr = NULL;
}
extern void* 	pEisHandle ;	/* ipanel浏览器句柄 */
INT32_T ipanel_porting_socket_open ( INT32_T domain, INT32_T type, INT32_T protocol )
{
	INT32_T ret =  IPANEL_ERR;
	STATUS socketd = -1; 
	INT32_T sockdomain, socktype ;
	SOCKPARAM tmpSockParam = {0} ;
	int argp=1;
	
	static UINT32_T addr = 0;
	if(addr==0 )
		CHMID_TCPIP_GetIP(&addr);
	if(addr==0)
		return ret ; 
	
	#ifdef ipanel_debug_socket
	eis_report( "\n pEisHandle=%x, time=%d  ipanel_porting_socket_open domain=%d, type=%d, protocol=%d\n",pEisHandle,get_time_ms(),domain, type, protocol );
	#endif

	if (  (IPANEL_AF_INET == domain) ||
		(IPANEL_AF_INET6 == domain))
	{
	    	sockdomain = AF_INET;
	}
	else
	{
		eis_report("\n ipanel_porting_socket_open ERROR: domain para error ! ");
	    	return ret;
	}
	//eis_report( "\n 2222222 pEisHandle=%x",pEisHandle);
	if (IPANEL_SOCK_STREAM == type)
	{
	    	socktype = SOCK_STREAM;
	}
	else if (IPANEL_SOCK_DGRAM == type)
	{
	    	socktype = SOCK_DGRAM;
	}
	else
	{
		eis_report("\n ipanel_porting_socket_open ERROR: stream(TCP&UDP) para error !");
	    	return ret;
	}
	//eis_report( "\n 2222222 pEisHandle=%x",pEisHandle);

	if (IPANEL_IPPROTO_IP == protocol)
	{
		socketd = CHMID_TCPIP_Socket(sockdomain, socktype, protocol);	
		if ( (socketd < 0)) 
		{
			eis_report("\n ipanel_porting_socket_open ERROR: create socket error !");
			return ret ; 
		}
		else
		if((socketd>=MAX_SOCKETS))
		{
			CHMID_TCPIP_Close (socketd );
			eis_report("\n ipanel_porting_socket_open more!");
			return ret ; 
		}
	}
	//eis_report( "\n 2222222 pEisHandle=%x",pEisHandle);

	CH_ipanel_register_register(ipanel_mgr, socketd);
		//eis_report( "\n 2222222 pEisHandle=%x",pEisHandle);

	tmpSockParam.sockid = socketd ;
	tmpSockParam.socktype  = type ;

		//eis_report( "\n 2222222 pEisHandle=%x",pEisHandle);

	CH_ipanel_register_set_param(ipanel_mgr, tmpSockParam);
	CHMID_TCPIP_Ioctl(socketd,CHMID_TCPIP_O_NONBLOCK,(void *)&argp);
	#ifdef ipanel_debug_socket
	eis_report("\n 2222222 pEisHandle=%x, time=%d, socket id = %d",pEisHandle,get_time_ms(), socketd);
	#endif
	CH_ipanel_SocketSumAdd();
       return socketd;
}

INT32_T ipanel_porting_socket_bind ( INT32_T sockfd, IPANEL_IPADDR * ipaddr,INT32_T port )
{
        INT32_T ret = IPANEL_ERR;
        STATUS bindstat ; 
        sockaddr_in_t serv_addr;
		
	#ifdef ipanel_debug_socket
	eis_report("\n time=%d, ipanel_porting_socket_bind  socket = %d ,ipaddr=0x%x,port=%d ",get_time_ms(), sockfd,ipaddr,port);
	#endif

        if ( ( sockfd >= 0 ) /*&& ( NULL != ipaddr ) */)
        {
        	if(NULL == ipaddr)
    		{
		 	unsigned int ip = 0;	
			serv_addr.us_Family= AF_INET;
			serv_addr.us_Port = CHMID_TCPIP_HTONS(port) ;  
			serv_addr.ui_IP= CHMID_TCPIP_NTOHL(ip);

	          	bindstat = CHMID_TCPIP_Bind( sockfd,  &serv_addr , sizeof ( serv_addr ) );
			if( bindstat < 0)
			{
				eis_report("\n ipanel_porting_socket_bind failed! socket = %d ,ipaddr=0x%x,port=%d ", sockfd,ipaddr,port);
				return ret ;
			}

			ret = IPANEL_OK ;
   			
    		}
		else
		{
	        	if (IPANEL_IP_VERSION_4 == ipaddr->version)
	        	{
		 		unsigned int ip = ipaddr->addr.ipv4;	
				serv_addr.us_Family= AF_INET;
				serv_addr.us_Port = CHMID_TCPIP_HTONS(port) ;  
				serv_addr.ui_IP= CHMID_TCPIP_NTOHL(ip);

		          	bindstat = CHMID_TCPIP_Bind( sockfd,  &serv_addr , sizeof ( serv_addr ) );
				if( bindstat < 0)
				{
					eis_report("\n ipanel_porting_socket_bind failed! socket = %d ,ipaddr=0x%x,port=%d ", sockfd,ipaddr,port);
					return ret ;
				}

				ret = IPANEL_OK ;
	        	}
			else if(IPANEL_IP_VERSION_6 == ipaddr->version)
			{
				/* IPv6协议 */
				eis_report("\n ipanel_porting_socket_bind ERROR: NOT suppurt IPV6 !!! ");
				return ret ; 
			}
			else
			{
				eis_report("\n ipanel_porting_socket_bind  ERROR: please set  IP_VERSION_4 !");
				return ret ;
			}
		}
        }
	#ifdef ipanel_debug_socket
	eis_report("\n bind end  ");
	#endif

        return ret;
}

INT32_T ipanel_porting_socket_listen ( INT32_T sockfd, INT32_T backlog )
{
        INT32_T ret = IPANEL_OK;
        STATUS lisstat ; 

	#ifdef ipanel_debug_socket
 	eis_report("\n ipanel_porting_socket_listen  socket = %d ,ipaddr=0x%x,backlog=%d ", sockfd,backlog);
	#endif

        if ( sockfd >= 0 )
        {
		  lisstat = CHMID_TCPIP_Listen(sockfd, backlog);
		  if( lisstat < 0) 
		  {
		  	eis_report("\n ipanel_porting_socket_listen  listen socket failed! ");
		  	ret = IPANEL_ERR ; 
		  }
        }
	//eis_report("\n listen end pEisHandle=%x, time=%d, socket id = %d",pEisHandle,get_time_ms(), sockfd);

        return ret;
}

INT32_T ipanel_porting_socket_accept ( INT32_T sockfd,IPANEL_IPADDR * ipaddr,INT32_T *port )
{
        INT32_T ret = IPANEL_ERR;
        STATUS newsock ; 
        sockaddr_in_t serv_addr = {0};		
        socklen_t len = sizeof ( sockaddr_in_t);
		
  	#ifdef ipanel_debug_socket
	eis_report("\n ipanel_porting_socket_accept  sockfd=%d, ip=0x%d, port=%d ",sockfd, ipaddr, port);
	#endif

        if ( sockfd >= 0 )
        {
        	  if (IPANEL_IP_VERSION_4 == ipaddr->version)
        	  {
			  newsock = CHMID_TCPIP_Accept(sockfd, &serv_addr, &len);
			  if ( newsock >= 0 )
			  {
	                        if ( ipaddr != NULL )
	                        {
	                                ipaddr->addr.ipv4 = CHARTOINT((unsigned char *)CHMID_TCPIP_InetNtoa(serv_addr.ui_IP));
	                        }

	                        if ( port  != NULL)
	                        {
	                                *port = serv_addr.us_Port ;
	                        }

			      	     eis_report("\n ipanel_porting_socket_accept   ip = 0x%x , port = %d", 
					   	ipaddr->addr.ipv4 , port); 
			  }
			  else
			  {
			  	eis_report("\n ipanel_porting_socket_accept  accept socket failed!");
				return ret ; 
			  }
        	  }
		  else if (IPANEL_IP_VERSION_6 == ipaddr->version)
        	 {
            		/* IPv6协议 */
			eis_report("\n ipanel_porting_socket_accept  ERROR: NOT suppurt IPV6 !!!");
			return ret ;
        	 }
		else
		{
			eis_report("\n ipanel_porting_socket_accept   ERROR: please set  IP_VERSION_4 !");
			return ret ;
		}		  
        }
	//eis_report("\n accept end pEisHandle=%x, time=%d, socket id = %d",pEisHandle,get_time_ms(), sockfd);

        return newsock;
}

INT32_T ipanel_porting_socket_connect (INT32_T sockfd,IPANEL_IPADDR * ipaddr,INT32_T port )
{
        INT32_T ret = IPANEL_ERR;
	 STATUS connstat ; 
        sockaddr_in_t serv_addr;

   	#ifdef ipanel_debug_socket
	eis_report("\n time=%d,  ipanel_porting_socket_connect  sockfd=%d, ip=0x%x, port=%d ",get_time_ms(),sockfd, ipaddr, port);
	#endif

       if ( ( sockfd >= 0 ) && ( NULL != ipaddr ) )
        {
        	if (IPANEL_IP_VERSION_4 == ipaddr->version)
        	{
			 unsigned int ip = ipaddr->addr.ipv4;	
			 int len = sizeof(serv_addr);	
			 INT32_T socktype ; 

			 socktype = CH_ipanel_get_socktype(ipanel_mgr,sockfd);

			 if( IPANEL_SOCK_DGRAM ==  socktype) 
			 {
				SOCKPARAM tmpSockParam = {0} ;

                		tmpSockParam.sockid = sockfd ;
				tmpSockParam.sockport = port ; 
				tmpSockParam.sockip.version = (*ipaddr).version ;
				tmpSockParam.sockip.padding1= (*ipaddr ).padding1;
				tmpSockParam.sockip.padding2 = (*ipaddr).padding2 ;
				tmpSockParam.sockip.padding3 = (*ipaddr).padding3 ;
				tmpSockParam.sockip.addr.ipv4 = (*ipaddr).addr.ipv4 ;
				tmpSockParam.sockip.addr.ipv6 = (*ipaddr).addr.ipv6 ;
				
                		tmpSockParam.socktype = socktype ;
				CH_ipanel_register_set_param(ipanel_mgr, tmpSockParam);
				
				ret = ipanel_porting_socket_bind(sockfd,ipaddr,port);
				return ret ; 			
			 }
			 else if( IPANEL_SOCK_STREAM == socktype )
			 {
				serv_addr.us_Family= AF_INET;		
				serv_addr.us_Port= CHMID_TCPIP_HTONS(port);  
				serv_addr.ui_IP= CHMID_TCPIP_NTOHL(ip);

                		connstat = CHMID_TCPIP_Connect(sockfd,&serv_addr, len);

				if ( connstat < 0)
				{
					eis_report("\n ipanel_porting_socket_connect connect failed!  sockfd=%d, ip=0x%x, port=%d ,error=%d", sockfd, ipaddr, port,connstat);
					return ret;
				}
			   	#ifdef ipanel_debug_socket
				eis_report("\n pEisHandle=%x, time=%d,   connect  ok",pEisHandle,get_time_ms());
				#endif
				ret = IPANEL_OK ; 
			 }
        	}
        	else if (IPANEL_IP_VERSION_6 == ipaddr->version)
        	{
            		/* IPv6协议 */
			eis_report("\n ipanel_porting_socket_connect ERROR: NOT suppurt IPV6 !!!");
			return ret ;
        	}
		else
		{
			eis_report("\n ipanel_porting_socket_connect  ERROR: please set  IP_VERSION_4 !");
			return ret ;
		}		
        }
	 #ifdef ipanel_debug_socket
	eis_report("\n connect  end pEisHandle=%x, time=%d, socket id = %d",pEisHandle,get_time_ms(), sockfd);
	#endif
        return ret;
}

INT32_T ipanel_porting_socket_send ( INT32_T sockfd,CHAR_T *buf,INT32_T buflen,INT32_T flags )
{
        INT32_T ret = IPANEL_ERR;
        INT32_T sendbyte ; 
   	#ifdef ipanel_debug_socket
	eis_report("\n time=%d, ipanel_porting_socket_send  sockfd=%d ,len=%d",get_time_ms(),sockfd,buflen);
	#endif

        if ( ( sockfd >= 0 ) && ( NULL != buf ) )
        {
    		INT32_T socktype ; 
    		socktype = CH_ipanel_get_socktype(ipanel_mgr,sockfd);
    		if( IPANEL_SOCK_DGRAM ==  socktype) 
    		{
    			SOCKPARAM tmpSockParam = {0} ;
    			INT32_T port ; 
    			IPANEL_IPADDR  * ipAddr ; 
    			tmpSockParam.sockid  = sockfd ;
    			CH_ipanel_register_get_param(ipanel_mgr,&tmpSockParam);
    			port = tmpSockParam.sockport ;
    			ipAddr = &tmpSockParam.sockip ; 

    			ret = ipanel_porting_socket_sendto(sockfd,buf,buflen,0, ipAddr, port );
    			if( sendbyte < 0)
    			{
    				do_report(0,"\n ipanel_porting_socket_send send byte failed! ");
    				return ret; 
    			} 
    		}
    		else if( IPANEL_SOCK_STREAM == socktype)
    		{
     	        	  sendbyte = CHMID_TCPIP_Send(sockfd, (char *)buf, buflen, flags);
    			  if( sendbyte < 0)
    			  {
    				do_report(0,"\n ipanel_porting_socket_send send byte failed! ");
    				 return ret; 
    			  }
    		}
        }

   	#ifdef ipanel_debug_socket
	eis_report("\n send   end pEisHandle=%x, time=%d, socket id = %d",pEisHandle,get_time_ms(), sockfd);
	#endif

        return sendbyte;
}

INT32_T ipanel_porting_socket_sendto ( INT32_T sockfd,CHAR_T *buf,
                                       INT32_T buflen,INT32_T flags,IPANEL_IPADDR * ipaddr,INT32_T port )
{
        INT32_T ret = IPANEL_ERR;
        INT32_T sendtobyte ; 
        sockaddr_in_t serv_addr;

   	#ifdef ipanel_debug_socket
	eis_report("\n time=%d, ipanel_porting_socket_sendto  sockfd=%d ,len=%d,ip=0x%x,port=%d",get_time_ms(),sockfd,buflen,ipaddr->addr.ipv4,port);
	#endif

        if ( ( sockfd >= 0 ) && ( NULL != buf ) )
        {
               if ( NULL != ipaddr )
               {
                	if (IPANEL_IP_VERSION_4 == ipaddr->version)
                	{
				unsigned int ip = ipaddr->addr.ipv4;
				serv_addr.us_Family= AF_INET;
				serv_addr.us_Port= CHMID_TCPIP_HTONS(port );                 

				serv_addr.ui_IP= CHMID_TCPIP_NTOHL(ip);
				sendtobyte = CHMID_TCPIP_SendTo(sockfd, (void*)buf, buflen, 0, &serv_addr, sizeof(CHMID_TCPIP_SockAddr_t));			
				if( sendtobyte < 0) 
				{	
					do_report(0,"\n ipanel_porting_socket_sendto sendto byte failed! ");
					return ret ; 
				}
                	}
			else if (IPANEL_IP_VERSION_6 == ipaddr->version)
			{
	                   /* IPv6协议 */
				do_report(0,"\n ipanel_porting_socket_sendto ERROR: NOT suppurt IPV6 !!! \n");
				return ret ; 
	             }	
			else
			{
				do_report(0,"\n ipanel_porting_socket_sendto  ERROR: please set  IP_VERSION_4 !\n");
				return ret ;
			}				  
              }
        }

   	#ifdef ipanel_debug_socket
	//eis_report(" \n time=%d, send data byte = %d ",get_time_ms(),sendtobyte);
	eis_report("\n  <<<<<<<<<< send to  end pEisHandle=%x, time=%d, socket id = %d",pEisHandle,get_time_ms(), sockfd);
	#endif

        return sendtobyte;
}

INT32_T ipanel_porting_socket_recv ( INT32_T sockfd,CHAR_T *buf,INT32_T buflen,INT32_T flags )
{
        int ret = IPANEL_ERR;
        int recvbyte ; 
	char *str[100];

		
   	#ifdef ipanel_debug_socket
	//CH_ipanel_Print("\n time=%d, ipanel_porting_socket_recv  sockfd=%d ",get_time_ms(),sockfd);
	#endif

        if ( sockfd >= 0 && buf != NULL && buflen > 0)
        {
		INT32_T socktype ; 
		IPANEL_IPADDR ipAddr ;
		INT32_T port ;
		socktype = CH_ipanel_get_socktype(ipanel_mgr,sockfd);
		if(IPANEL_SOCK_DGRAM ==  socktype) 
		{
			ipAddr.version = IPANEL_IP_VERSION_4 ;
			ret = ipanel_porting_socket_recvfrom(sockfd,buf,buflen,0,&ipAddr,&port);
			//eis_report("\n time=%d, IPANEL_SOCK_DGRAM  ipanel_porting_socket_recvsize=%d ",get_time_ms(),ret);
			memcpy(str,buf,100);
			str[99]=0;
			eis_report("\n recvfrom end data=%s ",str);

			return ret ;
		}
		else if( IPANEL_SOCK_STREAM == socktype)
		{
			#if 0
		     if(buflen > TCP_WINDOW_SIZE/2)
		  	buflen = TCP_WINDOW_SIZE/2;
			 #endif
	        	recvbyte = CHMID_TCPIP_Recv(sockfd, buf, buflen, 0);
			if ( recvbyte < 0) 
			{				
				if( -1 == recvbyte )
					return 0;
			
				eis_report("\n time=%d,  ipanel_porting_socket_recv  recv byte failed! ",get_time_ms());
				return ret ;
			}
		}
        }
		
  	#ifdef ipanel_debug_socket
	eis_report(" \n  sockfd=%d recv data byte = %d ", sockfd,recvbyte);
	#else
	task_delay(ST_GetClocksPerSecond()/1000);
	#endif

        return recvbyte;
}

INT32_T ipanel_porting_socket_recvfrom ( INT32_T sockfd,CHAR_T *buf,
                INT32_T buflen,INT32_T flags,IPANEL_IPADDR* ipaddr,INT32_T *port )
{
        int ret = IPANEL_ERR;
        int recvfbyte =0 ;  
        sockaddr_in_t serv_addr;
	int err = -1;
	char *str[100];

    	#ifdef ipanel_debug_socket
	eis_report("\n time=%d,  ipanel_porting_socket_recvfrom  socket=%d ",get_time_ms(),sockfd);
	#endif

        if ( ( sockfd >= 0 ) && ( NULL != buf ) )
        {	
        	 if (IPANEL_IP_VERSION_4 == ipaddr->version)
        	 {
	     	      socklen_t len = sizeof(serv_addr);
			recvfbyte = CHMID_TCPIP_RecvFrom(sockfd, buf, buflen, 0, &serv_addr,(socklen_t *) &len);
		         if ( recvfbyte > 0 )
		         {
	                        if ( ipaddr != NULL)
	                        {
                                ipaddr->addr.ipv4 = CHARTOINT((unsigned char *)CHMID_TCPIP_InetNtoa(serv_addr.ui_IP));
	                        }

	                        if ( port  != NULL )
	                        {
	                                *port =CHMID_TCPIP_NTOHS(serv_addr.us_Port);
	                        }

	                        do_report(0,"\n ipanel_porting_socket_recvfrom recv data length = %d, ip = 0x%x \n",recvfbyte,  ipaddr->addr.ipv4); 
		         }
		         else
		         {
	                        do_report(0,"\n ipanel_porting_socket_recvfrom  recv from byte failed!");
	                         return recvfbyte ; 
		         }			  
        	 }
        	 else if (IPANEL_IP_VERSION_6 == ipaddr->version)
        	 {
	                /* IPv6协议 */
		         do_report(0,"\n ipanel_porting_socket_recvfrom  ERROR: NOT suppurt IPV6 !!! ");
		         return recvfbyte ; 
        	 }
        	 else
        	 {
		         do_report(0,"\n ipanel_porting_socket_recvfrom  ERROR: please set  IP_VERSION_4 !");
		         return ret ;
        	 }			 
        }

   	#ifdef ipanel_debug_socket
	//CH_ipanel_Print(" \n time=%d, ++++++++++++++++++++++++++++recv data byte = %d ",get_time_ms(),recvfbyte);
	eis_report("\n <<<<<<<<<< recvfrom  end pEisHandle=%x, time=%d, socket id = %d",pEisHandle,get_time_ms(), sockfd);
	#endif


        return recvfbyte;
}

INT32_T ipanel_porting_socket_getsockopt(INT32_T sockfd,
                                         INT32_T level,
                                         INT32_T optname,
                                         VOID *optval,
                                         INT32_T *optlen)
{
	INT32_T ret = IPANEL_OK;
	STATUS sockret ; 
	
    	#ifdef ipanel_debug_socket
	eis_report("\n ipanel_porting_socket_getsockopt  socket=%d ",sockfd);
	#endif
	sockret = CHMID_TCPIP_GetSocketOpt(sockfd,	level,optname,optval,optlen);
	if( sockret != 0)
	{
		do_report(0,"\n ipanel_porting_socket_getsockopt get socket opt failed!");
		ret = IPANEL_ERR ;
	}

	return ret;
}

INT32_T ipanel_porting_socket_setsockopt(INT32_T sockfd,
                                         INT32_T level,
                                         INT32_T optname,
                                         VOID *optval,
                                         INT32_T optlen)
{
	INT32_T ret = IPANEL_OK;
	STATUS sockret ; 
	
    	#ifdef ipanel_debug_socket
	eis_report("\n time=%d,  ipanel_porting_socket_setsockopt  socket=%d ",get_time_ms(),sockfd);
	#endif
	sockret = CHMID_TCPIP_SetSocketOpt(sockfd,	level,optname,optval,optlen);
	if( sockret != 0)
	{
		do_report(0,"\n ipanel_porting_socket_setsockopt   set socket opt failed! \n");
		ret = IPANEL_ERR ;
	}

	return ret;
}

INT32_T ipanel_porting_socket_getsockname ( INT32_T sockfd,IPANEL_IPADDR * ipaddr,INT32_T *port )
{
        INT32_T ret = IPANEL_ERR;
        STATUS getstat ; 
        sockaddr_in_t serv_addr;

    	#ifdef ipanel_debug_socket
	eis_report("\n ipanel_porting_socket_setsockopt  socket=%d, ipaddr=0x%x, port=%d ",sockfd, ipaddr, port);
	#endif
	
        if ( sockfd >= 0 )
        {
        	  if (IPANEL_IP_VERSION_4 == ipaddr->version)
        	  {
	                socklen_t len = sizeof (serv_addr);
	                getstat = CHMID_TCPIP_GetSocketName(sockfd, &serv_addr, (socklen_t *)&len);
	                if ( getstat >= 0 )
	                {
	                        if ( ipaddr )
	                        {
	                                ipaddr->addr.ipv4 = CHARTOINT((unsigned char *)CHMID_TCPIP_InetNtoa(serv_addr.ui_IP));
	                        }

	                        if ( port )
	                        {
	                                *port = CHMID_TCPIP_NTOHS(serv_addr.us_Port);
	                        }

	                        do_report(0,"\n ipanel_porting_socket_setsockopt   ip = 0x%x , port = %d", ipaddr->addr.ipv4, *port); 
 							
	                        ret = IPANEL_ERR; 
	                }
        	  }
	        else if (IPANEL_IP_VERSION_6 == ipaddr->version)
	        {
	            	/* IPv6协议 */
	               do_report(0,"\n ipanel_porting_socket_setsockopt ERROR: NOT suppurt IPV6 !!! \n");
	                return ret ; 				
	        }		
	        else
	        {
	               do_report(0,"\n ipanel_porting_socket_setsockopt ERROR: please set  IP_VERSION_4 !\n");
	                return ret ;
	        }			
        }

        return ret;
}

INT32_T ipanel_porting_socket_select ( INT32_T nfds,IPANEL_FD_SET_S* readset,
                                       IPANEL_FD_SET_S* writeset,IPANEL_FD_SET_S* exceptset,INT32_T timeout )
{
#if 0
  struct timeval {
    long    tv_sec;         /* seconds */
    long    tv_usec;        /* and microseconds */
  };
{
    int  i;

    struct timeval tm= {0,2};

    IPANEL_FD_SET_S fds_r = {{0}};
    IPANEL_FD_SET_S fds_w = {{0}};
    IPANEL_FD_SET_S fds_e = {{0}};

    if (nfds <= 0)
    {
        return IPANEL_ERR;
    }
			//eis_report(" \n ipanel_porting_socket_select start exceptset= ");

    for (i = 0; i < nfds; i++)
    {
        if (readset)
        {
            fds_r.fds_bits[i] = readset->fds_bits[i];
		//eis_report("\n readset->fds_bits[%d]=0x%x",i,readset->fds_bits[i]);
        }

        if (writeset)
        {
            fds_w.fds_bits[i] = writeset->fds_bits[i];
			//eis_report("writeset->fds_bits[%d]=0x%x",i,writeset->fds_bits[i]);
		}

        if (exceptset)
        {
            fds_e.fds_bits[i] = exceptset->fds_bits[i];
			//eis_report(" %x",exceptset->fds_bits[i]);
        }
    }
    
   // tm.tv_sec   = timeout / 1000;
   // tm.tv_usec  = (timeout % 1000) * 1000;

    if (smsc_select(nfds+1, &fds_r, &fds_w, &fds_e, &tm) <= 0)
    {
    for (i = 0; i < IPANEL_FD_SET_SIZE; i++)

        if (exceptset)
        {
            exceptset->fds_bits[i] = 0;
		//eis_report("  %x",exceptset->fds_bits[i]);
        }
        return IPANEL_ERR;
    }
	//eis_report(" \n ipanel_porting_socket_select end exceptset= ");

    for (i = 0; i < nfds; i++)
    {
        if (readset)
        {
            readset->fds_bits[i] = fds_r.fds_bits[i];
		//eis_report("\n readset->fds_bits[%d]=0x%x",i,readset->fds_bits[i]);
        }

        if (writeset)
        {
            writeset->fds_bits[i] = fds_w.fds_bits[i];
		//eis_report("writeset->fds_bits[%d]=0x%x",i,writeset->fds_bits[i]);
        }

    }
    for (i = 0; i < IPANEL_FD_SET_SIZE; i++)

        if (exceptset)
        {
            exceptset->fds_bits[i] = 0;
			//eis_report("  %x",exceptset->fds_bits[i]);
        }

    return IPANEL_OK;
}

#else
    int  i;

    IPANEL_FD_SET_S fds_r = {{0}};
    IPANEL_FD_SET_S fds_w = {{0}};
    IPANEL_FD_SET_S fds_e = {{0}};

    if (nfds <= 0)
    {
        return IPANEL_ERR;
    }
			//eis_report(" \n ipanel_porting_socket_select start exceptset= ");

    for (i = 0; i < nfds; i++)
    {
        if (readset)
        {
            fds_r.fds_bits[i] = readset->fds_bits[i];
		//eis_report("\n readset->fds_bits[%d]=0x%x",i,readset->fds_bits[i]);
        }

        if (writeset)
        {
            fds_w.fds_bits[i] = writeset->fds_bits[i];
			//eis_report("writeset->fds_bits[%d]=0x%x",i,writeset->fds_bits[i]);
		}

        if (exceptset)
        {
            fds_e.fds_bits[i] = exceptset->fds_bits[i];
			//eis_report(" %x",exceptset->fds_bits[i]);
        }
    }
    
   // tm.tv_sec   = timeout / 1000;
   // tm.tv_usec  = (timeout % 1000) * 1000;
   if(timeout == -1)
   	{
              timeout= 0	;
   	}
       else if(timeout == 0)
   	{
              timeout = 2;
   	}
	   	task_delay(ST_GetClocksPerSecond()/500);
     //do_report(0,"CHMID_TCPIP_Select\n");
    if (CHMID_TCPIP_Select(nfds, (CHMID_TCPIP_FDSet_t *)(&fds_r), (CHMID_TCPIP_FDSet_t *)(&fds_w),(CHMID_TCPIP_FDSet_t *)( &fds_e), timeout) <= 0)
    {
    for (i = 0; i < IPANEL_FD_SET_SIZE; i++)

        if (exceptset)
        {
            exceptset->fds_bits[i] = 0;
		//eis_report("  %x",exceptset->fds_bits[i]);
        }

        return IPANEL_ERR;
    }
	//eis_report(" \n ipanel_porting_socket_select end exceptset= ");

    for (i = 0; i < nfds; i++)
    {
        if (readset)
        {
            readset->fds_bits[i] = fds_r.fds_bits[i];
		//eis_report("\n readset->fds_bits[%d]=0x%x",i,readset->fds_bits[i]);
        }

        if (writeset)
        {
            writeset->fds_bits[i] = fds_w.fds_bits[i];
		//eis_report("writeset->fds_bits[%d]=0x%x",i,writeset->fds_bits[i]);
        }

    }
    for (i = 0; i < IPANEL_FD_SET_SIZE; i++)

        if (exceptset)
        {
            exceptset->fds_bits[i] = 0;
			//eis_report("  %x",exceptset->fds_bits[i]);
        }

    return IPANEL_OK;
	#endif
}
INT32_T ipanel_porting_socket_ioctl ( INT32_T sockfd, INT32_T cmd, INT32_T arg )
{
        INT32_T ret = IPANEL_OK;
        STATUS ioctlstat ; 
		
    	#ifdef ipanel_debug_socket
	eis_report("\ntime=%d, ipanel_porting_socket_ioctl   socket=%d",get_time_ms(),sockfd);
	#endif
	#if 0  /*  ?????????????????????? */				              
        if ( ( sockfd >= 0 ) && ( IPANEL_FIONBIO == cmd ) )
        {
		ioctlstat = fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFL)|O_NONBLOCK);	  // 非阻塞模式 	     
		if( ioctlstat >= 0) 
    		{
			do_report(0,"**[ipanel_porting_socket_ioctl]:set nonblock failed...........................................\n");
			ret = IPANEL_OK; 
    		}
		else
			do_report(0,"**[ipanel_porting_socket_ioctl]:set nonblock succeed...........................................\n");
        }
	#endif
        return ret;
}

INT32_T ipanel_porting_socket_shutdown ( INT32_T sockfd, INT32_T what )
{
        INT32_T ret = IPANEL_OK;
        INT32_T flags   = -1;

      	#ifdef ipanel_debug_socket
	eis_report("\n ipanel_porting_socket_ioctl   sockfd=%d, what=%d", sockfd, what);
	#endif
	
        if ( ( sockfd >= 0 ) && ( ( what >= 0 ) && ( what <= 2 ) ) )
        {
	        if (IPANEL_DIS_RECEIVE == what)
	        {
	            	flags = 0;
	        }
	        else if (IPANEL_DIS_SEND == what)
	        {
	            	flags = 1;
	        }
	        else if (IPANEL_DIS_ALL == what)
	        {
	            	flags = 2;
	        }        	  
        }

        return ret;
}

INT32_T ipanel_porting_socket_close ( INT32_T sockfd )
{
        INT32_T ret = IPANEL_ERR;
        STATUS closestat ; 
	  
      	#ifdef ipanel_debug_socket
	eis_report("\n time=%d, ipanel_porting_socket_close   sockfd=%d",get_time_ms(), sockfd);
	#endif

        if ( sockfd >= 0 )
        {
		  CH_ipanel_register_unregister(ipanel_mgr,sockfd);
                closestat = CHMID_TCPIP_Close(sockfd );
		  if ( closestat >= 0)
		  {
		  	 ret = IPANEL_OK; 
		  }
        }

      	#ifdef ipanel_debug_socket
	eis_report("\n ipanel_porting_socket_close   ok ");
	#endif
	CH_ipanel_SocketReduce();
        return ret;
}

INT32_T ipanel_porting_socket_get_max_num(VOID)
{
       #ifdef ipanel_debug_socket
	eis_report("\n ipanel_porting_socket_get_max_num ");
	#endif
	
      return MAX_SOCKETS;
}

int ipanel_porting_device_read(const char* params, char* buf, int length)
{
	int readlen = 0;
	char cTmpBuf[32];
	unsigned long smartID;
	char * ctemp = NULL;
	
       #ifdef ipanel_debug_socket
	eis_report("\n ipanel_porting_device_read ");
	#endif
	
#ifdef USE_HWVOD
	if(0 == (strcmp(params,"cardid")))
	{
		smartID = NDS_Pif_Get_SmartCard_ID();
		memset( cTmpBuf, 0, 32 );
		readlen=ipanel_long_to_char(smartID,cTmpBuf);
		readlen=(readlen<length)? readlen: length;
		sprintf( buf,"400%s", cTmpBuf);
		return (readlen + 3);
	}
	else
	{
		hw_client_jsevent_read(params, length, buf, &readlen);
	}
#endif



} 


int ipanel_porting_device_write(const char* params, char *buf, int length)
{
	return 0;
}
int eis_mask_check(UINT32_T ipaddr,UINT32_T netmask,UINT32_T gateway)
{
	int flag = 0;
	int i;
	int errNum=0;
	UINT32_T mask=netmask;
	int maskBit =0x0001;
	do{
			if (0X7f != mask >> 25)//把mask右移25位，如果不等于0Xfe则出错,保证前7位全为1;
			{
				errNum = 21;
				break;
			}
			if (0 != mask << 24)//把mask左移24位，如果不等0则出错，保证后8后为全0
			{
				errNum = 21;
				break;
			}
			
			for (i = 0;i < 32;i++)
			{
				if (mask & maskBit << i)
				{
					if (0 == flag)
					{
						flag = 1;
					}
				}
				else
				{
					if (1 == flag)
					{
						//前面已经有了一个1，后面就不能有0了,出错
						errNum = 21;
						break;
					}
				}
			}
	}
	while(0);
	
	if (0 == errNum)//此块为判断
	{
		UINT32_T ip = ipaddr;
		UINT32_T mask = netmask;
		UINT32_T gateway1 = gateway;
		UINT32_T ipMask = ip & mask;
		UINT32_T gatewayMask  = gateway1 & mask;

		if (ipMask != gatewayMask)
		{
			errNum = 22;
		}
	}
		return 	errNum;
}
void eis_set_ip(U8 ipaddr[4],U8 netmask[4],U8 gateway[4],U8 mac[6],int mode)
{
	CHDRV_NET_Config_t localNetCfg;
	
	memset(&localNetCfg,0,sizeof(CHDRV_NET_Config_t));
	CHMID_TCPIP_GetNetConfig(&localNetCfg);
	memcpy(&(localNetCfg.ipaddress[0]),&(ipaddr[0]),4);
	memcpy(&(localNetCfg.netmask[0]),&(netmask[0]),4);
	memcpy(&(localNetCfg.gateway[0]),&(gateway[0]),4);
	memcpy(&(localNetCfg.macaddress[0]),&(mac[0]),6);
	if(mode == 1)
		localNetCfg.ipmodel = MANUAL;
	else
		localNetCfg.ipmodel = AUTO_DHCP;
	/*此处如果更新IP失败的话会阻塞10s*/
	CHMID_TCPIP_NetConfigRenew(localNetCfg);
}
void eis_DHCP_Start(void)
{
	U8 ipaddr[4],netmask[4],gateway[4],mac[6];
	memset(ipaddr,0,4);
	memset(netmask,0,4);
	memset(gateway,0,4);
	EIS_LoadMacAddr(&mac);
	eis_set_ip(ipaddr,netmask,gateway,mac,0);
	eis_init_ipaddr();
	
}

static boolean first_dhcp = false;
boolean get_ip_first_set(void)
{
	return first_dhcp;
}

INT32_T ipanel_porting_network_ioctl(IPANEL_NETWORK_DEVICE_e device, IPANEL_NETWORK_IOCTL_e op, VOID *arg)
{
	INT32_T ret = -1;
	char *p = NULL;
	unsigned short  net1=20;
	IPANEL_NETWORK_IF_PARAM *p1=NULL;
	IPANEL_NETWORK_IF_PARAM *p2=NULL;
	UINT32_T addr = 0;
	IPANEL_NETWORK_IF_PARAM *ipCfg = NULL;
	U8 ipaddr[4],netmask[4],gateway[4],mac[6];
	int dns_num=0;
	char dns_addr[30];
	CHMID_CM_ONLINE_STATUS_e cmStatus = CHMID_CM_OFFLINE;

	eis_report("\n ipanel_porting_network_ioctl[%d] [%d] time=%d\n",device,op);
	if(device==IPANEL_NETWORK_DEVICE_CABLEMODEM)
	{
		switch( op )
		{
			case IPANEL_NETWORK_GET_MAC: /*取Cable modem  的mac地址*/
				if(NULL!=arg)
				{
					U8 MAC_CM[10]={0,0,0,0,0,0,0,0,0};

					eis_get_CMMAC(MAC_CM);
					if((NULL!=arg))
					{
						eis_report("CMMac: %02x-%02x-%02x-%02x-%02x-%02x",MAC_CM[0],MAC_CM[1],MAC_CM[2],MAC_CM[3],MAC_CM[4],MAC_CM[5]);
						sprintf(arg,"%02x-%02x-%02x-%02x-%02x-%02x",MAC_CM[0],MAC_CM[1],MAC_CM[2],MAC_CM[3],MAC_CM[4],MAC_CM[5]);
						return IPANEL_OK;
					}
					else
					{
						return IPANEL_ERR;
					}
				}
				break;
			case IPANEL_NETWORK_GET_STATUS: /* 取CM的在线状态 */
				{
					if(arg!=NULL)
					{
							cmStatus=CHMID_CM_CheckCMStatus();
						if(CHMID_CM_ONLINE == cmStatus)
							{
								*(int*)arg=IPANEL_NETWORK_CM_ONLINE;
								eis_report("CM online\n");
							}
						else
						{
						*(int*)arg = IPANEL_NETWORK_CM_OFFLINE;
						eis_report("CM offline\n");
						}
						/* *(int*)arg=IPANEL_NETWORK_CM_ONLINE; for test */

							return IPANEL_OK;
					}
					else
					{
						return IPANEL_ERR;
					}
				}			
			case IPANEL_NETWORK_SET_CM_DOWNLINKFREQ: /* 设置CM下行频率 */
				{
					int freq;
					freq=*((int*)arg);
					eis_set_CMDownFreq(freq);
					eis_report("\nIPANEL_NETWORK_SET_CM_DOWNLINKFREQ  freq=%d",*((int*)arg));
				}
				return IPANEL_OK;
				break;
			case IPANEL_NETWORK_GET_DOWNLINK_STATUS:
				{
					if(arg!=NULL)
					{
					cmStatus=CHMID_CM_CheckCMStatus();
					if(cmStatus>=CHMID_CM_DOWN_FREQ_LOCKED)
					{
					*(int*)arg=1;
					eis_report("CHMID_CM_DOWN_FREQ_LOCKED\n");
					}
					else
					{
					 *(int*)arg=0;
					eis_report("CM offline\n");
					}
					}
					return IPANEL_OK;
				}	
				break;
			case IPANEL_NETWORK_GET_UPLINK_STATUS:
				if(arg!=NULL)
				{
					cmStatus=CHMID_CM_CheckCMStatus();
					if(cmStatus>=CHMID_CM_UP_FREQ_LOCKED)
					{
					 *(int*)arg=1;
					eis_report("IPANEL_NETWORK_GET_UPLINK_STATUS\n");
					}
					else
					{
					*(int*) arg=0;
					eis_report("CM offline\n");
					}
				}	
				return IPANEL_OK;

				break;
			case IPANEL_NETWORK_GET_CM_UPLINKFREQ:
				if(arg!=NULL)
				{
					char freq_str[30];
					int freq;
					char *end_p;
					memset(freq_str,0,30);
					eis_get_CMUpFreq(freq_str);
					freq=strtod(freq_str,&end_p);
					*((int *)arg)  = freq/100;
				}
				return IPANEL_OK;
				break;
				
			case IPANEL_NETWORK_GET_CM_DOWNLINKFREQ:
				if(arg!=NULL)
				{
					char freq_str[30];
					int freq;
					char *end_p;
					memset(freq_str,0,30);
					eis_get_CMDownFreq(freq_str);
					freq=strtod(freq_str,&end_p);
					*((int *)arg)  = freq/100;
				}
				return IPANEL_OK;
				break;
			
			default:
				return IPANEL_ERR;
				break;
		}
	}
	else
	{
		switch( op ){
			case IPANEL_NETWORK_CONNECT:
				if (IPANEL_NETWORK_DEVICE_LAN == device)
				{
					if(IPANEL_NETWORK_LAN_ASSIGN_IP == (U32)arg)
					{
						char strDns1[32] = {0};
						char strDns2[32] = {0};
						/*When static ip mode checked, iPanel notify dns to skyworth*/
						//ipanel_get_dns_server(1, (CHAR_T *)strDns1, 32);
						//ipanel_get_dns_server(2, (CHAR_T *)strDns2, 32);
						//CHMID_TCPIP_GetIP(&addr);
						 //eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_READY, 0 );
					}
					else if (IPANEL_NETWORK_LAN_DHCP == (U32)arg)
					{
						 eis_report("\n restart DHCP...");
						if(first_dhcp)
						{
							EIS_DHCP_set = true;
						}
						else
						{
							unsigned short  net1_temp=20;
							static UINT32_T ip_addr_temp = 0;

							first_dhcp = true;
							net1_temp = CHMID_TCPIP_GetNetStatus();
							CHMID_TCPIP_GetIP(&ip_addr_temp);
							if((net1_temp&CHMID_TCPIP_LINKUP)&&(ip_addr_temp!=0) )
							{
									 eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_READY, 0 );
				 					 eis_report("\n send IPANEL_IP_NETWORK_READY to IPANLE 1");
							}
							else
							{
									eis_report("\n send IPANEL_IP_NETWORK_FAILED to IPANLE");
									eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_FAILED, 0 );
							}
							
						}
						// eis_api_msg_send ( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_DISCONNECT, 0 );
						
					}
				}
				break;
			case IPANEL_NETWORK_DISCONNECT:
				eis_report("\nbegain the IPANEL_NETWORK_DISCONNECT");
				break;
			case IPANEL_NETWORK_SET_IFCONFIG:
				{
					{
						 if(NULL == arg)
						 	break;
						 
						ipCfg=(IPANEL_NETWORK_IF_PARAM *)arg;			 
						ipaddr[0]=((ipCfg->ipaddr)>>24) & 0xff;
						ipaddr[1]=((ipCfg->ipaddr)>>16) & 0xff;
						ipaddr[2]=((ipCfg->ipaddr)>>8 ) & 0xff;
						ipaddr[3]=ipCfg->ipaddr & 0xff;
						netmask[0]=ipCfg->netmask>>24 & 0xff;
						netmask[1]=ipCfg->netmask>>16 & 0xff;
						netmask[2]=ipCfg->netmask>>8 & 0xff;
						netmask[3]=ipCfg->netmask & 0xff;
						gateway[0]=ipCfg->gateway>>24 & 0xff;
						gateway[1]=ipCfg->gateway>>16 & 0xff;
						gateway[2]=ipCfg->gateway>>8 &0xff;
						gateway[3]=ipCfg->gateway & 0xff;
						EIS_LoadMacAddr(&mac);
						eis_set_ip(ipaddr,netmask,gateway,mac,1);
						ManualIP_Check(NULL);

						//eis_set_ManualIPCheck(true);
						//eis_api_msg_send( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_READY, 0);

						eis_api_msg_send( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_READY, 0);
					}
			
				}
				eis_report("\nbegain the IPANEL_NETWORK_SET_IFCONFIG");
				break;
			case IPANEL_NETWORK_GET_IFCONFIG:
				{
					U8 dns_addr_0,dns_addr_1,dns_addr_2,dns_addr_3;
					eis_report("\nbegain the IPANEL_NETWORK_GET_IFCONFIG");
					if(arg==NULL)
						return IPANEL_ERR;
					
					p1=(IPANEL_NETWORK_IF_PARAM*)arg;
					CHMID_TCPIP_GetGateWay(&addr);
					p1->gateway=CHMID_TCPIP_NTOHL(addr);
					CHMID_TCPIP_GetIP(&addr);
					p1->ipaddr=CHMID_TCPIP_NTOHL(addr);
					CHMID_TCPIP_GetNetMask(&addr);
					p1->netmask=CHMID_TCPIP_NTOHL(addr);
					CHMID_TCPIP_GetDNSServerNum(&dns_num);
					eis_report("\n new ip=0x%x, gateway=0x%x",p1->ipaddr,p1->gateway);
					if(dns_num==1)
					{
						CHMID_TCPIP_GetDNSServer(0,&addr);
						dns_addr_3=(addr>>24) & 0xff;
						dns_addr_2=(addr>>16) & 0xff;
						dns_addr_1=(addr>>8) & 0xff;
						dns_addr_0=(addr) & 0xff;
						sprintf(dns_addr,"%02d.%02d.%02d.%02d",dns_addr_0,dns_addr_1,dns_addr_2,dns_addr_3);
						ipanel_set_dns_server(1,dns_addr);
					}
					else
					{
						CHMID_TCPIP_GetDNSServer(0,&addr);
						dns_addr_3=(addr>>24) & 0xff;
						dns_addr_2=(addr>>16) & 0xff;
						dns_addr_1=(addr>>8) & 0xff;
						dns_addr_0=(addr) & 0xff;
						sprintf(dns_addr,"%02d.%02d.%02d.%02d",dns_addr_0,dns_addr_1,dns_addr_2,dns_addr_3);
						ipanel_set_dns_server(1,dns_addr);
						CHMID_TCPIP_GetDNSServer(1,&addr);
						dns_addr_3=(addr>>24) & 0xff;
						dns_addr_2=(addr>>16) & 0xff;
						dns_addr_1=(addr>>8) & 0xff;
						dns_addr_0=(addr) & 0xff;
						sprintf(dns_addr,"%02d.%02d.%02d.%02d",dns_addr_0,dns_addr_1,dns_addr_2,dns_addr_3);
						ipanel_set_dns_server(2,dns_addr);
					}
				}
				break;
			case IPANEL_NETWORK_GET_DNS_CONFIG:
				break;
			case IPANEL_NETWORK_GET_STATUS:
				{
					if(arg!=NULL)
					{
						if(device==IPANEL_NETWORK_DEVICE_LAN)
						{
								net1=CHMID_TCPIP_GetNetStatus();
							eis_report("\nthe netstatus======%d",net1);
							if(net1&CHMID_TCPIP_LINKUP){
								*(int*)arg=IPANEL_NETWORK_IF_CONNECTED;
								eis_report("\nbegain the IPANEL_NETWORK_GET_STATUS----successed");
							}else if(net1&CHMID_TCPIP_RENEWING_IP){
								*(int*)arg=IPANEL_NETWORK_IF_CONNECTING;
								eis_report("\nbegain the IPANEL_NETWORK_GET_STATUS----connectting");
							}else {
								*(int*)arg=IPANEL_NETWORK_IF_DISCONNECTED;
								eis_report("\nbegain the IPANEL_NETWORK_GET_STATUS----disconnectted");
							}
						}
						
					}
				}
				break;
			case IPANEL_NETWORK_SET_STREAM_HOOK:
				eis_report("\nbegain the IPANEL_NETWORK_SET_STREAM_HOOK");
				break;
			case IPANEL_NETWORK_SET_USER_NAME:
				eis_report("\nbegain the IPANEL_NETWORK_SET_USER_NAME");
				break;
			case IPANEL_NETWORK_SET_PWD:
				eis_report("\nbegain the IPANEL_NETWORK_SET_PWD");
				break;
			case IPANEL_NETWORK_SET_DIALSTRING:
				eis_report("\nbegain the IPANEL_NETWORK_SET_DIALSTRING");
				break;
			case IPANEL_NETWORK_GET_NIC_MODE :
				{
					if(arg!=NULL)
					{
					*(int*)arg=IPANEL_NETWORK_AUTO_CONFIG;
					}
				}
				break;
			case IPANEL_NETWORK_GET_MAC:
				//CH_ReadSTBMac(arg);
				break;
			case IPANEL_NETWORK_SEND_PING_REQ:
				{
					CHMID_TCPIP_Pingcfg_t pingCfg;
					INT32_T timeUsed = 0;
					UINT32_T addr = 0;
					CHMID_TCPIP_RESULT_e pingRet = CHMID_TCPIP_FAIL;
					if(arg==NULL)
						return IPANEL_ERR;
					
					pingCfg.i_Datalen = 32;
					pingCfg.i_Timeoutms = 5000;
					addr = CHMID_TCPIP_InetAddr(( s8_t *) arg);
					pingCfg.ui_IPAddr[0] = addr&0xfful;
					pingCfg.ui_IPAddr[1] = (addr>>8)&0xfful;
					pingCfg.ui_IPAddr[2] = (addr>>16)&0xfful;
					pingCfg.ui_IPAddr[3] = (addr>>24)&0xfful;
					pingRet = CHMID_TCPIP_Ping(pingCfg,&timeUsed);
					if(pingRet != CHMID_TCPIP_OK)
						timeUsed = -1;
					eis_report("ping timeUsed = %d\n",timeUsed);
					eis_api_msg_send( IPANEL_EVENT_TYPE_NETWORK, IPANEL_IP_NETWORK_PING_RESPONSE, timeUsed);
				}
				break;
			case IPANEL_NETWORK_STOP_PING_REQ:
				break;
			default:
				break;
		}
	}
    	return IPANEL_OK;

}

/*eof*/
