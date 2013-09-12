# KBUILD Makefile for stvtg_ioctl

HEADER_IMPORTS += stvtg/linux stvtg stvtg/src stos stcommon

include $(DVD_MAKE)/kbuild.mak

obj-m             := stvtg_ioctl.o
stvtg_ioctl-objs := stvtg_ioctl_main.o \
                     stvtg_ioctl_open.o \
                     stvtg_ioctl_read.o \
                     stvtg_ioctl_ioctl.o

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

all: default libstvtg.a

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules


libstvtg.a: stvtg_ioctl_lib.o
	$(BUILD_LIBRARY)

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stvtg_ioctl-objs)
	$(RM) $(foreach FILE,$(stvtg_ioctl-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean


endif
