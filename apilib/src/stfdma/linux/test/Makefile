# COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

HEADER_IMPORTS += stos stcommon stfdma

include $(DVD_MAKE)/kbuild.mak

#Add your STAPI objects below here
STAPI_TESTS_OBJS     := ../../tests/fdmatst.o \
                        ../../tests/func.o \
                        ../../tests/state.o \
                        ../../tests/param.o

obj-m                :=	test.o
test-objs    	     :=	$(STAPI_TESTS_OBJS) \
        		       	fdmatest_main.o \
        		       	fdmatest_open.o \
        	    		fdmatest_read.o \
            			fdmatest_ioctl.o \
            			fdmatest_ops.o \
            			fdmatest_evts.o \
            			fdmatest_proc.o

#ifdef TEST_HARNESS
#CFLAGS += -DTEST_HARNESS
#endif

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
#EXTRA_CFLAGS += PUT_EXTRA_CFLAGS_HERE

ifdef STFDMA_NO_PACED_TESTS
EXTRA_CFLAGS += -DSTFDMA_NO_PACED_TESTS
endif

# The following checks to see if we have been invoked in the kbuild
# (KERNELRELEASE will be defined). If not we have the means of launching
# the KBUILD (all and default targets).

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The enviroment variable KDIR must be set)
endif

PWD := $(shell pwd)

all: default

default:
	$(MAKE) -C $(KDIR) M=$(PWD) V=1 modules

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(STAPI_TESTS_OBJS)
	$(RM) $(foreach FILE,$(STAPI_TESTS_OBJS),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) V=1 clean

endif
