# KBUILD Makefile for stvmix_core

HEADER_IMPORTS += stvmix/linux stvmix/linux/stvmix_core stvmix stvmix/src
HEADER_IMPORTS += stvmix/src/hal stvout/linux/stvout_core
HEADER_IMPORTS += stos stcommon

include $(DVD_MAKE)/kbuild.mak

ifndef DVD_PLATFORM
  DVD_PLATFORM := mb411
endif


EXTRA_CFLAGS += -D$(DVD_PLATFORM) \
-DST_$(DVD_FRONTEND) \
-DARCHITECTURE_ST40 \
-D__SH4__


obj-m            := stvmix_core.o
stvmix_core-objs := stvmix_core_main.o \
                    stvmix_core_exports.o \
                    ../../src/stvmix.o \
                    ../../src/vmix_drv.o \
                    ../../src/hal/gengamma.o

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
EXTRA_CFLAGS += -DSTVMIX_GENGAMMA -DSTVMIX_GENGAMMA_$(DVD_BACKEND)

ifdef STLINUX_RT_THREADS
EXTRA_CFLAGS += -DSTLINUX_RT_THREADS
endif

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The environment variable KDIR must be set)
endif

# External makefile
PWD     := $(shell pwd)

all: default

default:
	$(MAKE) V=1 -C $(KDIR) M=$(PWD)  modules

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest

clean:
	$(RM) $(stvmix_core-objs)
	$(RM) $(foreach FILE,$(stvmix_core-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif
