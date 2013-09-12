# PJS 270705 Changed to just use KDIR

# Header inports go here
HEADER_IMPORTS += stos stcommon staudlx/linux staudlx staudlx/src stpti4 stclkrv

include $(DVD_MAKE)/kbuild.mak 

obj-m           := staudlx_ioctl.o
staudlx_ioctl-objs := staudlx_ioctl_main.o \
		   staudlx_ioctl_open.o \
		   staudlx_ioctl_read.o \
		   staudlx_ioctl_ioctl.o \
		   staudlx_ioctl_ops.o \
		   staudlx_ioctl_evts.o \
		   staudlx_ioctl_proc.o

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
EXTRA_CFLAGS += $(DVD_LINUX_CFLAGS)
EXTRA_CFLAGS += -DST_OSLINUX -DST_$(DVD_FRONTEND) -D$(DVD_PLATFORM) -DST_LINUX

ifdef STAUD_REMOVE_PTI_SUPPORT
	CFLAGS := $(CFLAGS) -DSTAUD_REMOVE_PTI_SUPPORT
endif 

# The following checks to see if we have been invoked in the kbuild
# (KERNELRELEASE will be defined). If not we have the means of launching
# the KBUILD (all and default targets).

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

# External makefile
PWD     := $(shell pwd)

all: default libstaudlx.a

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules

libstaudlx.a: staudlx_ioctl_lib.o
	$(BUILD_LIBRARY)

# Some version of tidying up...
clean:  
#	$(RM) $(staudlx_ioctl-objs) $(obj-m) staudlx_ioctl.ko
#	$(RM) staudlx_ioctl_lib.o libstaudlx_ioctl.a 

	$(RM) $(staudlx_ioctl-objs)
	$(RM) Modules.symvers
	$(RM) $(foreach FILE,$(staudlx_ioctl-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) libstaudlx.a
	$(RM) staudlx_ioctl_lib.o
endif

