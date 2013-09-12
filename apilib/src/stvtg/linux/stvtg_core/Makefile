# KBUILD Makefile for stvtg_core

HEADER_IMPORTS += stvtg/linux stvtg/linux/stvtg_core stvtg stvtg/src
HEADER_IMPORTS += stvtg/src/hal stvtg/src/hal/denc stvtg/src/hal/omega2 stvtg/src/hal/omega2/vos  stvtg/src/hal/omega2/vfe
HEADER_IMPORTS += stclkrv
ifeq "$(STCLKRV_NO_PTI)" ""
HEADER_IMPORTS += stpti4
endif
HEADER_IMPORTS += stos stcommon

include $(DVD_MAKE)/kbuild.mak

obj-m           := stvtg_core.o
stvtg_core-objs := stvtg_core_main.o \
                   stvtg_core_exports.o \
                   ../../src/stvtg.o \
                   ../../src/vtg_drv.o \
                   ../../src/hal/omega2/vos/clk_gen.o \
                   ../../src/hal/omega2/vos/hal_vos.o \
                   ../../src/hal/omega2/vos/sync_in.o \
                   ../../src/hal/vtg_ihal.o \
                   ../../src/hal/denc/vtg_hal1.o \
                   ../../src/hal/omega2/vtg_hal2.o \
                   ../../src/hal/omega2/vfe/hal_vfe.o \

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)

VFE_FAMILY = 5528 5301
VOS_FAMILY = 7100 7109 7200
ifeq "$(findstring $(DVD_FRONTEND), $(VFE_FAMILY))" "$(DVD_FRONTEND)"
VTG_DEFS := -D STVTG_VFE2
endif
ifeq "$(findstring $(DVD_FRONTEND), $(VOS_FAMILY))" "$(DVD_FRONTEND)"
VTG_DEFS := -D STVTG_VOS
endif

USE_CLKRV = 7100 7109 7200
ifeq "$(findstring $(DVD_FRONTEND), $(USE_CLKRV))" "$(DVD_FRONTEND)"
VTG_DEFS+= -D STVTG_USE_CLKRV
endif

ifneq "$(STCLKRV_NO_PTI)" ""
	VTG_DEFS+= -DSTCLKRV_NO_PTI
endif

VTG_DEFS += -D BE_$(DVD_BACKEND)
EXTRA_CFLAGS += $(VTG_DEFS)

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
	$(RM) $(stvtg_core-objs)
	$(RM) $(foreach FILE,$(stvtg_core-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif
