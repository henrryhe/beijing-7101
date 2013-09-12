# COPYRIGHT (C) STMicroelectronics 2006.  All rights reserved.

HEADER_IMPORTS += stos stcommon stfdma
HEADER_IMPORTS += stpti4 stpti4/linux stpti4/src stpti4/linux/stpti4_core
HEADER_IMPORTS += stpti4/src/stpti stpti4/src/stpti/pti4 stpti4/src/stpti/memapi
HEADER_IMPORTS += stpti4/src/stpti/pti4/tc stpti4/src/stpti/frontend
HEADER_IMPORTS += stpti4/src/stpti/frontend/stfe stpti4/src/stpti/frontend/tsgdma

include $(DVD_MAKE)/kbuild.mak

# Pick up default rules and variables
#include $(MAKE_ROOT)/linux-generic.mak

STPTI_OBJS := ../../src/stpti/pti_ba.o \
            ../../src/stpti/pti_vali.o \
            ../../src/stpti/memapi/memget.o \
            ../../src/stpti/memapi/pti_hndl.o \
            ../../src/stpti/memapi/pti_list.o \
            ../../src/stpti/memapi/pti_mem.o \
            ../../src/stpti/pti4/basic.o \
            ../../src/stpti/pti4/tc/tchal.o \
            ../../src/stpti/pti4/validate.o \
            ../../src/stpti/pti4/helper.o \
            ../../src/stpti/pti4/tcinit.o \
            ../../src/stpti/pti4/slotlist.o \
            ../../src/stpti/pti4/tcterm.o \
            ../../src/stpti/pti4/session.o \
            ../../src/stpti/pti4/extended.o \
            ../../src/stpti/pti4/config.o \
            ../../src/stpti/pti4/isr.o \
            ../../src/stpti/pti4/swcdfifo.o \
            ../../src/stpti/pti4/exm.o \
            ../../src/stpti/pti4/hal_dbg.o \
            ../../src/stpti/pti_extd.o \
            ../../src/stpti/pti_int.o \
            ../../src/stpti/pti_swc.o \
            ../../src/stpti/osx.o \
            ../../src/stpti/pti_evt.o \
            ../../src/stpti/pti_fe.o \
            ../../src/stpti/pti_linux.o

#ifeq ($(findstring 7200,$(DVD_FRONTEND)), 7200)
 STPTI_OBJS += ../../src/stpti/frontend/frontend.o \
               ../../src/stpti/frontend/stfe/stfe.o \
               ../../src/stpti/frontend/tsgdma/tsgdma.o
#endif

ifeq ($(findstring 5528,$(DVD_FRONTEND)), 5528)
 STPTI_OBJS += ../../src/stpti/pti4/tc/stdcam.o
else
 STPTI_OBJS += ../../src/stpti/pti4/tc/ramcam.o
endif

ifneq ($(findstring STPTI_NO_INDEX_SUPPORT,$(STPTI_CONFIG)), STPTI_NO_INDEX_SUPPORT)
 STPTI_OBJS += ../../src/stpti/pti_indx.o ../../src/stpti/pti4/ptiindex.o
endif

ifeq ($(findstring STPTI_DVB_SUPPORT,$(STPTI_CONFIG)), STPTI_DVB_SUPPORT)
 STPTI_OBJS += ../../src/stpti/pti_dvb.o ../../src/stpti/pti4/dvb.o
endif

ifeq ($(findstring STPTI_DTV_SUPPORT,$(STPTI_CONFIG)), STPTI_DTV_SUPPORT)
 STPTI_OBJS += ../../src/stpti/pti_dtv.o ../../src/stpti/pti4/dtv.o
endif

ifeq ($(findstring STPTI_CAROUSEL_SUPPORT,$(STPTI_CONFIG)), STPTI_CAROUSEL_SUPPORT)
 STPTI_OBJS += ../../src/stpti/pti_car.o ../../src/stpti/pti4/carousel.o
endif

ifeq ($(findstring STPTI_DEBUG_SUPPORT,$(STPTI_CONFIG)), STPTI_DEBUG_SUPPORT)
STPTI_OBJS += ../../src/stpti/debug.o
endif

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
EXTRA_CFLAGS += -DSTPTI_NOINLINE_MEMGET  \
                $(STPTI_CONFIG) \
                -DSTPTI_WORKAROUND_SFCAM \
                -DSTPTI_LINUX_PRINT_DEBUG \
                -DLINUX_FULL_KERNEL_VERSION \
                -DUSING_BIG_PHYSAREA \
                -DSTSYS_DEVICE_ACCESS


EXTRA_CFLAGS += -DDVD_EXCLUDE_OS20TO21_MACROS

ifeq ($(findstring STTBX_PRINT,$(EXTRA_CFLAGS)), STTBX_PRINT)
STPTI_OBJS += ../../test/utils/errors.o
endif

obj-m           := stpti4_core.o
stpti4_core-objs := stpti4_core_main.o  \
                    stpti4_core_proc.o  \
                    $(STPTI_OBJS)       \
                    stpti4_core_exports.o

# The following checks to see if we have been invoked in the kbuild
# (KERNELRELEASE will be defined). If not we have the means of launching
# the KBUILD (all and default targets).

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The enviroment variable KDIR must be set)
endif

PWD := $(shell pwd)

all: default

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules



# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(STPTI_OBJS)
	$(RM) $(foreach FILE,$(STPTI_OBJS),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean



endif
