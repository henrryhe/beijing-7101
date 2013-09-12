# KBUILD Makefile for stlayer_core

HEADER_IMPORTS += stlayer/linux stlayer/linux/stlayer_core stlayer stlayer/src
HEADER_IMPORTS += stlayer/src/api stlayer/src/video/hal
HEADER_IMPORTS += stlayer/src/graphics/hal/gamma stcommon stos
ifneq ($(STVID_USE_FGT),)
HEADER_IMPORTS += stlayer/src/video/hal/xvp stlayer/src/video/hal/displaypipe
endif

include $(DVD_MAKE)/kbuild.mak

obj-m             := stlayer_core.o
stlayer_core-objs := stlayer_core_main.o \
                     stlayer_core_exports.o \
                     ../../src/api/layervp.o \
                     ../../src/api/layerlay.o \
                     ../../src/api/layerflt.o \
                     ../../src/video/hal/halv_lay.o

ifneq "$(DVD_FRONTEND)" "7200"
stlayer_core-objs += ../../src/video/hal/sddispo2/hv_lay8.o \
                     ../../src/video/hal/sddispo2/hv_tools.o
endif

ifneq (,$(findstring $(DVD_BACKEND),7109 7200))
stlayer_core-objs += ../../src/video/hal/displaypipe/hv_vdplay.o \
                     ../../src/video/hal/displaypipe/hv_vdptools.o \
                     ../../src/video/hal/displaypipe/hv_vdpdei.o

endif

stlayer_core-objs += ../../src/graphics/hal/gamma/gfxutils.o \
                     ../../src/graphics/hal/gamma/pool_mng.o \
                     ../../src/graphics/hal/gamma/hard_mng.o \
                     ../../src/graphics/hal/gamma/front_vp.o \
                     ../../src/graphics/hal/gamma/back_end.o \
                     ../../src/graphics/hal/gamma/front_cm.o \
                     ../../src/graphics/hal/gamma/desc_mng.o

ifneq ($(STVID_USE_FGT),)
stlayer_core-objs += ../../src/video/hal/xvp/hxvp_main.o \
                     ../../src/video/hal/xvp/hxvp_cluster.o \
                     ../../src/video/hal/xvp/hxvp_fgt.o \
                     ../../src/video/hal/xvp/hxvp_xp70.o
endif

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
EXTRA_CFLAGS += -DHW_$(DVD_BACKEND)

ifneq (,$(findstring $(DVD_BACKEND),7109 7200))
ifneq ($(VIDEO_MINIMIZE_MEMORY_USAGE_FOR_DEI),)
  EXTRA_CFLAGS += -DSTLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI
endif
ifneq ($(STLAYER_NO_DEINTERLACER),)
  EXTRA_CFLAGS += -DSTLAYER_NO_DEINTERLACER
endif
endif

ifneq ($(VIDEO_DEBUG_DEINTERLACING_MODE),)
    EXTRA_CFLAGS +=  -DVIDEO_DEBUG_DEINTERLACING_MODE
endif

ifneq ($(STVID_DEBUG_GET_DISPLAY_PARAMS),)
    EXTRA_CFLAGS += -DSTVID_DEBUG_GET_DISPLAY_PARAMS
endif

ifneq ($(STVID_USE_FGT),)
    EXTRA_CFLAGS +=  -DST_XVP_ENABLE_FLEXVP -DST_XVP_ENABLE_FGT -DSTVID_USE_FGT
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
	$(RM) $(stlayer_core-objs)
	$(RM) $(foreach FILE,$(stlayer_core-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean


endif
