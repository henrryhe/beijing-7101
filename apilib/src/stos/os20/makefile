#####################################
# MODULE : os20.lib / libstos20.a #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := stos
ST20_IMPORTS :=
ST40_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS := os20.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST20_EXPORTS :=
ST40_EXPORTS :=

ST20_OBJS :=  os20interrupt.tco os20memory.tco  \
                          os20semaphore.tco     \
                          os20tasks.tco


ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(OS20_SUBDIRS))

# Sub-libs for HAL :
#--------------------------

SUBDIRS     := $(OS20_SUBDIRS)

# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
#$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
#        $()

ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
endif


# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)



subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies

