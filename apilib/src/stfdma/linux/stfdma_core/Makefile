# COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

HEADER_IMPORTS += stos stcommon stfdma

include $(DVD_MAKE)/kbuild.mak

#Add your STAPI objects below here
STAPI_SRC_OBJS      := ../../src/stfdma.o \
                       ../../src/handler.o \
                       ../../src/fdmareq.o \
                       ../../src/fdma1.o \
                       ../../src/fdma2.o \
                       ../../src/context.o \

obj-m               := stfdma_core.o
stfdma_core-objs    := $(STAPI_SRC_OBJS) \
                       stfdma_core_main.o \
        		       stfdma_core_ioctl.o \
        		       stfdma_core_evts.o \
        		       stfdma_core_open.o \
        		       stfdma_core_ops.o \
    	               stfdma_core_proc.o \
        		       stfdma_core_read.o

ifdef TEST_HARNESS
CFLAGS += -DTEST_HARNESS
endif

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
	$(RM) $(STAPI_SRC_OBJS)
	$(RM) $(foreach FILE,$(STAPI_SRC_OBJS),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) V=1 clean

endif
