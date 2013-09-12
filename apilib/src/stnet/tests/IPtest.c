/*	IPtest - Remote IP protocol tester
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <assert.h>

#include "rtp.h"


#define PGM_NAME	"IPtest"

#define PROTO_TS	1
#define PROTO_RTP	2

#define	TS_BUFSIZE	(7*188)
#define RTP_BUFSIZE	1500		/* ETH_DATA_LEN */

#define DFLT_PROTOCOL	PROTO_TS
#define DFLT_IP		INADDR_LOOPBACK
#define DFLT_PORT	6000
#define DFLT_DELAY	10

static struct {
	char		protocol;
	unsigned long	ipaddr;
	int		port;
	int		count;
	int		delay;
	char		*file;
} option = {
	DFLT_PROTOCOL,
	DFLT_IP,
	DFLT_PORT,
	0,
	DFLT_DELAY,
	NULL
};

/******************************************************************************
Name         : 

Description  : 

Parameters   : 

Return Value : 
******************************************************************************/

void dump_pid(int fd)
{
	struct {
		unsigned	i1:1;
		unsigned	i2:1;
		unsigned	i3:1;
		unsigned	pid_hi:5;
		unsigned char	sync;

		unsigned	pid_lo:8;
		unsigned	i4:8;
	} ts_header;
	int	len;

	lseek(fd, 0, SEEK_SET);
	len = read(fd, &ts_header, sizeof(ts_header));
	if (len) {
		printf("PID:\t\t%d\n", ts_header.pid_lo); /*ts_header.pid_hi<<8 |*/
	}
	lseek(fd, 0, SEEK_SET);
}

void print_helpmsg()
{
	printf("Usage: %s [options] [TSfile]\n\n"
"%s can be used to send single messages or stream a complete file to a\n"
"specific UDP port on the network.\n\n"
"OPTIONS\n\n"
"-r\tUse RTP protocol. Default is to send Transport Stream data.\n"
"-i\tIP address. Only IPv4 is supported. Default is the loopback address.\n"
"-p\tUDP port.\n"
"-c\tRepeat count. Indicates either the number of messages or complete\n"
"\tfiles that are sent.\n"
"-d\tDelay between each message sent.\n"
"TSfile\tFile with Transport Stream data. When no file is given dummy\n"
"\tpackages will be sent. With the RTP protocol, this data is streamed.\n"
"\n", PGM_NAME, PGM_NAME);

}

int parse_cmdline(int argc, char *argv[])
{
	struct in_addr	in;
	int	rc = 0;

/*	for(rc=0; rc<argc; rc++)
		printf("%d: %s\t", rc, argv[rc]);
*/
	while( (rc = getopt(argc, argv, "ri:p:c:d:")) != -1) {
		switch(rc) {
		case 'r':
			option.protocol = PROTO_RTP;
			break;
		case 'p':
			option.port = atoi(optarg);
			break;
		case 'c':
			option.count = atoi(optarg);
			break;
		case 'd':
			option.delay = atoi(optarg);
			break;
		case 'i':
			rc = inet_aton(optarg, &in);
			if (rc == 0) {
				printf("Invalid IP address: %s\n", optarg);
				rc = -2;
			}
			option.ipaddr = in.s_addr;
			break;
		default:
			print_helpmsg();
			rc = -2;
			break;
		}
		if (rc == -2) break;
	}

	if (optind < argc) {
		option.file = argv[optind];
	}

	printf("Protocol:\t%s\nIP address:\t%lx\nPort:\t\t%d\nCount:\t\t%d\n",
		(option.protocol == PROTO_RTP? "RTP": "TS"),
		option.ipaddr, option.port, option.count);
	printf("Delay:\t\t%dms\nFile:\t\t%s\n", option.delay,
		(option.file? option.file: "None"));

	if (rc == -1) rc = 0;
	return(rc);
}

int read_command()
{
	char	line[80];
	int	count;

	if (option.file)
		printf("Files");
	else	printf("Packets");
	printf(" to send (0 to stop): ");
	fgets(line, 80, stdin);
	count = atoi(line);
	return(count);
}

void set_realtime(void)
{
	struct sched_param	param;
	int			rc;

	param.sched_priority = 99;
	rc = sched_setscheduler(0, SCHED_FIFO, &param);
	if (rc != 0) { perror("sched_getparam"); }
}

void htona(void *dest, void *src, int len)
{
	unsigned long	*d, *s;
	int	i;

	assert(len%4==0);
	assert((unsigned long)dest%4==0);
	assert((unsigned long)src%4==0);
	d = (unsigned long *)dest;
	s = (unsigned long *)src;

	for (i=0; i<len/4; i++) {
		d[i] = htonl(s[i]);
	}
}


/*****************************************************************************
 *	TS data
 *****************************************************************************/
void inject_TSmsg(int sfd, struct sockaddr_in *to)
{
	char	buf[TS_BUFSIZE];
	int	rc;

	bzero(buf,TS_BUFSIZE);

	rc = sendto(sfd, buf, TS_BUFSIZE, 0, (struct sockaddr *)to, sizeof(*to));
	if (rc < 0) { perror("send"); exit(rc); }
	printf("%s send %d bytes.\n",__FUNCTION__, rc);

	usleep(option.delay);
}

void inject_TSfile(int fd, int sfd, struct sockaddr_in *to)
{
	char	buf[TS_BUFSIZE];
	int	len, rc;

	lseek(fd, 0, SEEK_SET);
	while ((len = read(fd, buf, TS_BUFSIZE) != 0)) {
		if (len < 0) { perror("read"); exit(-1); }

		rc = sendto(sfd, buf, len, 0, (struct sockaddr *)to, sizeof(*to));
		printf("Read %d bytes. Send: %d\n", len, rc);
		if (rc < 0) { perror("send"); exit(-1); }
		usleep(option.delay);
	}
}

/*****************************************************************************
 *	RTP data
 *****************************************************************************/

int inject_RTPmsg(int fd, int sfd, struct sockaddr_in *to)
{
	char		buf[RTP_BUFSIZE];
	RTP_Packet_t	data;
	int		bufsize, i, len;
	static int	rtpsize = 0;
	static char	first = 1;

	if (first) {
		rtpsize = sizeof(data.hdr);
		data.hdr.Version = 2;
		data.hdr.Padding = 0;
		data.hdr.Extension = 0;
		data.hdr.CSRCCount = 0;
		data.hdr.Marker = 0;
		data.hdr.PayloadType = 33;
		data.hdr.SequenceNumber = random() & 0xffff;
		data.hdr.SSRC = getpid();
		first = 0;
	} else {
		data.hdr.SequenceNumber++;
	}
	data.hdr.TimeStamp = time(NULL);

	htona(buf, &data, rtpsize);	/* Swap the data */
	bufsize = rtpsize;
	/* How much TS data can fit? */
	i = ((RTP_BUFSIZE - bufsize)/188)*188;
	if (fd) {
		len = read(fd, &buf[bufsize], i);
		if (len < 0) { perror("read"); exit(-1); }
		bufsize += len;
	} else {
		bzero(&buf[bufsize], i);
		bufsize += i;
		len = i;
	}

	i = sendto(sfd, buf, bufsize, 0, (struct sockaddr *)to, sizeof(*to));
	if (i < 0) { perror("send"); exit(i); }

	usleep(option.delay);
	return(len);
}

void inject_RTPfile(int fd, int sfd, struct sockaddr_in *to)
{
	int	len;

	lseek(fd, 0, SEEK_SET);
	while((len = inject_RTPmsg(fd, sfd, to)) != 0);
}

/*****************************************************************************
 *	main program
 *****************************************************************************/
int main(int argc, char *argv[])
{
	struct sockaddr_in	to;
	unsigned	sfd;
	int		rc, fd, c;

	rc = parse_cmdline(argc, argv);
	if (rc != 0) exit(rc);

	if (option.file) {
		fd = open(option.file, O_RDONLY);
		if (fd < 0) { perror("open"); exit(-1); }
//		dump_pid(fd);
	} else	fd = 0;

	sfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sfd < 0) { perror("socket"); exit(-1); }

	to.sin_family = AF_INET;
	to.sin_port = htons(option.port);
	to.sin_addr.s_addr = htonl(option.ipaddr);

	set_realtime();

	if (option.count == 0)
		c = read_command();
	else	c = option.count;

	while (c != 0) {
	    switch(option.protocol) {
	    case PROTO_TS:
		if (fd)
			inject_TSfile(fd, sfd, &to);
		else
			inject_TSmsg(sfd, &to);
		break;
	    case PROTO_RTP:
		if (fd)
			inject_RTPfile(fd, sfd, &to);
		else
			inject_RTPmsg(fd, sfd, &to);
		break;
	    default:
		break;
	    }
	    c--;
	    if (!c && !option.count)
		c = read_command();
	}

	rc = close(sfd);
	if (fd) rc |= close(fd);
	return(rc);
}
