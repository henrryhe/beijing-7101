#########################################
# MODULE : testtool.lib / libtesttool.a #
#########################################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Dependencies
COMPONENT_NAME := testtool

ST20_IMPORTS = stos stpio stuart sttbx stcommon
ST40_IMPORTS = $(ST20_IMPORTS)
ST200_IMPORTS = $(ST20_IMPORTS)
LINUX_IMPORTS = stos stcommon sttbx

# Define the targets
#-------------------
ST20_TARGETS := $(COMPONENT_NAME).lib
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))


ST20_EXPORTS := $(COMPONENT_NAME).h $(COMPONENT_NAME).lib
LINUX_EXPORTS := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))


# local objects which comprise this component
ST20_OBJS = testtool.tco clilib.tco
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))



# Remove echo when Posix is used
ifneq ($(DVD_POSIX),)
        CFLAGS += -D CONFIG_POSIX
endif

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------

ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif
ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	@$(ECHO) ST40_Objects $(ST40_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	@$(ECHO) ST200_Objects $(ST200_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS)
	@$(ECHO) LINUX_Objects $(LINUX_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif


# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	$(RM) $($(BUILDING_FOR)_OBJS)
	$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
testtool.tco testtool.o: clilib.h
clilib.tco clilib.o: clilib.h

#end
