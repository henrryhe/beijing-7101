######################
# MODULE : halv_dis  #
######################

# video specific variables
HAL_NAME = halv_dis

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := halv_dis.tco

ifneq ($(findstring blitdisp,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) blitdisp.tco
endif

ifneq ($(findstring omega1,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_dis1.tco
endif

ifneq ($(findstring omega2,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_dis2.tco
ifneq ($(findstring diginput,$(VID_OPTIONAL_FEATURES)),)
ST20_OBJS += hv_di_sd.tco hv_di_hd.tco
endif
ifneq ($(findstring diginput,$(VID_OPTIONAL_MODULES)),)
ST20_OBJS += hv_di_sd.tco hv_di_hd.tco
endif
endif

ifneq ($(findstring sddispo2,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_dis8.tco
endif

ifneq ($(findstring displaypipe,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_vdp.tco
endif

# Add generic digital input for all chips requiring it
# sddispo2 & displaypipe not in VID_HW_CELLS
ifeq ($(findstring sddispo2,$(VID_HW_CELLS)),)
ifeq ($(findstring displaypipe,$(VID_HW_CELLS)),)
ifneq ($(findstring diginput,$(VID_OPTIONAL_FEATURES)),)
ST20_OBJS += hv_gx_di.tco
endif
endif
endif

# Add graphics video display support
ifneq ($(findstring graphics,$(VID_HW_CELLS)),)
ST20_OBJS := $(ST20_OBJS) hv_gdp.tco
endif

# Pick up standard rules
#-----------------------
include $(DVD_BUILD_DIR)/../../halrules.mak

# Additional rules for files not in current directory
hv_dis1.tco hv_dis1.o: omega1/hv_dis1.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_dis2.tco hv_dis2.o: omega2/hv_dis2.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_di_sd.tco hv_di_sd.o: omega2/hv_di_sd.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_di_hd.tco hv_di_hd.o: omega2/hv_di_hd.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_gx_di.tco hv_gx_di.o: omega2/hv_gx_di.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_dis8.tco hv_dis8.o: sddispo2/hv_dis8.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
blitdisp.tco blitdisp.o: blitdisp/blitdisp.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_vdp.tco hv_vdp.o: displaypipe/hv_vdp.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
hv_gdp.tco hv_gdp.o: graphics/hv_gdp.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Dependencies
halv_dis.tco halv_dis.o : halv_dis.h

#end of file
