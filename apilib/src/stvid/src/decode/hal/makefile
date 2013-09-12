######################
# MODULE : halv_dec  #
######################

# video specific variables
HAL_NAME = halv_dec

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := halv_dec.tco
# Multi-decode SW controller specific
ifneq ($(findstring multidecodeswcontroller,$(VID_OPTIONAL_FEATURES)),)
ST20_OBJS := $(ST20_OBJS) halv_mult.tco
endif
# HAL's specific
ifneq ($(findstring omega1,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_dec1.tco
endif

ifneq ($(findstring omega2,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_dec2.tco
ifeq "$(DVD_BACKEND)" "7015"
HALSUBDIR := 7015
endif
ifeq "$(DVD_BACKEND)" "7020"
HALSUBDIR := 7020
ifeq "$(DVD_FRONTEND)" "5517"
CFLAGS += -D STEM_7020
endif
endif
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
CFLAGS := $(CFLAGS) -DSTAPI_INTERRUPT_BY_NUMBER
endif

ifneq ($(findstring sdmpgo2,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_dec8.tco
endif

ifneq ($(findstring lcmpegx1,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) lcmpegx1.tco
endif

ifneq (,$(findstring $(DVD_BACKEND), 7100 7109 7710))
VID_DEFS := $(VID_DEFS) -D LCMPEGH1
endif

# Pick up standars rules
#-----------------------
include $(DVD_BUILD_DIR)/../../halrules.mak

# Additional rules for files not in current directory
hv_dec1.tco hv_dec1.o: omega1/hv_dec1.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_dec2.tco hv_dec2.o: omega2/$(HALSUBDIR)/hv_dec2.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_dec8.tco hv_dec8.o: sdmpgo2/hv_dec8.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
lcmpegx1.tco lcmpegx1.o: lcmpegx1/lcmpegx1.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)


# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Dependancies
halv_dec.tco halv_dec.o : halv_dec.h halv_mult.h ../vid_mpeg.h
halv_mult.tco halv_mult.o : halv_mult.h ../vid_mpeg.h

#end of file
