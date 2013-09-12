# KBUILD Makefile for stdenc_ioctl

HEADER_IMPORTS += stvmix/linux stvmix stvmix/src stos stcommon

include $(DVD_MAKE)/kbuild.mak


obj-m           := stvmix_ioctl.o
stvmix_ioctl-objs := stvmix_ioctl_main.o \
		   stvmix_ioctl_open.o \
		   stvmix_ioctl_read.o \
		   stvmix_ioctl_ioctl.o \
           stvmix_ioctl_exports.o


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

all: default libstvmix.a

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules


libstvmix.a: stvmix_ioctl_lib.o
	$(BUILD_LIBRARY)

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stvmix_ioctl-objs)
	$(RM) $(foreach FILE,$(stvmix_ioctl-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean




endif
