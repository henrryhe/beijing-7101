# COPYRIGHT (C) STMicroelectronics 2006.  All rights reserved.

HEADER_IMPORTS += stos stcommon stfdma
HEADER_IMPORTS += stpti4 stpti4/linux stpti4/src stpti4/linux/stpti4_core
HEADER_IMPORTS += stpti4/src/stpti stpti4/src/stpti/pti4 stpti4/src/stpti/memapi
HEADER_IMPORTS += stpti4/src/stpti/pti4/tc

include $(DVD_MAKE)/kbuild.mak


obj-m           := stpti4_ioctl.o
stpti4_ioctl-objs := stpti4_ioctl_main.o \
		   stpti4_ioctl_open.o \
		   stpti4_ioctl_ops.o \
		   stpti4_ioctl_utils.o \
		   stpti4_ioctl_ioctl.o \
		   stpti4_ioctl_test.o


# Add the TCLOADERS to stpti4_ioctl-objs
# If the user has not specified which TCLOADERS sources to use, then work them out...
ifndef STPTI_TCLOADER_SRCS

# Actively gather the loaders present in the ../../src/tcloaders/ directory
# This mirrors a section in ../../src/tcloaders/makefile.  You should make sure these remain in sync!
ifeq ($(findstring STPTI_DVB_SUPPORT,$(STPTI_CONFIG)), STPTI_DVB_SUPPORT)
    # DVB loaders are all the DVB_Ldr*.c without the NDS ones DVB_LdrNDS*.c
    STPTI_TCLOADER_SRCS += $(filter-out $(wildcard ../../src/tcloaders/DVB_LdrNDS*.c),$(wildcard ../../src/tcloaders/DVB_Ldr*.c))
    ifeq ($(findstring STPTI_NDS_SUPPORT,$(STPTI_CONFIG)), STPTI_NDS_SUPPORT)
        # DVB NDS loaders are all the DVB_LdrNDS*.c
        STPTI_TCLOADER_SRCS += $(wildcard ../../src/tcloaders/DVB_LdrNDS*.c)
    endif
endif
ifeq ($(findstring STPTI_DTV_SUPPORT,$(STPTI_CONFIG)), STPTI_DTV_SUPPORT)
    # DTV loaders are all DTV_Ldr*.c and COMBI_Ldr*.c
    STPTI_TCLOADER_SRCS += $(wildcard ../../src/tcloaders/DTV_Ldr*.c)
    STPTI_TCLOADER_SRCS += $(wildcard ../../src/tcloaders/COMBI_Ldr*.c)
endif

endif

stpti4_ioctl-objs += $(patsubst %.c,%.o,$(STPTI_TCLOADER_SRCS))


EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
EXTRA_CFLAGS += -DDVD_EXCLUDE_OS20TO21_MACROS
EXTRA_CFLAGS += -DSTSYS_DEVICE_ACCESS
EXTRA_CFLAGS += -DLINUX_FULL_KERNEL_VERSION
EXTRA_CFLAGS += $(STPTI_CONFIG)

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The enviroment variable KDIR must be set)
endif

# External makefile
PWD     := $(shell pwd)

all: default libstpti4.a

default: 
	# We must defined STPTI_TCLOADER_SRCS as wildcard searching does not work when the kernel reuses this
    # makefile for building the module
	$(MAKE) -C $(KDIR) M=$(PWD) STPTI_TCLOADER_SRCS="$(STPTI_TCLOADER_SRCS)" modules

libstpti4.a: stpti4_ioctl_lib.o
	$(BUILD_LIBRARY)

loaders:
	$(MAKE) -C $(SRC_BASE)/src/tccode
	echo Creating stpti.h
	$(MAKE) -C $(SRC_BASE)/src/tccode tcproto.h
	cat ../../src/stpti/stptiint.h  > ../../stpti.h
	cat ../../src/tccode/tcproto.h >> ../../stpti.h

# Some version of tidying up...
clean:
	$(RM) $(DVB_TCCODE_OBJS) $(DTV_TCCODE_OBJS)
	$(RM) $(foreach FILE,$(patsubst %.c,%.o,$(STPTI_TCLOADER_SRCS)),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(RM) $(foreach FILE,$(patsubst %.c,%.o,$(STPTI_TCLOADER_SRCS)),$(dir $(FILE))$(notdir $(FILE)))
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif
