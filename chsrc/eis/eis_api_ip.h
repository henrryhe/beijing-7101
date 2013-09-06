/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_ip.h
  * 描述: 	定义双向相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */

#ifndef __EIS_API_IP__
#define __EIS_API_IP__

#define MAX_SOCKETS 16

/*
功能说明：
	建立一个套接字。
参数说明：
	输入参数：
		family：指定要创建的套接字协议族。IPANEL_AF_INET：表示IPv4 协议，
		IPANEL_AV_INET6 表示IPv6 协议，其他值保留
		type ： 指定要创建的套接字的类型。IPANEL_SOCK_STREAM ： 表示
		SOCK_STREAM，IPANEL_SOCK_DGRAM：表示SOCK_DGRAM，
		其他值保留
		protocol：指定哪种协议。通常设置为IPANEL_IPPROTO_IP，表示与type 匹
		配的默认协议，其他值保留
	输出参数：无
	返 回：
		若套接字建立成功，则返回非负描述符，否则返回-1。
		> ＝ 0: 套接字建立成功，则返回非负描述符，描叙符的值应该不超过
		IPANEL_NFDBITS 的定义范围；
		IPANEL_ERR: 创建套接字失败。
  */
extern INT32_T ipanel_porting_socket_open(INT32_T family,INT32_T type, INT32_T protocol);


/*
功能说明：
	将指定的端口号与套接字绑定在一起。
参数说明：
	输入参数：
		sockfd：套接字描述符
		ipaddr：包含待绑定IP 地址和IP 版本描述结构体的指针
		port：端口号
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_socket_bind(INT32_T sockfd,IPANEL_IPADDR *ipaddr, INT32_T port);


/*
功能说明：
	响应连接请求，建立连接并产生一个新的socket 描述符来描述该连接
参数说明：
	输入参数：
		sockfd：监听套接字描述符
		ipaddr：包含IP 版本描述的结构体指针
	输出参数：
		ipaddr：保存连接对方IP 地址的结构体指针
		port：保存连接对方的端口号
		返 回：
		>＝0:执行成功则返回非负描述符。
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_socket_accept(INT32_T sockfd,IPANEL_IPADDR *ipaddr, INT32_T *port);


/*
功能说明：
	将套接字与指定的套接字连接起来
参数说明：
	输入参数：
		sockfd：套接字描述符
		ipaddr：包含待连接IP 地址及IP 版本描述的结构体指针
		port：待连接的端口号
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_socket_connect(INT32_T sockfd,IPANEL_IPADDR *ipaddr, INT32_T port);


/*
功能说明：
	发送数据到套接字。
参数说明：
	输入参数：
		sockfd：套接字描述符
		buf：待发送数据的缓冲区
		len：缓冲区中数据的长度
		flags：操作选项，一般设为0，其他值保留
	输出参数：无
	返 回：
		>0:执行成功返回实际发送的数据长度。
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_socket_send(INT32_T sockfd,CHAR_T *buf, INT32_T lenght, INT32_T flags);


/*
功能说明：
	发送数据到指定套接字。
参数说明：
	输入参数：
		sockfd：套接字描述符
		buf：待发送数据的缓冲区
		len：缓冲区中数据的长度
		flags：操作选项，一般设为0，其他值保留
		ipaddr：包含目的IP 地址和IP 版本描述的结构体指针
		port：目的端口号
	输出参数：无
	返 回：
		>=0:执行成功返回实际发送的数据长度。
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_socket_sendto(INT32_T sockfd,CHAR_T *buf, INT32_T len, INT32_T flags, IPANEL_IPADDR*ipaddr, INT32_T port);


/*
功能说明：
	从一个套接字接收数据。
参数说明：
	输入参数：
		sockfd：套接字描述符
		buf：用于接收数据的缓冲区
		len：缓冲区的长度
		flags：操作选项，一般设为0，其他值保留
	输出参数：无
	返 回：
		>=0: 执行成功返回实际接收的数据长度。
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_socket_recv(INT32_T sockfd,CHAR_T *buf, INT32_T len, INT32_T flags);


/*
功能说明：
	从一个指定的套接字接收数据。
参数说明：
	输入参数：
		sockfd：套接字描述符
		buf：用于接收数据的缓冲区
		len：缓冲区的长度
		flags：操作选项，一般设为0，其他值保留
		ipaddr：包含IP 版本描述的结构体指针
	输出参数：
		ipaddr：保存发送方IP 地址的结构体指针
		port：保存发送方的端口号
	返 回：
		>=0: 执行成功返回实际接收的数据长度。
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_socket_recvfrom(INT32_T sockfd, CHAR_T *buf, INT32_T len, INT32_T flags, IPANEL_IPADDR *ipaddr, INT32_T *port);


/*
功能说明：
	获取与套接字有关的本地协议地址，即主机地址。
参数说明：
	输入参数：
		sockfd：套接字描述符
		ipaddr：包含IP 版本描述的结构体指针
	输出参数：
		ipaddr：保存主机协议IP 地址的结构体指针
		port：保存主机协议的端口号
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_socket_getsockname ( INT32_T sockfd, IPANEL_IPADDR *ipaddr, INT32_T *port );


/*
功能说明：
	确定一个或多个套接字的状态，判断套接字上是否有数据，或者能否向一个套接字
	写入数据。
参数说明：
	输入参数：
		nfds： 需要检查的文件描述符个数，数值应该比readset、writeset、
		exceptset 中最大数更大，而不是实际的文件描述符总数
		readset： 用来检查可读性的一组文件描述符
		writeset：用来检查可写性的一组文件描述符
		exceptset：用来检查意外状态的文件描述符，错误不属于意外状态
		timeout： 大于0 表示等待多少毫秒；为IPANEL_NO_WAIT(0)时表示不等待立
		即返回，为IPANEL_WAIT_FOREVER(-1)表示永久等待。
	输出参数：无
	返 回：
		响应操作的对应操作文件描述符的总数，且readset、writeset、exceptset
		三组数据均在恰当位置被修改；
		IPANEL_OK:要查询的文件描叙符已准备好;
		IPANEL_ERR:失败，等待超时或出错。
  */
extern INT32_T ipanel_porting_socket_select(INT32_T nfds,IPANEL_FD_SET_S *readset, IPANEL_FD_SET_S *writeset,IPANEL_FD_SET_S *exceptset, INT32_T timeout);


/*
功能说明：
	获取套接字的属性。可获取套接字的属性见下表中get 项为Y 的属性项。
参数说明：
	输入参数：
		sockfd： 套接字描述符
		level： 选项定义的层次
		optname：需要获取的属性名
		level 				optname 						get 	set 	数据类型

							IPANEL_SO_BROADCAST 		Y 	Y 	INT32_T
		IPANEL_SOL_SOCKET	IPANEL_SO_KEEPALIVE 			Y 	Y 	INT32_T
							IPANEL_SO_REUSEADDR 		Y 	Y 	INT32_T

		IPANEL_IPPROTO_IP	IPANEL_IP_ADD_MEMBERSHIP 	N 	Y 	PANEL_IP_MREQ
							IPANEL_IP_DROP_MEMBERSHIP 	N 	Y 	PANEL_IP_MREQ
	输出参数：
		optval：指向保存属性值的缓冲区
		optlen：指向保存属性值的长度
	返 回：
		IPANEL_OK:成功
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_socket_getsockopt(INT32_T sockfd, INT32_T level, INT32_T optname, VOID *optval,INT32_T *optlen);


/*
功能说明：
	设置套接字的属性。可设置的套接字的属性见下表中set 项为Y 的属性项。
参数说明：
	输入参数：
		sockfd：套接字描述符
		level： 选项定义的层次
		optname：需要设置的属性名
		optval：指向保存属性值的缓冲区
		optlen： optval 缓冲区的长度
	输出参数：无
	返 回：
		IPANEL_OK:成功；
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_socket_setsockopt(INT32_T sockfd,INT32_T level,INT32_T optname,VOID *optval,INT32_T optlen);


/*
功能说明：
	改变套接字属性为非阻塞。注：此函数只提供将套接字属性设置为非阻塞，不提供
	将套接字属性设置为阻塞的功能。
参数说明：
	输入参数：
		sockfd：套接字描述符
		cmd：命令类型，为IPANEL_FIONBIO
		arg：命令参数，1 表示非阻塞
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_socket_ioctl ( INT32_T sockfd, INT32_T cmd, INT32_T arg );


/*
功能说明：
	禁止在一个套接字上进行数据的接收与发送。尽量不要使用此函数。
参数说明：
	输入参数：
		sockfd：套接字描述符。
		what：为IPANEL_DIS_RECEIVE 表示禁止接收；为IPANEL_DIS_SEND 表示
		禁止发送，为IPANEL_DIS_ALL 表示同时禁止发送和接收。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_socket_shutdown(INT32_T sockfd, INT32_T what);


/*
功能说明：
	关闭指定的套接字
参数说明：
	输入参数：
		sockfd：套接字描述符
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_socket_close(INT32_T sockfd);


/*
功能说明：
	获取准备让iPanel MiddleWare 同时处理socket 的最大个数。注意：该数目建议
	在8 个以上，16 个以下，即[8~16]，缺省为8。若iPanel MiddleWare 只是浏览网页的
	话，8 个socket 就足够了，若iPanel MiddleWare 在浏览网页的时候同时又需要使用
	VOIP、VOD 点播等应用就需要多个socket 以提高速度，否则可能出现socket 都已经使
	用，需要关闭一些socket 才能运行某个应用的情况。
参数说明：
	输入参数：无
	输出参数：无
返 回：
	>0:返回iPanel MiddleWare 可创建的最大socket 数目。
	IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_socket_get_max_num ( VOID);
#endif

/*--eof------------------------------------------------------------------------------------------------------------*/

