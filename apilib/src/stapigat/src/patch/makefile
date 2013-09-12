######################
# MODULE : patch.lib #
######################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak


# Header dependant components
#----------------------------
# Following for testcfg.h
HEADER_IMPORTS := $(STAPIGAT_INCLUDE)

INCLUDE_PATH := $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../dumpreg
ifeq "$(DVD_OS)" "LINUX"
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../linux
endif

# Remove stapigat from imports
#-----------------------------
ST20_IMPORTS := $(filter-out stapigat,$(STAPIGAT_IMPORTS_ST20))
ST40_IMPORTS := $(filter-out stapigat,$(STAPIGAT_IMPORTS_ST40))
ST200_IMPORTS := $(filter-out stapigat,$(STAPIGAT_IMPORTS_ST200))

# Define the targets
#-------------------
ST20_TARGETS := patch.lib
ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
else
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
endif
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

# Objs for ST20 & ST40
# --------------------
ST20_OBJS := $(OBJS20_USED_IN_PATCH)
ST40_OBJS := $(OBJS40_USED_IN_PATCH)
ST200_OBJS := $(OBJS200_USED_IN_PATCH)
STLINUX_OBJS := $(OBJSLINUX_USED_IN_PATCH)

ifeq "$(DVD_OS)" "LINUX"
ST40_OBJS := $(STLINUX_OBJS)
endif

# For compilation of pti+link or stpti
CFLAGS += $(OPTIONAL_CFLAGS) $(FLAGS_FOR_SUBDIRS)

# for compilation with new STVIN.
ifeq ($(DVD_STVIN),MASTER)
CFLAGS += -D ST_MasterDigInput
endif

# Pick up standars targets
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif
ifeq "$(ARCHITECTURE)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)
endif
clean:
	@$(ECHO) Cleaning patch
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) *$(OBJ_SUFFIX)

else

include $(DVD_MAKE)/builddir.mak

endif

# Local Dependancies

#end of file
