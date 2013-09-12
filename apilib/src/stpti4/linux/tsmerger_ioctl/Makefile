# PJS 270705 Changed to just use KDIR

HEADER_IMPORTS += stcommon stos stpti4 stpti4/test/utils stpti4/src/stpti


include $(DVD_MAKE)/kbuild.mak


obj-m           := tsmerger_ioctl.o
tsmerger_ioctl-objs := tsmerger_ioctl_main.o \
                       tsmerger_ioctl_open.o \
                       tsmerger_ioctl_ioctl.o \
                       tsmerger_ioctl_proc.o

ifneq ($(DVD_FRONTEND),7200)
  tsmerger_ioctl-objs += ../../test/utils/tsmerger.o
endif

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS) -USTPTI_FDMA_SUPPORT

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

# External makefile
PWD := $(shell pwd)

all: default libtsmerger.a

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules


libtsmerger.a: tsmerger_ioctl_lib.o
	$(BUILD_LIBRARY)


# Some version of tidying up...
clean:
	$(RM) ../../test/utils/tsmerger.o ../../test/utils/.tsmerger.o.cmd
	$(MAKE) -C $(KDIR) M=$(PWD) clean


endif
