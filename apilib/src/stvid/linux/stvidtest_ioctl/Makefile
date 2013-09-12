# KBUILD Makefile for stvid_ioctl

HEADER_IMPORTS += stcommon stos stapigat/src stvid/tests stvid/tests/src stpti4

#TARGET_BASE := $(DVD_ROOT)/dvdgr-prj-stvid/src/linux/objs
#SRC_BASE := $(DVD_ROOT)/dvdgr-prj-stvid
#STCOMMON_BASE := $(DVD_ROOT)/dvdbr-prj-stcommon
#BOARD_BASE := $(DVD_ROOT)/dvdbr-prj-board
#CHIP_BASE := $(DVD_ROOT)/dvdbr-prj-chip
#STAPIGAT_BASE := $(DVD_ROOT)/dvdgr-prj-stapigat
#PTI_BASE := $(DVD_ROOT)/dvdbr-prj-stpti4
#
#INCLUDE_BASE := $(DVD_ROOT)/dvdbr-prj-include

include $(DVD_MAKE)/kbuild.mak

obj-m                := stvidtest_ioctl.o
stvidtest_ioctl-objs := stvidtest_ioctl_main.o \
                        stvidtest_ioctl_open.o \
                        stvidtest_ioctl_ioctl.o

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)

#ifeq "$(DEBUG)" "1" 
#EXTRA_CFLAGS += -O0 -g -fno-omit-frame-pointer
#EXTRA_CFLAGS += $(MOD_WARN)
#else
#EXTRA_CFLAGS += -O2
#endif

#EXTRA_CFLAGS += -I$(PTI_BASE) 
#EXTRA_CFLAGS += -I$(INCLUDE_BASE) -I$(STCOMMON_BASE) -I$(BOARD_BASE) -I$(CHIP_BASE) -I$(STAPIGAT_BASE)/src
#EXTRA_CFLAGS += -I$(SRC_BASE)/linux/stvid_ioctl -I$(SRC_BASE)/.. -I$(SRC_BASE)/tests -I$(SRC_BASE)/tests/src -I$(SRC_BASE)/tests/src/stvidtest_ioctl 

#EXTRA_CFLAGS += -D$(LINUX_ARCHITECTURE)
#ifdef KERNEL_MODE
#EXTRA_CFLAGS += -DMODULE
#endif

#EXTRA_CFLAGS += -DLINUX_OS -DST_OSLINUX -DLINUX_FULL_KERNEL_VERSION -DAVMEM_KERNEL_MODE
#EXTRA_CFLAGS += -DSTTBX_PRINT -DUSE_LMI_SYS_FOR_LINUX -DSTVID_DEBUG_GET_STATISTICS -DGLOBAL_PASS_PARAMS_THROUGH_IOCTLS

# From generic.mak
EXTRA_CFLAGS += -D$(DVD_PLATFORM)
EXTRA_CFLAGS += -DST_$(DVD_FRONTEND)

EXTRA_CFLAGS += $(DVD_CFLAGS)
EXTRA_CFLAGS += $(DVD_LINUX_CFLAGS)
EXTRA_CFLAGS += $(INCLUDES) $(BASE_INCLUDES) $(OS_INCLUDES)

# Disabling pti if not used
ifneq ($(STVID_NO_PTI),)
	EXTRA_CFLAGS += -DSTPTI_UNAVAILABLE
endif

# Disabling hdd if not used
ifneq ($(STVID_NO_HDD),)
	EXTRA_CFLAGS += -DHDD_UNAVAILABLE
endif

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The environment variable KDIR must be set)
endif
# External makefile
PWD     := $(shell pwd)

all: default libstvidtest.a

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules

libstvidtest.a: stvidtest_ioctl_lib.o
	$(BUILD_LIBRARY)

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stvidtest_ioctl-objs)
	$(RM) $(foreach FILE,$(stvidtest_ioctl-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) libstvidtest.a
	$(RM) stvidtest_ioctl_lib.o
endif

