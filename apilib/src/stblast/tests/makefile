DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stsys stblast
ifeq "$(DVD_OS)" "LINUX"
IMPORTS        := stos stpio stcommon testtool stevt stblast
else
IMPORTS        := stos stpio stcommon stboot stevt stblast testtool
endif

ifneq "$(DVD_OS)" "LINUX"
ifndef DISABLE_TBX
IMPORTS += sttbx
ifndef STTBX_NO_UART
IMPORTS += stuart
endif
endif
endif

TARGETS := auto$(EXE_SUFFIX) manual$(EXE_SUFFIX) blast$(EXE_SUFFIX)

#ST40
ifndef ST40_TARGETS
  ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
  ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
  ST40_IMPORTS = $(IMPORTS)
endif

# ST200
ifndef ST200_TARGETS
  ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
  ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
  ST200_IMPORTS = $(IMPORTS)
endif

# Setup values
ifdef TEST_VERBOSE
    CFLAGS := $(CFLAGS) -DVERBOSE
endif


ifdef STBLAST_USE_UART
    CFLAGS := $(CFLAGS) -DSTBLAST_USE_UART
endif
ifdef DISABLE_TBX
    CFLAGS := $(CFLAGS) -DDISABLE_TBX
endif

CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT

ifdef UNIFIED_MEMORY
    CFLAGS := $(CFLAGS) -DUNIFIED_MEMORY
endif

ifdef AUTO_TESTING_ENABLE
    CFLAGS := $(CFLAGS) -DAUTO_TESTING_ENABLE
endif

ifdef DISABLE_ICACHE
    CFLAGS := $(CFLAGS) -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
    CFLAGS := $(CFLAGS) -DDISABLE_DCACHE
endif

ifdef IR_INVERT
    CFLAGS := $(CFLAGS) -DIR_INVERT
endif

include $(DVD_MAKE)/defrules.mak

# .tco files we have to build
ifeq "$(ARCHITECTURE)" "ST40"
auto$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -DTEST_AUTO $(BASE_INCLUDES) -o $@ -c $<

blast$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -DTEST_BLAST $(BASE_INCLUDES) -o $@ -c $<

manual$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -DTEST_MANUAL $(BASE_INCLUDES) -o $@ -c $<
endif

ifeq "$(ARCHITECTURE)" "ST200"
auto$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -mcore=st231 -DTEST_AUTO $(BASE_INCLUDES) -o $@ -c $<

blast$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -mcore=st231 -DTEST_BLAST $(BASE_INCLUDES) -o $@ -c $<

manual$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -mcore=st231 -DTEST_MANUAL $(BASE_INCLUDES) -o $@ -c $<
endif

ifeq "$(ARCHITECTURE)" "ST20"
auto$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -DTEST_AUTO $(BASE_INCLUDES) $^ -o $@

blast$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -DTEST_BLAST $(BASE_INCLUDES) $^ -o $@

manual$(OBJ_SUFFIX): irtest.c
	$(CC) $(CFLAGS) -DTEST_MANUAL $(BASE_INCLUDES) $^ -o $@
endif


# .lku files
auto$(EXE_SUFFIX): auto$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

blast$(EXE_SUFFIX): blast$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

manual$(EXE_SUFFIX): manual$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clean:
	-$(RM) *$(OBJ_SUFFIX)
	-$(RM) *.dbg
	-$(RM) *$(EXE_SUFFIX)

else

include $(DVD_MAKE)/builddir.mak

endif

