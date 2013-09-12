# STBOOT makefile
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak



COMPONENT_NAME := stboot
# Other components this component is dependant upon
HEADER_IMPORTS := stsys
ST20_IMPORTS := stcommon
ST20_EXPORTS := stboot.h stboot.lib
ST20_TARGETS := $(COMPONENT_NAME).lib
ifeq "$(DVD_OS)" "OS40"
 ST40_IMPORTS := stcommon os20emu
else
 ST40_IMPORTS := stcommon
endif
ST40_EXPORTS := stboot.h $(LIB_PREFIX)stboot$(LIB_SUFFIX)     # stboot.h & libstboot.a
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_IMPORTS := stcommon
ST200_EXPORTS := stboot.h $(LIB_PREFIX)stboot$(LIB_SUFFIX)    # stboot.h & (libstboot.a with make)/(libstboot_st200.a with makeplus)
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

# Supported devices with distinct subdirs - not for ST40/ST200
STBOOT_DEVICES := TP3 5510 5512 5508 5518 5580 7015 5514 5528 5100 5101 7710 5105 5700 5188 5107

# Set backend directory and local objects (defaults)

ifeq "$(filter $(DVD_FRONTEND),5700)" "$(DVD_FRONTEND)"
BACKEND_DIR = $(DVD_BUILD_DIR)$(SLASH)stm$(DVD_FRONTEND)
else
BACKEND_DIR = $(DVD_BUILD_DIR)$(SLASH)sti$(DVD_FRONTEND)
endif
ifeq "$(filter $(DVD_FRONTEND),5528 5100 5101 7710 5105 5700 7100 5301 8010 7109 5188 5525 5107 7200)" "$(DVD_FRONTEND)"
    # pll/sdrm done in the cfg file and not supplied in C
    ST20_OBJS := stboot.tco cache.tco be_init.tco
else # old-style with pll/sdrm/be_init
    ST20_OBJS := stboot.tco cache.tco sdram.tco be_init.tco \
                 $(BACKEND_DIR)$(SLASH)pll.tco
endif


ifeq "$(filter $(DVD_FRONTEND),7710 5105 5700 5188 5107)" "$(DVD_FRONTEND)"
ST20_OBJS += st_c1_timer.tco
endif

# Handle exceptions

70XX_CHIPS = 7015 7020
ifeq "$(filter $(DVD_BACKEND),$(70XX_CHIPS))" "$(DVD_BACKEND)"
    BACKEND_DIR := $(DVD_BUILD_DIR)$(SLASH)sti7015
    CFLAGS += -DREMOVE_GENERIC_ADDRESSES  -DUSE_$(DVD_BACKEND)_GENERIC_ADDRESSES
ifeq ($(DVD_FRONTEND),TP3)
    INCLUDES := $(INCLUDES) $(INCLUDE_PREFIX)$(DVD_BUILD_DIR)$(SLASH)st20tp3
else
    INCLUDES := $(INCLUDES) $(INCLUDE_PREFIX)$(DVD_BUILD_DIR)$(SLASH)sti$(DVD_FRONTEND)
endif
    ST20_OBJS := stboot.tco cache.tco be_init.tco \
           $(BACKEND_DIR)$(SLASH)sdrm7015.tco \
           $(BACKEND_DIR)$(SLASH)be_7015.tco \
           $(BACKEND_DIR)$(SLASH)clk_gen.tco \
	   $(BACKEND_DIR)$(SLASH)board.tco

ifeq "$(filter $(DVD_PLATFORM),mb290 mb295 mb376)" "$(DVD_PLATFORM)"
    ST20_OBJS += \
           $(BACKEND_DIR)$(SLASH)ics9161.tco \
           $(BACKEND_DIR)$(SLASH)fpga.tco
else
    # db573 requires sti2c to reset 7020 via PCF8575
    IMPORTS += sti2c
endif
endif

ifeq ($(DVD_PLATFORM),mb193)
    BACKEND_DIR := $(DVD_BUILD_DIR)$(SLASH)st20tp3
    ST20_OBJS := stboot.tco cache.tco \
                 $(BACKEND_DIR)$(SLASH)stubs.tco
endif

ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

ifdef STBOOT_CONFIGURABLE_INT_TRIGGER
    CFLAGS := $(CFLAGS) -DSTBOOT_CONFIGURABLE_INT_TRIGGER=1
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS := $(CFLAGS) -DSTAPI_INTERRUPT_BY_NUMBER
endif

ifeq ($(DVD_OS20),RUNTIME)
    CFLAGS := $(CFLAGS) -DOS20_RUNTIME=1
endif

ifeq ($(ARCHITECTURE),ST200)
 ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
    CFLAGS := $(CFLAGS) -DSTBOOT_DISPLAY_BSP_MEMORY_MAP
 endif

 ifdef STBOOT_SPECIFY_HOST_MEMORY_MAP
    CFLAGS := $(CFLAGS) -DSTBOOT_SPECIFY_HOST_MEMORY_MAP
 endif
endif

ifdef STBOOT_LMI_TOP_REGION_ICACHE_ENABLE
    CFLAGS := $(CFLAGS) -DSTBOOT_LMI_TOP_REGION_ICACHE_ENABLE
endif

ifdef WA_GNBvd44290
    CFLAGS := $(CFLAGS) -DWA_GNBvd44290
endif

# Allow user to pass CFLAGS (eg STTBX_PRINT) to just STBOOT
CFLAGS  := $(CFLAGS) $(STBOOT_CFLAGS)

# Enumerate objects to delete for _all_ backend directories
ifeq "$(filter $(DVD_FRONTEND),5700)" "$(DVD_FRONTEND)"
BACKEND_CLEAN := $(foreach DEVICE,$(STBOOT_DEVICES),\
                 $(if $(filter $(DEVICE),TP3),\
                   $(DVD_BUILD_DIR)$(SLASH)st20tp3$(SLASH)*$(OBJ_SUFFIX),\
                   $(DVD_BUILD_DIR)$(SLASH)stm$(DEVICE)$(SLASH)*$(OBJ_SUFFIX)))
else
BACKEND_CLEAN := $(foreach DEVICE,$(STBOOT_DEVICES),\
                 $(if $(filter $(DEVICE),TP3),\
                   $(DVD_BUILD_DIR)$(SLASH)st20tp3$(SLASH)*$(OBJ_SUFFIX),\
                   $(DVD_BUILD_DIR)$(SLASH)sti$(DEVICE)$(SLASH)*$(OBJ_SUFFIX)))
endif

# Local subdirectories containing header files
INCLUDES := $(INCLUDES) $(INCLUDE_PREFIX)$(BACKEND_DIR) \
            $(INCLUDE_PREFIX).

# Pick up standard targets

include $(DVD_MAKE)/defrules.mak

#$(ST20_TARGETS): $(ST20_OBJS)
#	$(BUILD_LIBRARY)
#$(ST40_TARGETS): $(ST40_OBJS)
#	$(BUILD_LIBRARY)
#$(ST200_TARGETS): $(ST200_OBJS)
#	$(BUILD_LIBRARY)

$(LIB_PREFIX)stboot$(LIB_SUFFIX): $($(ARCHITECTURE)_OBJS)
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) *$(OBJ_SUFFIX)
	-$(RM) $(BACKEND_CLEAN)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)

else

include $(DVD_MAKE)/builddir.mak

endif

# end
