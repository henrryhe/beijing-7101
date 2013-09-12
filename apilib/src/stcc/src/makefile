#####################################
# MODULE : stcc.lib / libstcc.a #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := stos stcc
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

# DTV Decoder / Rendering linked as a sub-lib
#--------------------------------------------
ifeq  "$(DVD_DTV_CC)" "TRUE"
    SUBDIRS := dtv_cc
    CFLAGS :=  -DDTV_CC $(CFLAGS)
else
    SUBDIRS :=
endif



ST20_TARGETS := stcc.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))


ST20_OBJS    := ccslots.tco parsers.tco settings.tco stccinit.tco stcctask.tco
ST200_OBJS    := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------

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
ccslots.tco ccslots.o: stccinit.h ccslots.h
parsers.tco parsers.o: stccinit.h ccslots.h parsers.h
settings.tco settings.o: stccinit.h ccslots.h
stccinit.tco stccinit.o: stccinit.h stcctask.h
stcctask.tco stcctask.o: stccinit.h ccslots.h parsers.h stcctask.h

