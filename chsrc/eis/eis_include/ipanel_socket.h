/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author Zouxianyun 2005/04/28                                           */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_SOCKET_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_SOCKET_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IPANEL_INADDR_NONE
#define IPANEL_INADDR_NONE		  (-1)
#endif
	
#define IPANEL_AF_INET            (2)
#define IPANEL_AF_INET6           (23)

#define IPANEL_SOCK_STREAM        (1)
#define IPANEL_SOCK_DGRAM         (2)

#define IPANEL_IPPROTO_IP         (0)
#define IPANEL_SOL_SOCKET         (0xFFFF)
#define IPANEL_SO_REUSEADDR       (0x0004)
#define IPANEL_SO_KEEPALIVE       (0x0008)
#define IPANEL_SO_BROADCAST       (0x0020)
#define IPANEL_SO_LINGER	      (0x0080)
#define IPANEL_IP_ADD_MEMBERSHIP  (12)
#define IPANEL_IP_DROP_MEMBERSHIP (13)

#define IPANEL_FIONBIO            (16)
                                  
#define IPANEL_DIS_RECEIVE        (0)     /* receives disallowed */
#define IPANEL_DIS_SEND           (1)     /* sends disallowed */
#define IPANEL_DIS_ALL            (2)     /* sends and receives disallowed */
                                  
typedef UINT32_T                  IPANEL_FD_MASK;
#define IPANEL_NFDBITS            (8 * sizeof(IPANEL_FD_MASK))
#define IPANEL_FD_SET_SIZE        (16)

#define IPANEL_FD_SET(n, p)       ((p)->fds_bits[(n) / IPANEL_NFDBITS] |=  (1 << ((n) % IPANEL_NFDBITS)))
#define IPANEL_FD_CLR(n, p)       ((p)->fds_bits[(n) / IPANEL_NFDBITS] &= ~(1 << ((n) % IPANEL_NFDBITS)))
#define IPANEL_FD_ISSET(n, p)     ((p)->fds_bits[(n) / IPANEL_NFDBITS] &   (1 << ((n) % IPANEL_NFDBITS)))
#define IPANEL_FD_ZERO(p)         do { \
                                      INT32_T _iii_ = 0; \
                                      for (; _iii_ < IPANEL_FD_SET_SIZE; _iii_++) { \
                                          (p)->fds_bits[_iii_] = 0; \
                                      } \
                                  } while (0)

typedef struct
{
    IPANEL_FD_MASK fds_bits[IPANEL_FD_SET_SIZE];
} IPANEL_FD_SET_S;

/* define IPAddr */
#define IPANEL_IP_VERSION_4       ((BYTE_T)4)
#define IPANEL_IP_VERSION_6       ((BYTE_T)6)

typedef UINT32_T                  IPANEL_IPV4;

typedef struct
{
    UINT16_T ip[8];
} IPANEL_IPV6;

typedef union
{
    IPANEL_IPV4 ipv4;
    IPANEL_IPV6 ipv6;
} IPANEL_UNIIP;

typedef struct
{
    BYTE_T  version;
    BYTE_T  padding1;
    BYTE_T  padding2;
    BYTE_T  padding3;
    IPANEL_UNIIP addr;
} IPAddr0;

/* IPANEL_IPADDR is for external porting usage. IPAddr is for own usage. */
typedef IPAddr0 IPANEL_IPADDR, *IPAddr;

typedef struct
{
    UINT32_T addr;
} IPANEL_IN_ADDR;

typedef struct
{
    IPANEL_IN_ADDR imr_multiaddr;
    IPANEL_IN_ADDR imr_interface;
} IPANEL_IP_MREQ;

typedef struct {
	UINT16_T l_onoff; 
	UINT16_T l_linger; 
} IPANEL_LINGER;
	
/* interfaces */
INT32_T ipanel_porting_socket_open(INT32_T family,
                                   INT32_T type,
                                   INT32_T protocol);

INT32_T ipanel_porting_socket_bind(INT32_T sockfd,
                                   IPANEL_IPADDR *ipaddr,
                                   INT32_T port);

INT32_T ipanel_porting_socket_listen(INT32_T sockfd, INT32_T backlog);

INT32_T ipanel_porting_socket_accept(INT32_T sockfd,
                                     IPANEL_IPADDR *ipaddr,
                                     INT32_T *port);

INT32_T ipanel_porting_socket_connect(INT32_T sockfd,
                                      IPANEL_IPADDR *ipaddr,
                                      INT32_T port);

INT32_T ipanel_porting_socket_send(INT32_T sockfd,
                                   CHAR_T *buf,
                                   INT32_T buflen,
                                   INT32_T flags);

INT32_T ipanel_porting_socket_sendto(INT32_T sockfd,
                                     CHAR_T *buf,
                                     INT32_T buflen,
                                     INT32_T flags,
                                     IPANEL_IPADDR *ipaddr,
                                     INT32_T port);

INT32_T ipanel_porting_socket_recv(INT32_T sockfd,
                                   CHAR_T *buf,
                                   INT32_T buflen,
                                   INT32_T flags);

INT32_T ipanel_porting_socket_recvfrom(INT32_T sockfd,
                                       CHAR_T *buf,
                                       INT32_T buflen,
                                       INT32_T flags,
                                       IPANEL_IPADDR *ipaddr,
                                       INT32_T *port);

INT32_T ipanel_porting_socket_getsockname(INT32_T sockfd,
                                          IPANEL_IPADDR *ipaddr,
                                          INT32_T *port);

INT32_T ipanel_porting_socket_select(INT32_T nfds,
                                     IPANEL_FD_SET_S* readset,
                                     IPANEL_FD_SET_S* writeset,
                                     IPANEL_FD_SET_S* exceptset,
                                     INT32_T timeout);

INT32_T ipanel_porting_socket_getsockopt(INT32_T sockfd,
                                         INT32_T level,
                                         INT32_T optname,
                                         VOID *optval,
                                         INT32_T *optlen);

INT32_T ipanel_porting_socket_setsockopt(INT32_T sockfd,
                                         INT32_T level,
                                         INT32_T optname,
                                         VOID *optval,
                                         INT32_T optlen);

INT32_T ipanel_porting_socket_ioctl(INT32_T sockfd, INT32_T cmd, INT32_T arg);

INT32_T ipanel_porting_socket_shutdown(INT32_T sockfd, INT32_T what);

INT32_T ipanel_porting_socket_close(INT32_T sockfd);

INT32_T ipanel_porting_socket_get_max_num(VOID);

#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_SOCKET_API_FUNCTOTYPE_H_

