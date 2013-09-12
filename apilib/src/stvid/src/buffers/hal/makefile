######################
# MODULE : halv_buf  #
######################

# video specific variables
HAL_NAME = halv_buf

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := halv_buf.tco
ifneq ($(findstring omega1,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_buf1.tco
endif
ifneq ($(findstring omega2,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_buf2.tco
endif
ifneq ($(findstring genbuff,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) genbuff.tco
endif

# Pick up standars rules
#-----------------------
include $(DVD_BUILD_DIR)/../../halrules.mak

# Additional rules for files not in current directory
hv_buf1.tco hv_buf1.o: omega1/hv_buf1.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_buf2.tco hv_buf2.o: omega2/hv_buf2.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
genbuff.tco genbuff.o: genbuff/genbuff.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Dependancies
halv_buf.tco halv_buf.o : halv_buf.h

#end of file
