#########################################
# MODULE : tbx_test.lib / libtbx_test.a #
#########################################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Dependencies
HEADER_IMPORTS := stos stsys stapigat/src stcommon

STTBX_NO_UART := 1


ST20_IMPORTS := stapigat stos stcommon stboot sttbx
ifeq "$(STTBX_NO_UART)" ""
  ST20_IMPORTS += stpio stuart
endif
ST40_IMPORTS := $(ST20_IMPORTS)

ST200_IMPORTS := $(ST20_IMPORTS)

LINUX_IMPORTS := $(ST20_IMPORTS)

# local objects which comprise this component
ST20_OBJS := tbx_test.tco metrics.tco
ST40_OBJS := tbx_test.o metrics.o
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

CFLAGS := $(CFLAGS) -D STTBX_REPORT -D STTBX_PRINT -D STTBX_INPUT -D STTBX_FILTER
ifneq "$(STTBX_NO_UART)" ""
  CFLAGS := $(CFLAGS) -D STTBX_NO_UART
  DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART
endif

STAPIGAT_FLAGS :=
STTBX_DCACHE_DISABLED :=
STTBX_ICACHE_DISABLED :=
ifneq "$(STTBX_TEST_DCACHE_OFF)" ""
  STTBX_DCACHE_DISABLED := 1
endif
ifneq ($(DEBUG),)
  STTBX_ICACHE_DISABLED := 1
  STTBX_DCACHE_DISABLED := 1
endif
ifeq ($(STTBX_DCACHE_DISABLED),)
	STAPIGAT_FLAGS +=  -DDCACHE_ENABLE
endif
ifeq ($(STTBX_ICACHE_DISABLED),)
	STAPIGAT_FLAGS += -DICACHE_ENABLE
endif
ifeq "$(BUILDING_FOR)" "ST200"
	STAPIGAT_FLAGS += -DICACHE_ENABLE
endif
stapigat.lib libstapigat.a: \
	DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
	STAPIGAT_INCLUDE := sttbx/tests/src stcommon
	STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS)
	STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS)
	STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS)
	STAPIGAT_IMPORTS_LINUX := $(LINUX_IMPORTS)

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200
export STAPIGAT_IMPORTS_LINUX 

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

ST20_TARGETS := tbx_test.lku
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

LINKFLAGSDRAM = -O2 -D STTBX_REPORT -D STTBX_PRINT -D STTBX_INPUT -D STTBX_FILTER

# generate mapfile for tbx_test application with OS20
tbx_test_ST20_LKFLAGS = -M tbx_test.map

tbx_test_INIT := board_init

ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

BOARDSPECS=
ifeq "$(ARCHITECTURE)" "ST40"
	ifeq "$(DVD_BACKEND)" "7200"
		ifndef MULTICOM_COMPANION_CONFIG
			export MULTICOM_COMPANION_CONFIG=0x07
		endif
		export USE_MBOARD=board_$(DVD_PLATFORM)_lmi0_$(MULTICOM_COMPANION_CONFIG)
		BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7200
	endif
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(BOARDSPECS)
endif
ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif
ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif
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
