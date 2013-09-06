#Unix Makefile for Linux loop back demo stack
PROGRAM=stack
SOURCE=../../Source

INTERFACE_FILES=$(SOURCE)/interfaces/loop_back.c

UTILITIES_FILES=$(SOURCE)/utilities/memory_pool.c  $(SOURCE)/utilities/network_stack.c $(SOURCE)/utilities/packet_manager.c $(SOURCE)/utilities/task_manager.c 

PROTOCOLS_FILES=$(SOURCE)/protocols/icmpv4.c $(SOURCE)/protocols/tcp.c $(SOURCE)/protocols/udp.c $(SOURCE)/protocols/ipv4.c	

APPLICATIONS_FILES=$(SOURCE)/applications/icmp_echo_tester.c $(SOURCE)/applications/udp_tester.c $(SOURCE)/applications/tcp_tester.c

PLATFORM_FILES=main.c smsc_environment.c smsc_threading.c

ALL_SOURCES=$(INTERFACE_FILES) $(PROTOCOLS_FILES) $(APPLICATIONS_FILES) $(PLATFORM_FILES) $(UTILITIES_FILES)

#ALL_OBJECTS=$(patsubst %.c,%.o,$(ALL_SOURCES))

INCLUDES=-I./ -I../../Source 
CFLAGS=-DLINUX_LOOPBACK_DEMO
LIBS=-pthread -lrt
CC=gcc

$(PROGRAM): *.o
	$(CC) -o $(PROGRAM) $(INCLUDES) $(LIBS) $(CFLAGS) $^

*.o : $(ALL_SOURCES)
	$(CC) -c $(CFLAGS) $(INCLUDES) $^
clean:
	rm -f *.o $(PROGRAM)
