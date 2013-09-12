##########################
# MODULE : vcodmpg2.lib  #
##########################

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS :=
ST20_TARGETS := vcodmpg2.lib
ST20_IMPORTS := sttbx stevt

ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(ST20_EXPORTS)
ST40_IMPORTS := $(ST20_IMPORTS)

# Sub-libs for vcodmpg2 :
#------------------------
SUBDIRS := $(SUBDIRS) parser
SUBDIRS := $(SUBDIRS) decoder

# General rules
CFLAGS := $(VID_DEFS) $(CFLAGS)

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../ $(DVD_BUILD_DIR)/../ $(DVD_BUILD_DIR)/../../../ $(DVD_BUILD_DIR)/../../../../
# inject path not needed for now, but needed for actual 7100 MPEG decoder DeltaMU
INCLUDE_PATH += ../../../inject
# catch decode module header files for quick mpg2 codec porting based on decode module
#INCLUDE_PATH += ../../../decode

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean_message: FORCE
	@$(ECHO) Cleaning MPG2 Video CODEC $(ST20_TARGETS)

clean: clean_message $(SUBDIR_CLEAN)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

FORCE:


# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

#end of file
