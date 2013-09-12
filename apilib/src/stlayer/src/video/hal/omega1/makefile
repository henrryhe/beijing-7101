#########################
# MODULE : omega1.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Concat include path
#---------------------
INCLUDE_PATH += $(DVD_BUILD_DIR)/..

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := hv_layer.tco
ST20_TARGETS := omega1.lib

# compil flags, objs and clean_cell for each chips
#-------------------------------------------------
ifeq "$(DVD_BACKEND)" "5510"
SUBDIRS := 5510-12
endif
ifeq "$(DVD_BACKEND)" "5512"
SUBDIRS := 5510-12
endif
ifeq "$(DVD_BACKEND)" "5508"
SUBDIRS := 551-4678
endif
ifeq "$(DVD_BACKEND)" "5518"
SUBDIRS := 551-4678
endif
ifeq "$(DVD_BACKEND)" "5514"
SUBDIRS := 551-4678
endif
ifeq "$(DVD_BACKEND)" "5516"
SUBDIRS := 551-4678
endif
ifeq "$(DVD_BACKEND)" "5517"
SUBDIRS := 551-4678
endif
ifeq "$(DVD_BACKEND)" "5578"
SUBDIRS := 551-4678
endif
# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning Omega1
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

