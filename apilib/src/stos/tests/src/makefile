#########################
# STDENC tests makefile #
#########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stos/tests/linux/stostest_ioctl

COMMON_IMPORTS :=  stcommon stos

ST20_IMPORTS := $(COMMON_IMPORTS) stboot sttbx
ST40_IMPORTS := $(COMMON_IMPORTS) stboot sttbx
ST200_IMPORTS := $(COMMON_IMPORTS) stboot sttbx
LINUX_IMPORTS := $(COMMON_IMPORTS)

# Object for ST20
ST20_OBJS := os_test.tco
ST20_OBJS += memory.tco
ST20_OBJS += tasks.tco
ST20_OBJS += regions.tco
ST20_OBJS += semaphores.tco
ST20_OBJS += mutex.tco


ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

# Object for ST200
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)

# For output of toolbox
CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT -D STTBX_NO_UART
DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART -DSTTBX_PRINT

ifeq "$(DVD_OS)" "LINUX"
SUBDIRS := ../linux
endif

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

# Target
ST20_TARGETS = stos_test.lku
ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))

LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak


STOSTEST_IMPORTS = stostest
IMPORT_STOSTEST_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(STOSTEST_IMPORTS)))
$(IMPORT_STOSTEST_LIBS):
	@$(ECHO) Adding library for final linking: $@


#Add Flag for map file
ifeq "$(BUILDING_FOR)" "ST20"
LKFLAGS= -M $(basename+ $@).map
endif

ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(IMPORT_LIBS) $(ST20_OBJS)  $(SUBDIR_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(IMPORT_LIBS) $(ST40_OBJS)  $(SUBDIR_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif


ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(IMPORT_LIBS) $(ST200_OBJS)  $(SUBDIR_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(IMPORT_LIBS) $(LINUX_OBJS)  $(SUBDIR_LIBS) $(IMPORT_STOSTEST_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
	$(CP) $(LINUX_TARGETS) $(DVD_EXPORTS)/bin/$(LINUX_TARGETS).exe
endif

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS) $($(BUILDING_FOR)_TARGETS_OTHER)
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample ST20 makefile


