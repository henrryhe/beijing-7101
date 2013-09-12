# STBOOT test makefile
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

# Produce a map file for ST20 [must use '=' not ':=']
ifeq ($(ARCHITECTURE),ST20)
 LKFLAGS = -M boot_test.map
endif 
ifeq ($(ARCHITECTURE),ST40)
 LKFLAGS = -Xlinker -Map=boot_test.map
endif
ifeq ($(ARCHITECTURE),ST200)
LKFLAGS = -Wl,--M boot_test.map
endif

ST20_IMPORTS := stboot stcommon
ifeq "$(DVD_OS)" "OS40"
 ST40_IMPORTS := stboot stcommon os20emu
else
 ST40_IMPORTS := stboot stcommon
endif
ST20_TARGETS := boot_test.lku
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST200_IMPORTS := stboot stcommon
ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))

ST20_OBJS := boot_test.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

ifndef TEST_INVALID
TEST_INVALID=0
endif

ifdef DISABLE_ICACHE
    CFLAGS := $(CFLAGS) -D DISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
    CFLAGS := $(CFLAGS) -D DISABLE_DCACHE
endif

ifeq ($(ARCHITECTURE),ST200)
 ifdef STBOOT_SPECIFY_HOST_MEMORY_MAP
    CFLAGS := $(CFLAGS) -DSTBOOT_SPECIFY_HOST_MEMORY_MAP
 endif
endif

CFLAGS := $(CFLAGS) -DTEST_INVALID=$(TEST_INVALID)

include $(DVD_MAKE)/defrules.mak

boot_test_INIT := board_init
#$(ST20_TARGETS): $(ST20_OBJS) $(IMPORT_LIBS)
#	@$(ECHO) Linking $@
#	$(LINK_EXECUTABLE)

#$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS)
#	@$(ECHO) Linking $@
#	$(LINK_EXECUTABLE)

#$(ST200_TARGETS): $(ST200_OBJS) $(IMPORT_LIBS)
#	@$(ECHO) Linking $@
#	$(LINK_EXECUTABLE)

boot_test$(EXE_SUFFIX): $($(ARCHITECTURE)_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) *.dbg
	-$(RM) *.map

else

include $(DVD_MAKE)/builddir.mak

endif

# End
