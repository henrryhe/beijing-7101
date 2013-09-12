# KBUILD Makefile for stvout_ioctl

HEADER_IMPORTS += stvout/linux stvout stvout/src stvout/linux/stvout_core stos stcommon

include $(DVD_MAKE)/kbuild.mak

obj-m             := stvout_ioctl.o
stvout_ioctl-objs := stvout_ioctl_main.o \
                     stvout_ioctl_open.o \
                     stvout_ioctl_read.o \
                     stvout_ioctl_ioctl.o



EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)


ifeq "$(STVOUT_CEC_CAPTURE)" "TRUE"
        EXTRA_CFLAGS :=$(EXTRA_CFLAGS)  -DSTVOUT_CEC_PIO_COMPARE -DSTVOUT_CEC_CAPTURE
else
ifeq "$(STVOUT_CEC_PIO_COMPARE)" "TRUE"
        EXTRA_CFLAGS :=$(EXTRA_CFLAGS)  -DSTVOUT_CEC_PIO_COMPARE
endif
endif



ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
     EXTRA_CFLAGS := $(EXTRA_CFLAGS) -DSTVOUT_HDCP_PROTECTION
endif


ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The environment variable KDIR must be set)
endif

# External makefile
PWD     := $(shell pwd)

all: default libstvout.a

default:

	$(MAKE) -C $(KDIR) M=$(PWD)  modules

libstvout.a: stvout_ioctl_lib.o
	$(BUILD_LIBRARY)

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest

clean:
	$(RM) $(stvout_ioctl-objs)
	$(RM) $(foreach FILE,$(stvout_ioctl-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif

