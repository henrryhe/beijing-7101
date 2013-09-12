###############################################
# MODULE : vos.lib / libvos.a for stvtg       #
###############################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Add include path
#---------------------
INCLUDE_PATH += $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/../../..

# Define the targets
#---------------------------------------------
HEADER_IMPORTS := stos stsys stvtg
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

# Add dependence on correct stpti vob if STCLKRV_NO_PTI is not set
# stpti4 lite for 7710 and stpti4 for rest
USE_VOS_CLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND),$(USE_VOS_CLKRV))" "$(DVD_BACKEND)"
ifeq "$(STCLKRV_NO_PTI)" ""
ifneq (,$(findstring $(DVD_FRONTEND),7710))
    HEADER_IMPORTS += $(DVD_TRANSPORT)lt
else
    HEADER_IMPORTS += $(DVD_TRANSPORT)
endif
endif
endif

ST20_IMPORTS := stevt stgxobj stpio stuart sttbx
ST200_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS := vos.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_OBJS := hal_vos.tco clk_gen.tco sync_in.tco
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))


LINUX_IMPORTS := stevt stgxobj sttbx
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ST40_IMPORTS := $(ST20_IMPORTS) os20emu
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


# Compile options
#----------------
ifneq ($(STVTG_WA_GNBVD35956),)
CFLAGS := $(CFLAGS) -D WA_GNBvd35956
endif

ifdef WA_GNBvd50834
DVD_CFLAGS+= -DWA_GNBvd50834
endif

CFLAGS := $(VTG_DEFS) $(CFLAGS)

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)
endif

# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
hal_vos.tco hal_vos.o: vtg_drv.h vtg_hal2.h clk_gen.h sync_in.h vos_reg.h hal_vos.h
clk_gen.tco clk_gen.o: vtg_drv.h clk_gen.h vos_reg.h
sync_in.tco sync_in.o: vtg_drv.h sync_in.h vos_reg.h
