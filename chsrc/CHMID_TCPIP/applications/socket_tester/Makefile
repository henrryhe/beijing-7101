# Unix Makefile for socktest.c 

PROGRAM = socktest
CFLAGS = -DUNIX
OBJTEXT = o

SOURCE = getopt.c socktest.c

.c.$(OBJEXT):
	$(CC) -c $(CFLAGS) $<

all: $(PROGRAM)

clean:
	rm -f *.$(OBJTEXT) $(PROGRAM)

$(PROGRAM):		$(SOURCE)

