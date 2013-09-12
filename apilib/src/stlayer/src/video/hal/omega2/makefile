#########################
# MODULE : omega2.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Concat include path
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/..

# Local objects which comprise this component
#---------------------------------------------
ST20_TARGETS := omega2.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

ST20_OBJS := hv_layer.tco hv_filt.tco
ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

IMPORTS := sttbx stevt stavmem stgxobj stvtg
ST40_IMPORTS := $(IMPORTS)

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)

# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning Omega2
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
hv_layer.tco hv_layer.o : hv_layer.c hv_layer.h layer2.h hv_reg.h filters.h \
                          hv_filt.h

hv_filt.tco hv_filt.o : hv_filt.c hv_filt.h layer2.h hv_reg.h

