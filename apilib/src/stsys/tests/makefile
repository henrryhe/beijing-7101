#########################################
# MODULE : sys_test.lib / libsys_test.a #
#########################################
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon (stsys excluded)
HEADER_IMPORTS := stsys
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

ST20_IMPORTS =
ST40_IMPORTS =
LINUX_IMPORTS =

# Define the targets
#-------------------
ST20_TARGETS = sys_test.lku
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))

ST20_EXPORTS =
ST40_EXPORTS =
LINUX_EXPORTS =


# local objects which comprise this component
ST20_OBJS = sys_test.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

CFLAGS := $(CFLAGS) -D TESTED_PLATFORM=\"$(DVD_PLATFORM)\" -D TESTED_CHIP=\"$(DVD_FRONTEND)\"

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

sys_test_INIT := board_init
BOARDSPECS=
ifeq "$(ARCHITECTURE)" "ST40"
	ifeq "$(DVD_BACKEND)" "7200"
		ifndef MULTICOM_COMPANION_CONFIG
			export MULTICOM_COMPANION_CONFIG=0x07
		endif
		export USE_MBOARD=board_$(DVD_PLATFORM)_lmi0_$(MULTICOM_COMPANION_CONFIG)
		BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stsys/tests/boardspecs_7200
	endif
endif


# Build lib and obj
#------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(BOARDSPECS)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

#end
