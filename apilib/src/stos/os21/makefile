#####################################
# MODULE : os21.lib / libstos21.a #
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

ST20_TARGETS := os21.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST20_EXPORTS :=
ST40_EXPORTS :=

ST20_OBJS :=  os21interrupt.tco os21memory.tco  \
                          os21semaphore.tco     \
                          os21tasks.tco


ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(OS21_SUBDIRS))

# Sub-libs for HAL :
#--------------------------

SUBDIRS     := $(OS21_SUBDIRS)

# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$()
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

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

