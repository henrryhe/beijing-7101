#########################################
# MODULE : tst_test.lib / libtst_test.a #
#########################################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Dependencies
HEADER_IMPORTS := stos stsys stapigat/src
ST20_IMPORTS := stos stapigat stcommon  stboot sttbx testtool

LINUX_IMPORTS := stos stapigat stcommon stavmem sttbx testtool 
ST40_IMPORTS := $(ST20_IMPORTS)

ST200_IMPORTS := $(ST20_IMPORTS)

# local objects which comprise this component
ST20_OBJS := cltst.tco tst_test.tco

LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
ifneq "$(DVD_OS)" "LINUX"
ST20_OBJS := $(ST20_OBJS) metrics.tco
endif

STAPIGAT_FLAGS :=
ifneq ($(DEBUG),)
  STTST_ICACHE_DISABLED := 1
  STTST_DCACHE_DISABLED := 1
endif
ifeq ($(STTST_DCACHE_DISABLED),)
  STAPIGAT_FLAGS +=  -DDCACHE_ENABLE
endif
ifeq ($(STTST_ICACHE_DISABLED),)
  STAPIGAT_FLAGS += -DICACHE_ENABLE
endif
ifeq "$(BUILDING_FOR)" "ST200"
	STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

# Remove echo when Posix is used
ifneq ($(DVD_POSIX),)
       CFLAGS += -D CONFIG_POSIX
endif
# For output of toolbox
CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT -D STTBX_NO_UART
DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART
#number of max symboles could be programmed with build variable
ifneq ($(TESTTOOL_MAX_SYMBOLS),)
  ifeq ($(findstring TESTTOOL_MAX_SYMBOLS,$(DVD_CFLAGS)),)
    DVD_CFLAGS := $(DVD_CFLAGS) -D TESTTOOL_MAX_SYMBOLS
  endif
endif
# STAVMEM is special in linux - no library is created so we must not add
# the library to the link command. The following link option suppresses this.
# This only works currently on LINUX.
LINUX_libstavmem.a_NOLINK := 1

stavmem.lib: \
	STAVMEM_NO_GPDMA=1
	STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA

stapigat.lib libstapigat.a : \
	DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
        STAPIGAT_INCLUDE := testtool/tests/src $(ST40_IMPORTS)
	STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS)
ifeq "$(DVD_OS)" "LINUX"
        STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)
else
        STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS)
endif
	STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS)

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

ST20_TARGETS = tst_test.lku
LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))

ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))

include $(DVD_MAKE)/defrules.mak

tst_test_INIT := board_init

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

# Local dependancies
cltst.tco cltst.o:
tst_test.tco tst_test.o:
metrics.tco: metrics.h

#end
