########################
# MODULE : decode.lib  #
########################

# video specific variables
VID_MODULE_NAME = decode
VID_OTHER_MODULES_DEPENDENCIES = buffers producer
ifneq ($(findstring ordqueue,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) ordqueue
endif
ifneq ($(findstring inject,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) inject
endif
ifneq ($(findstring deblock,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) deblock
endif
ifneq ($(findstring fgt,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) fgt
endif

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := vid_bits.tco vid_head.tco vid_dec.tco vid_intr.tco vid_sc.tco
# Multi-decode SW controller specific
ifneq ($(findstring multidecodeswcontroller,$(VID_OPTIONAL_FEATURES)),)
ST20_OBJS := $(ST20_OBJS) vid_mult.tco
endif
# DirectTV specific
ifeq ($(DVD_SERVICE),DIRECTV)
ST20_OBJS += vid_dtv.tco
CFLAGS += -DSTVID_DIRECTV
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
CFLAGS := $(CFLAGS) -DSTAPI_INTERRUPT_BY_NUMBER
endif

# Sub-libs for HAL :
#-------------------
SUBDIRS := hal

# Pick up standars rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
vid_bits.tco vid_bits.o : ../vid_com.h vid_dec.h decode.h vid_bits.h vid_dec.h hal/halv_dec.h
vid_head.tco vid_head.o : ../vid_com.h vid_dec.h decode.h vid_head.h vid_dec.h vid_bits.h hal/halv_dec.h
vid_mult.tco vid_mult.o : ../vid_com.h vid_dec.h decode.h vid_dec.h hal/halv_mult.h
vid_dec.tco vid_dec.o   : ../vid_com.h vid_dec.h decode.h vid_dec.h vid_head.h vid_intr.h vid_sc.h vid_bits.h ../vid_ctcm.h hal/halv_dec.h
vid_intr.tco vid_intr.o : ../vid_com.h vid_dec.h decode.h vid_intr.h vid_dec.h vid_head.h vid_bits.h hal/halv_dec.h
ifeq ($(DVD_SERVICE),DIRECTV)
vid_sc.tco vid_sc.o     : ../vid_com.h vid_dec.h decode.h vid_sc.h hal/halv_dec.h ../vid_ctcm.h vid_dtv.h
#else
vid_sc.tco vid_sc.o     : ../vid_com.h vid_dec.h decode.h vid_sc.h hal/halv_dec.h ../vid_ctcm.h
endif
ifeq ($(DVD_SERVICE),DIRECTV)
vid_dtv.tco vid_dtv.o   : ../vid_com.h vid_dec.h decode.h vid_dtv.h
endif

#end of file
