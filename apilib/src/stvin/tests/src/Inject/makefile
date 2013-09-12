DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stsys
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    ifeq "$(DVD_OS)" "OS40"
         HEADER_IMPORTS += os20emu
    endif
endif

COMMON_IMPORTS := stboot stcommon stevt testtool stvtg stvout stdenc stgxobj
COMMON_IMPORTS += stlayer stvmix stavmem sttbx stuart stpio stvid

ST20_IMPORTS := $(COMMON_IMPORTS)
ST40_IMPORTS := $(COMMON_IMPORTS)

ST20_IMPORTS += stclkrv stpwm sti2c stlli
ST20_IMPORTS += $(DVD_TRANSPORT)

ifneq (,$(findstring $(DVD_BACKEND),7015 7020))
ST20_IMPORTS += stintmr
endif

ifeq "$(DVD_BACKEND)" "5514"
ST20_IMPORTS += stcfg stgpdma sttsmux
endif

ST40_IMPORTS += os20emu

# Object for ST20
COMMON_OBJS := startup.tco api_cmd.tco lay_cmd.tco vmix_cmd.tco
COMMON_OBJS += vout_cmd.tco vtg_cmd.tco denc_cmd.tco

ST20_OBJS := $(COMMON_OBJS) pti_cmd.tco vid_cmd.tco vid_util.tco

# Object for ST40
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(COMMON_OBJS))

# For compilation of pti+link or stpti
CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)

# For output of toolbox
CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
ifeq ($(DEBUG), 1)
	CFLAGS := $(CFLAGS) -O0 -D DEBUG
endif

# Target
ST20_TARGETS = inject.lku
ST20_TARGETS_OTHER = inject.dbg
ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST40_TARGETS_OTHER =

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build general application for DRAM
inject_INIT := board_init
$(ST20_TARGETS): $(IMPORT_LIBS) $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS) $(SUBDIR_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS) $($(ARCHITECTURE)_TARGETS_OTHER)
ifeq "$(ARCHITECTURE)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(ARCHITECTURE)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(ARCHITECTURE)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# end of makefile inject
