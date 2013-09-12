#############################
#  Test Application #
#############################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak


# Other components this component is dependant upon
# -------------------------------------------------

HEADER_IMPORTS := stsys stapigat/src
IMPORTS := stvid stavmem stevt stfdma stlayer stapigat stboot sttbx testtool \
           stcommon stvmix stgxobj stvtg stdenc stvout

ifneq (,$(findstring $(DVD_FRONTEND),5100))
        IMPORTS += compo stdisp
endif

# Sources of the test
# -------------------
# Object for ST20
ST20_OBJS := video.tco stubs.tco main.tco vid_cmd2.tco


# Flags and options
# -----------------
ifneq ($(STVID_DEBUG_GET_STATISTICS),)
    CFLAGS += -DSTVID_DEBUG_GET_STATISTICS
    STAPIGAT_FLAGS += -DSTVID_DEBUG_GET_STATISTICS
endif


# Specific compilation flags
stlayer.lib:\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STLAYER_OPTIONS)

stavmem.lib : STAVMEM_NO_GPDMA :=1
export STAVMEM_NO_GPDMA

# Priority for video is Display > Trickmode > Decode > Error Recovery
stvid.lib:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTVID_TASK_PRIORITY_DECODE=10 \
        -DSTVID_TASK_PRIORITY_DISPLAY=12 -DSTVID_TASK_PRIORITY_ERROR_RECOVERY=9\
        -DSTVID_TASK_PRIORITY_TRICKMODE=11 -DSTVID_HARDWARE_ERROR_EVENT \
        -DSTVID_TASK_PRIORITY_INJECTERS=9 -DSTVID_DEBUG_GET_STATISTICS

stvtg.lib:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTVTG_TASK_PRIORITY_IT_PROCESSING=15 \
        -DSTVTG_NOTIFY_VSYNC_UNDER_IT

sttbx.lib: \
    DVD_CFLAGS += -DSTTBX_NO_UART

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

ifeq ($(FLOATING_POINT), fp)
        LKFLAGS += -fp
endif

# Target
# ------
TARGETS = vid_test.lku

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

stapigat.lib:\
    DVD_CFLAGS += $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT -DSTVID_DEBUG_GET_STATISTICS
    STAPIGAT_INCLUDE := stvid/tests/emulator stpti4 $(STAPIGAT_INCLUDE_PATH)
    STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS)
    export STAPIGAT_INCLUDE
    export STAPIGAT_IMPORTS_ST20

# Build general application for DRAM
vid_test_INIT := board_init
$(TARGETS): $(IMPORT_LIBS) $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) -M ../../map.map

# Build video driver and test application but ignore dependencies
no_imports: stvid.lib $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	$(CC) $(LIBRARIES) -T $(DVD_PLATFORM).cfg \
	$(ST20_LKFLAGS) $(vid_test_LKFLAGS) $(vid_test_ST20_LKFLAGS) \
	$^ $(IMPORT_LIBS) $(OSLIBS) -p $(vid_test_INIT) -o $(TARGETS)

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS) $($(ARCHITECTURE)_TARGETS_OTHER)
ifeq "$(ARCHITECTURE)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(ARCHITECTURE)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(ARCHITECTURE)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif







