# KBUILD Makefile for stlayer_ioctl

HEADER_IMPORTS += stlayer/linux stlayer stlayer/src stos stcommon

include $(DVD_MAKE)/kbuild.mak

# Work out what stcommon's name is (for working in and outside of the VOB)
# This is likely to be removed when stcommon is turned into a .ko
ifneq "$(wildcard $(M)/../../../dvdbr-prj-stcommon)" ""
  STCOMMON_NAME := dvdbr-prj-stcommon
else
  STCOMMON_NAME := stcommon
endif

obj-m              := stlayer_ioctl.o
stlayer_ioctl-objs := stlayer_ioctl_main.o \
		      stlayer_ioctl_open.o \
                      stlayer_ioctl_ioctl.o

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

all: default libstlayer.a

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules


libstlayer.a: stlayer_ioctl_lib.o
	$(BUILD_LIBRARY)

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stlayer_ioctl-objs)
	$(RM) $(foreach FILE,$(stlayer_ioctl-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean


endif
