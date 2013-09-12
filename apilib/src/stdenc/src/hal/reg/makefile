##########################################
# MODULE : reg.lib / libreg.a for stdenc #
##########################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Add include path
#---------------------
INCLUDE_PATH += $(DVD_BUILD_DIR)/../..

# Define the targets
#---------------------------------------------
HEADER_IMPORTS := stsys stos stdenc
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif


ST20_IMPORTS := stpio
ST200_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS := reg.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_OBJS := denc_reg.tco
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))


LINUX_IMPORTS := stpio stcommon
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ST40_IMPORTS := $(ST20_IMPORTS) os20emu
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


# Define HW cells present
#--------------------------
ifeq "$(DENC_OPTIONAL_MODULES)" "i2c"
ST20_IMPORTS += sti2c
endif

# Sub-libs :
#-------------------------------------------------
SUBDIRS     := rbus $(DENC_OPTIONAL_MODULES)

# Compile options
#----------------
CFLAGS := $(DENC_DEFS) $(CFLAGS)

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif


ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif


# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
denc_reg.tco denc_reg.o: denc_drv.h
