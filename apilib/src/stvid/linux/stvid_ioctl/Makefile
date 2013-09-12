# KBUILD Makefile for stvid_ioctl

HEADER_IMPORTS += stvid stvid/src
HEADER_IMPORTS += stvid/src/api stvid/src/buffers
HEADER_IMPORTS += stcommon stos

ifneq ($(STVID_DIRECT_INTERFACE_PTI),)
HEADER_IMPORTS += stpti4
endif

ifneq (,$(findstring $(DVD_FRONTEND),7100 7109))
HEADER_IMPORTS += stfdma
endif

ifneq ($(STVID_USE_CRC),)
HEADER_IMPORTS += stvid/src/crc
endif

include $(DVD_MAKE)/kbuild.mak

obj-m           := stvid_ioctl.o
stvid_ioctl-objs := stvid_ioctl_main.o \
                    stvid_ioctl_open.o \
                    stvid_ioctl_ioctl.o

ifneq (,$(findstring $(DVD_FRONTEND),7100 7109))
# All MULTICOM concern
EXTRA_CFLAGS += -I$(RPC_ROOT)/include -I$(RPC_ROOT)/src/mme/include -I$(RPC_ROOT)/src/embx/include
endif

ifdef KERNEL_MODE
EXTRA_CFLAGS += -DMODULE
endif

EXTRA_CFLAGS += $(DVD_CFLAGS)

#EXTRA_CFLAGS += -DSTTBX_PRINT -DSTTBX_REPORT

EXTRA_CFLAGS += -DAVMEM_KERNEL_MODE -DUSE_LMI_SYS_FOR_LINUX -DSTVID_DEBUG_GET_STATISTICS -DGLOBAL_PASS_PARAMS_THROUGH_IOCTLS

ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
EXTRA_CFLAGS += -D__SH4__
EXTRA_CFLAGS += -DHAL_ST40
ifdef STUB_INJECT
EXTRA_CFLAGS += -DSTUB_INJECT
endif
endif

ifneq ($(STVID_USE_FGT),)
    HEADER_IMPORTS += stvid/src/fgt
    EXTRA_CFLAGS += -DST_XVP_ENABLE_FLEXVP -DST_XVP_ENABLE_FGT -DSTVID_USE_FGT -I$(DVD_ROOT)/dvdgr-prj-stvid/src/fgt
endif

ifneq ($(VIDEO_DEBUG_DEINTERLACING_MODE),)
    EXTRA_CFLAGS += -DVIDEO_DEBUG_DEINTERLACING_MODE
endif

ifneq ($(STVID_DEBUG_GET_DISPLAY_PARAMS),)
    EXTRA_CFLAGS += -DSTVID_DEBUG_GET_DISPLAY_PARAMS
endif

ifneq ($(STVID_USE_CRC),)
EXTRA_CFLAGS += -DSTVID_USE_CRC -DST_crc
endif

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)

ifneq ($(STVID_DEBUG_GET_STATUS),)
EXTRA_CFLAGS +=  -DSTVID_DEBUG_GET_STATUS
endif

ifneq ($(STVID_DIRECT_INTERFACE_PTI),)
EXTRA_CFLAGS +=  -DSTVID_DIRECT_INTERFACE_PTI
endif

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The environment variable KDIR must be set)
endif

# External makefile
PWD     := $(shell pwd)

all: default libstvid.a

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules

#libstvid.a: ../../tests/src/linux/libstvidtest.a stvid_ioctl_lib.o
#libstvid.a: $(BUILD_DIR)/tests/src/linux/libstvidtest.a stvid_ioctl_lib.o
#	$(BUILD_LIBRARY)
#
#../../tests/src/linux/libstvidtest.a:
#	$(MAKE) -C ../../tests/src/linux
#	$(MAKE) -C ../../tests/src/linux lib


ifdef STVID_BUILD_TESTS
libstvid.a: stvid_ioctl_lib.o ../stvidtest_ioctl/stvidtest_ioctl_lib.o
	$(BUILD_LIBRARY)
else
libstvid.a: stvid_ioctl_lib.o
	$(BUILD_LIBRARY)
endif

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stvid_ioctl-objs)
	$(RM) $(foreach FILE,$(stvid_ioctl-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) libstvid.a
	$(RM) stvid_ioctl_lib.o
	$(RM) ../stvidtest_ioctl/stvidtest_ioctl_lib.o
endif
