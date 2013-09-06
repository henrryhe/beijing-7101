/* Copyright 2008 SMSC, All rights reserved
FILE: socktest.h
*/

#ifndef SOCK_TESTER_H
#define SOCK_TESTER_H

#if defined (UNIX) || defined (WIN32) 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#else   /* smsc network stack */
#include "smsc_environment.h"
#include "smsc_threading.h"
#include "utilities/sockets.h"
#endif

#ifdef WIN32
#include <io.h>
#include <winsock2.h>
#endif
#ifdef UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h> 
#endif 

struct tester {
	struct sockaddr_in myAddr; 		/* local address */
	unsigned short lport;			/* local port */
	struct sockaddr_in otherAddr;	/* Dest address */
	unsigned short dport;			/* Dest port */
	char dataCount;		 			/* counter to send incremental data */
	unsigned long count;			/* No. of mesg to send/recv */
	unsigned short sflag;			/* Do transmit */
	unsigned short udp;				/* Do udp test if set 1 else tcp test */
	unsigned int sndMsgLen;			/* send this length else default */
	unsigned int msgCount;			/* no. of mesg sent/received */
	unsigned int byteCount;			/* no. of bytes sent/received */
	unsigned short skipVerify;		/* to skip data verification */
	unsigned short burstTime;		/* time to pause in ms after each burst*/
	unsigned long burstCount;		/* No. of messages to send in each burst */
#ifdef WIN32
	SOCKET sd, nsd;
	double startTime;				/* keep track of start time */
#else 
#ifdef UNIX
	struct timeval startTime;
#else  /* smsc network stack */
	smsc_clock_t startTime;
#endif 
	int sd, nsd;
#endif
	double realTime;			/* time to send/recev */
};

void read_end_time(struct tester *test);
void read_start_time(struct tester *test);
void tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0);
void end_of_transmission(struct tester *test);
int verify_data(struct tester *test, char *msgbuf, int count);
int msgsend(struct tester *test, char *msgbuf, unsigned int msglen);
int msgrecv(struct tester *test, char *msgbuf, unsigned int msglen);

#if !defined (UNIX) || !defined (WIN32)
int socket_test_initiate( struct tester *test, int udp, int sflag, char *destAddr, 	
	int dport, int lport, unsigned int count, int msgLen, short skipVerify, short burstTime, unsigned long burstCount);
int tester_start(struct tester *test);
void tester_thread(void *arg);
#endif

#endif /* SOCK_TESTER_H */
