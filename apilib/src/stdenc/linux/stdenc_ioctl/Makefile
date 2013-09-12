# KBUILD Makefile for stdenc_ioctl

HEADER_IMPORTS += stdenc/linux stdenc stdenc/src stos stcommon

include $(DVD_MAKE)/kbuild.mak

obj-m             := stdenc_ioctl.o
stdenc_ioctl-objs := stdenc_ioctl_main.o \
		     stdenc_ioctl_open.o \
		     stdenc_ioctl_read.o \
                     stdenc_ioctl_ioctl.o

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)


ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The environment variable KDIR must be set)
endif

# External makefile
PWD     := $(shell pwd)

all: default libstdenc.a

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules


libstdenc.a: stdenc_ioctl_lib.o
	$(BUILD_LIBRARY)

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stdenc_ioctl-objs)
	$(RM) $(foreach FILE,$(stdenc_ioctl-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean


endif
