#########################
# MODULE : stillpic.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Concat include path 
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../api 

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := still_cm.tco still_vp.tco stillhal.tco stilutil.tco
ST20_TARGETS := stillpic.lib

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
	@$(ECHO) Cleaning Still Picture
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)


# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif
