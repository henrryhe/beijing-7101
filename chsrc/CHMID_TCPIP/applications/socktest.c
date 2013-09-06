/* Copyright 2008 SMSC, All rights reserved 
 *
 * udp & tcp test application 
 * File : socktest.c 
 */ 
#include "socktest.h"

#ifndef WIN32 /* UNIX or smsc network stack */
#define SOCKET_ERROR	-1
#define INVALID_SOCKET	-1
#endif

#define DEF_PORT 6000
#define DEF_SEND_MSG_SIZE 64
#define MAX_RECV_MSG_SIZE 1500
#define DEF_BURST_COUNT 10
#define START_MARKER "SSSSSS"
#define END_MARKER "EEEEEE"

#if defined (UNIX) || defined (WIN32)
#define SMSC_NOTICE(condition, message) \
	do {								\
		if(condition)					\
			printf message;				\
			printf ("\n");				\
	} while(0)
#define SMSC_TRACE(condition, message) 	\
	do {								\
		if(condition)					\
			printf message;				\
			printf ("\n");				\
	} while(0)
#endif

struct stat {
	unsigned int msgReceived;
	double  secRecvTime;
};

#if defined (UNIX) || defined (WIN32)
extern char *optarg;
struct tester test;

void usage() 
{
	SMSC_NOTICE(1,("usage : socktest <[-s -d <destination ip> <-p port> [-m msg size]]> [-c count] [-l local port] [-u] [-t] [-b burst time] [-n burst count]\n)"));
	SMSC_NOTICE(1, ("where : s - send, u - udp, t - skip data verify)"));
	exit(1);
}

void catch_signal()
{
	/* Record end time */
	read_end_time(&test);

	/* Declare end of transmission */
	if (test.sflag)
		end_of_transmission(&test);
	else {
		SMSC_TRACE(1, ("Msg received:%u Time to receive:=%f", test.msgCount, test.realTime));
	}	
	exit(1);
}

int main(int argc, char *argv[]) 
{
	int i, c, rc = 0;
	char *destAddr = NULL;

#ifdef WIN32
	WSADATA	wsa_data ;
	/* Initialize the winsock lib ( version 2.2 ) */
	if ( WSAStartup(MAKEWORD(2,2), &wsa_data) == SOCKET_ERROR ) {
		SMSC_NOTICE(1, ("WSAStartup() failed"));
		return 1 ;
	}
#endif	
	memset(&test, 0, sizeof(test));
	test.burstCount = DEF_BURST_COUNT;

	while ((c = getopt (argc, argv, "suthc:d:p:l:m:b:n:")) != -1) {
		switch (c) {
			case 's':
				test.sflag = 1;
				break;
			case 'c':
				test.count = atoi(optarg);
				break;
			case 'd':
				destAddr = optarg;
				break;
			case 'p':
				test.dport = atoi(optarg);
				break;
			case 'l':
				test.lport = atoi(optarg);
				break;
			case 'm':
				test.sndMsgLen = atoi(optarg);
				break;
			case 'u':
				test.udp = 1;
				break;
			case 't':
				test.skipVerify = 1;
				break;
			case 'b':
				test.burstTime = atoi(optarg);
				break;
			case 'n':
				test.burstCount = atoi(optarg);
				break;
			case '?':
			case 'h':
				usage();
			default:
				usage();
		}
	}

	if (test.sflag && !destAddr && !test.dport)
		usage();
	if (!test.lport && !test.sflag)
		test.lport = DEF_PORT;

	test.dataCount = 0;
	
	if (!test.sndMsgLen)
		test.sndMsgLen = DEF_SEND_MSG_SIZE;

	test.myAddr.sin_family = AF_INET;
	test.myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	test.myAddr.sin_port = htons(test.lport);	

	if (test.sflag) {
		test.otherAddr.sin_family = AF_INET;
		test.otherAddr.sin_addr.s_addr = inet_addr(destAddr);
		test.otherAddr.sin_port = htons(test.dport);
	}
	
	tester_start(&test); 
}

#else   /* smsc network stack */

int socket_test_initiate(
	struct tester *test,
	int udp, 			/* 1 for udp test, 0 for tcp */
	int sflag, 			/* 1 if sender(client), 0 for receiver(server) */
	char *destAddr, 	/* Destination address, used when sender (sflag=1) */ 
	int dport, 			/* Destination port, used when sender */
	int lport, 			/* Local port to bind, used when receiver (sflag=0) */
	unsigned int count,	/* No. of message to send, used when sender */ 
	int msgLen,			/* Length of the message to send, used when sender */
	short skipVerify,	/* If 1, skip the data verification (only throughput) */
	short burstTime,    /* time to pause in ms after each burst*/
	unsigned long burstCount	/* No. of messages to send in each burst */
	)
{
	test->udp = udp;
	test->sflag = sflag;
	test->dport = dport;
	test->lport = lport;
	test->count = count;
	test->skipVerify = skipVerify;
	test->burstTime = burstTime;
	
	if (!burstCount)
		test->burstCount = DEF_BURST_COUNT;		
	else
		test->burstCount = burstCount;
	    
	if (!msgLen)
		test->sndMsgLen = DEF_SEND_MSG_SIZE;
	else
		test->sndMsgLen = msgLen;

	if (!test->lport && !test->sflag)
		test->lport = DEF_PORT;

	test->dataCount = 0;

	test->myAddr.sin_family = AF_INET;
	test->myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	test->myAddr.sin_port = htons(test->lport);	

	if (test->sflag) {
		test->otherAddr.sin_family = AF_INET;
		test->otherAddr.sin_addr.s_addr = inet_addr(destAddr);
		test->otherAddr.sin_port = htons(test->dport);
	}
	
	/* Start the tester thread */
	smsc_thread_create(tester_thread, (void *)test, 2048, SMSC_THREAD_PRIORITY_HIGH, "socktest", 0);
} 

void tester_thread(void *arg)
{
	tester_start(arg);		
}
#endif

int tester_start(struct tester *test)
{
	int otherAddrLen, i, bCount = 0, rc = 0;
	char *msgbuf;
	struct stat stat;
	fd_set read_fd;

	memset(&stat, 0, sizeof(stat));
#if !(defined (UNIX) || defined (WIN32))
	/* udp sender need to wait till stack initializes */	
	if (test->udp && test->sflag)
		smsc_mdelay(10000);		
#endif
	if (test->udp)
		SMSC_TRACE(1, ("***UDP TEST**"));
	else
		SMSC_TRACE(1, ("***TCP TEST**"));

	if (test->sflag)
		SMSC_TRACE(1, ("Destination addr: %s Destination Port:%d", (char *)inet_ntoa(test->otherAddr.sin_addr), test->dport));
	else
		SMSC_TRACE(1, ("Local port:%d", test->lport));                

	/* socket creation */
	if ((test->sd = socket(AF_INET, test->udp ? SOCK_DGRAM : SOCK_STREAM, 0)) == SOCKET_ERROR) {
		SMSC_NOTICE(1,("cannot open socket"));
		return 1;
	}
  
	/* bind to any local port */
  	if(bind(test->sd, (struct sockaddr *) &test->myAddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
	    SMSC_NOTICE(1, ("cannot bind port"));
	    return 1;
	}

	if (!test->udp) {
		if (test->sflag) {
			if (connect(test->sd, (struct sockaddr *) &test->otherAddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR ) {
				SMSC_NOTICE(1, ("connect error.."));
				return 1;
			}
		}
		else {
			listen(test->sd, 5);

			/* To exercise select */
			FD_ZERO (&read_fd);
			FD_SET (test->sd, &read_fd);
			if ((select(FD_SETSIZE, &read_fd, NULL, NULL, NULL)) == SOCKET_ERROR) {
				SMSC_NOTICE(1, ("select error..."));
				return 1;
	}
			otherAddrLen = sizeof(struct sockaddr_in);
			if ((test->nsd = accept(test->sd, (struct sockaddr *) &test->otherAddr, &otherAddrLen)) == INVALID_SOCKET ) {
				SMSC_NOTICE(1, ("accept error..."));
				return 1;
			}
#ifdef WIN32
			closesocket(test->sd);
#else
			close(test->sd);
#endif
		}
	}
	/* send data */
	if (test->sflag) {

		if ((msgbuf = malloc((unsigned int)test->sndMsgLen)) == NULL)
		{
			SMSC_NOTICE(1, ("malloc error.."));
			return 1;
		}

#if defined (UNIX) || defined (WIN32)
		/* install signal handler */
		signal(SIGINT, (void *)catch_signal);
#endif

		SMSC_TRACE(1,("Send the START marker...")); 
		
		/* Send start marker */
		if (msgsend(test, START_MARKER, strlen(START_MARKER)) < 0)
		{
			SMSC_NOTICE(1, ("send data failure.."));
			return 1;
		}	

		/* Record start time */
		read_start_time(test);	

		do {
			memset(msgbuf, 0, test->sndMsgLen);
        
			if(!test->skipVerify) {
				/* Fill the sequential data to the buffer */
				for (i = 0; i < test->sndMsgLen; i++)
				{
					msgbuf[i] = test->dataCount++;
				}
				if (test->udp)
					test->dataCount = 0;
			}
			
			rc = msgsend(test, msgbuf, test->sndMsgLen);
			if(rc < 0) 
			{
				SMSC_NOTICE(1,("send data failure.."));
				exit(1);
			}

			test->msgCount++;
			test->byteCount += rc;
			if (test->count)
				if (!(--test->count))
					break;
			bCount++;
			if (test->burstTime && bCount == test->burstCount) {
				bCount = 0;
#ifdef WIN32
				Sleep(test->burstTime);
#else
#ifdef UNIX
				usleep(test->burstTime*1000);
#else
				smsc_mdelay(test->burstTime);
#endif
#endif				
			}				
				
		} while (1);

		/* Record end time */
		read_end_time(test);

		SMSC_TRACE(1,("Data send done..."));
        
		end_of_transmission(test);
	}

	/* receive data */
	if (!test->sflag)
	{
		if ((msgbuf = malloc(MAX_RECV_MSG_SIZE)) == NULL)
		{
			SMSC_NOTICE(1, ("malloc error.."));
			return 1;
		}
		otherAddrLen = sizeof(test->otherAddr);
#if defined (UNIX) || defined (WIN32)
		/* install signal handler */
		signal(SIGINT, (void *)catch_signal);
#endif
		/* Record the start time */
		read_start_time(test);	
		
		/* receive message */
		do {
			memset(msgbuf, 0, MAX_RECV_MSG_SIZE);

			if ((rc = msgrecv(test, msgbuf, MAX_RECV_MSG_SIZE)) < 0) {
				SMSC_NOTICE(1, ("msgrecv data failure.."));
				return 1;
			}

			if (strcmp((char *)msgbuf, START_MARKER) == 0) {	
				SMSC_TRACE(1, ("Got the START marker .."));

				/* Restart the record start time */
				read_start_time(test);	

				/* discard the extra bytes read */ 
				test->dataCount += (rc - strlen(START_MARKER));
				continue;
			}

			if (strcmp((char *)msgbuf, END_MARKER) == 0) {	
				SMSC_TRACE(1,("Got the END marker .."));

				/* record end time */
				read_end_time(test);

				stat.msgReceived = test->msgCount;
				stat.secRecvTime = test->realTime;

				SMSC_TRACE(1,("stat.msgReceived=%d, stat.secRecvTime=%lu",stat.msgReceived, (unsigned long)stat.secRecvTime));
				
				/* Send no. of bytes received*/
				if (msgsend(test, (char *)&stat.msgReceived, sizeof(int)) < 0) {
					SMSC_NOTICE(1, ("send data failure.."));
					return 1;
				}	
				if (msgsend(test, (char *)&stat.secRecvTime, sizeof(double)) < 0) {
					SMSC_NOTICE(1, ("send data failure.."));
					return 1;
				}	
				break;
			}
				
			if (!test->skipVerify) {
				/* Verify integrity of the data */
				if (verify_data(test, msgbuf, rc) != 0)
					return 1;
			}
			test->msgCount++;
			test->byteCount += rc;
			/* if (test->count)
				if (!(--test->count))
					break;
			*/
		} while (1);

		SMSC_TRACE(1, ("%d bytes received in %d messages", test->byteCount, test->msgCount));
	}

#ifdef WIN32
	closesocket(test->nsd);
#else
	close(test->nsd);
#endif

	return 0;

}

int msgsend(struct tester *test, char *msgbuf, unsigned int msglen)
{
	int count;
	if (test->udp) {
#ifdef UNIX
again:
#endif
		if ((count = sendto(test->sd, msgbuf, msglen, 0, 
			(struct sockaddr *) &test->otherAddr, sizeof(struct sockaddr_in))) < 0) {
#ifdef UNIX
			if (errno == ENOBUFS) {
				SMSC_NOTICE(1, ("send to error.."));
				usleep(10000);
				errno = 0;
				goto again;
			}
#endif
			SMSC_NOTICE(1, ("send to error.."));
			return -1;
		}
	}
	else {
		if ((count = send(test->sflag ? test->sd : test->nsd, msgbuf, msglen, 0)) < 0) {
			SMSC_NOTICE(1, ("send error.."));
			return -1;
		}
	}
	return count;
}

int msgrecv(struct tester *test, char *msgbuf, unsigned int msglen)
{                                                
	int count; 
	int otherAddrLen = sizeof(test->otherAddr);
	if (test->udp)
	{
		if ((count = recvfrom(test->sd, (void *)msgbuf, msglen, 0,
			(struct sockaddr *) &test->otherAddr, (int *)&otherAddrLen)) < 0) {
			SMSC_NOTICE(1, ("recevfrom error.."));
			return -1;
		}
	}	
	else {
		if ((count = recv(test->sflag ? test->sd : test->nsd, msgbuf, msglen, 0)) < 0) {
			SMSC_NOTICE(1,("read error.."));
			return -1;
		}
	}
	return count;
}

void read_start_time(struct tester *test)
{
	SMSC_TRACE(1,("Recording start time.."));
#ifdef WIN32
	test->startTime = GetTickCount();
#else
#ifdef UNIX
	gettimeofday(&test->startTime, (struct timezone *)0);
#else
	test->startTime = smsc_time_now();
#endif
#endif
}

void read_end_time(struct tester *test)
{
#ifdef WIN32
	double endtime;
	endtime = GetTickCount();
	test->realTime = ((double) endtime - (double) test->startTime)/1000;
#else
#ifdef UNIX
	struct timeval endtime, tdiff;
	gettimeofday(&endtime, (struct timezone *)0);		
	tvsub(&tdiff, &endtime, &test->startTime);
	test->realTime = tdiff.tv_sec + ((double)tdiff.tv_usec) / 1000000;
#else
	/*test->realTime = (double) time_minus(smsc_time_now(),test->startTime)/time_ticks_per_sec();*/
	test->realTime = (double) smsc_tick_to_sec(smsc_time_minus(smsc_time_now(), test->startTime));
#endif
#endif	
	SMSC_TRACE(1,("Record END time"));
}

#ifdef UNIX
void tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0)
{
	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}
#endif

void end_of_transmission(struct tester *test)
{
	int count, otherAddrLen;
	struct stat stat;

	/* Slow server may be still receiving data..wait..*/
	SMSC_TRACE(1,("Waiting for 4 sec before sending end marker"));
#ifdef WIN32
	Sleep(4000);
#else
#ifdef UNIX
	sleep(4);
#else
	smsc_mdelay(4000);
#endif
#endif
	memset(&stat, 0, sizeof(stat));

	SMSC_TRACE(1,("Sending END_MARKER"));

	/* Send end marker */
	if (msgsend(test, END_MARKER, strlen(END_MARKER)) < 0) {
		SMSC_NOTICE(1, ("send data failure.."));
		return;
	}	

	SMSC_TRACE(1,("Waiting for stat response..."));

	/* Receive response */	
	otherAddrLen = sizeof(test->otherAddr);
	if (test->udp) {
		if ((count = recvfrom(test->sd, (char *)&stat, sizeof(stat), 0,
			(struct sockaddr *) &test->otherAddr, &otherAddrLen)) < 0) {
			SMSC_NOTICE(1, ("recevfrom error.."));
			return;
		}
	}
	else {
		/* For TCP */
		if ((count = recv(test->sd, (char *)&stat.msgReceived, sizeof(int), 0)) < 0) {
			SMSC_NOTICE(1, ("read error.."));
			return;
		}
		if ((count = recv(test->sd, (char *)&stat.secRecvTime, sizeof(double), 0)) < 0) {
			SMSC_NOTICE(1, ("read error.."));
			return;
		}
	}
   	SMSC_TRACE(1, ("Msg sent:%u   Msg received:%u Time to receive:=%d", test->msgCount, stat.msgReceived, (int)(test->realTime)));
   	/* For throughput measurement, use the msgReceived count from other host and use the local time. */ 
	SMSC_TRACE(1, ("Throughput: %d Kbps", (int)(stat.msgReceived * (double)test->sndMsgLen * 8.0/test->realTime/1000.0)));
}

int verify_data(struct tester *test, char *msgbuf, int count)
{
	int i;
	if (test->udp) 
		test->dataCount = 0;
	/*SMSC_TRACE(1,("msgbuf=%d dataCount=%d count=%d\n", msgbuf[0], test->dataCount, count));
	*/
	if(count <= 0) {
		SMSC_NOTICE(1, ("verify_data: count passed is 0"));
		return -1;
	}
	for (i = 0; i < count; i++, test->dataCount++)
	{
		if (msgbuf[i] != test->dataCount)
		{
			SMSC_NOTICE(1, ("Invalid Data, Expecting Incrementing pattern"));
			return -1;	
		}
	}
	return 0;
}
struct tester chgSocketTesterData1;
 struct tester chgSocketTesterData2;
 void  ch_testsocket()
{
socket_test_initiate(
		&chgSocketTesterData2,
		0, 				/* 1 for udp test, 0 for tcp */
		1, 				/* 1 if sender(client), 0 for receiver(server) */
		"192.168.1.12",	/* Destination address, used when sender (sflag=1) */ 
		6000, 			/* Destination port, used when sender */
		4000,			/* Local port to bind, used when receiver (sflag=0) */
		0,				/* No. of message to send, used when sender, 0 for continuous transmit */ 
		1472,			/* Length of the message to send, used when sender. if 0, default 64 bytes */
		1,				/* 1 to skip the data verification */
		10,				/* burst time,  time to pause in ms after each burst. 0 for no pause/burst */
		10				/* burst count, no. of messages to send in each burst. default 10. ignored if burst time 0*/
		);
}
#endif
/*eof*/
