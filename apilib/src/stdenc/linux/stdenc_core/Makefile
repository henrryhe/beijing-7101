# KBUILD Makefile for stdenc_core

HEADER_IMPORTS += stdenc/linux stdenc/linux/stdenc_core stdenc stdenc/src
HEADER_IMPORTS += stdenc/src/hal stdenc/src/hal/reg stdenc/src/hal/reg/rbus
HEADER_IMPORTS += stos stcommon

include $(DVD_MAKE)/kbuild.mak

obj-m            := stdenc_core.o
stdenc_core-objs := stdenc_core_main.o \
                    stdenc_core_exports.o \
                    ../../src/stdenc.o \
                    ../../src/denc_drv.o \
                    ../../src/hal/denc_hal.o \
                    ../../src/hal/reg/denc_reg.o \
                    ../../src/hal/reg/rbus/reg_rbus.o \

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

all: default

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules


# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stdenc_core-objs)
	$(RM) $(foreach FILE,$(stdenc_core-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif
