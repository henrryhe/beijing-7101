#####################
# MODULE : 5512.lib #
##################### 
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Concat include path 
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../..

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := 5512.tco
ST20_TARGETS := 5512.lib

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib         
#---------------
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)

# Clean .lib and obj    
#-------------------
clean:
	@$(ECHO) Cleaning 5512
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)


# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif
