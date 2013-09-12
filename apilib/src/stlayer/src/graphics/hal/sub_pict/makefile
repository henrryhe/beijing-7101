########################
# MODULE : subtpic.lib #
########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

#Debug only : uncomment and update the lines : 
#----------------------------------------------
#CFLAGS :=  -c2 -c -O0 -g -DSTTBX_REPORT  -DARCHITECTURE_ST20
#CFLAGS +=   -mb231  -DST_5510

# local objects which comprise this component
#---------------------------------------------
ST20_OBJS := sub_vp.tco sub_pool.tco sub_spu.tco sub_reg.tco \
             sub_init.tco sub_copy.tco
ST20_TARGETS := sub_pict.lib

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
	@$(ECHO) Cleaning Sub Picture
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif
