#########################
# MODULE : osdtopbt.lib #
#########################

# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak
 
# concat include path 
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../api 


# local objects which comprise this component
#---------------------------------------------
ST20_OBJS =  osd_list.tco osd_cm.tco osd_hal.tco osd_vp.tco \
                osd_task.tco osd_util.tco
ST20_TARGETS = osdtopbt.lib

# Pick up standars targets
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib         
#---------------
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)

# Clean .lib and obj    
#-------------------
clean:
	@$(ECHO) Cleaning OSD Top Bottom
	$(RM) $($(ARCHITECTURE)_OBJS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif
