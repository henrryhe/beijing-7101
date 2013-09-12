#######################################################################
# Makefile for STBURNER
#
# Board Options
#   set DVD_PLATFORM mb231/mb275/mb282b
#
# Type
#   'make' to build applic
#   'make run TARGET=<jei-name>' to run applic
#   'make debug TARGET=<jei-name>' to run applic with GUI Debugger
#   'make clean' to remove dependancies
#   'make clean_all' to remove all driver dependancies
#######################################################################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

TARGETS := stburner.lku

# Other components this component is dependant upon

ST20_IMPORTS := stboot stcommon sttbx stflash stpio stuart

#----------------------------------------------------------------------
# objects to be built

ST20_OBJS := stflasht_burner.tco 

#----------------------------------------------------------------------
# Reporting 
CFLAGS := $(CFLAGS) -DSTTBX_PRINT

#----------------------------------------------------------------------
# Build targets

include $(DVD_MAKE)/defrules.mak

stburner.lku : $(ST20_OBJS) $(IMPORT_LIBS)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)

#######################################################################
# Run targets
#######################################################################

clean:
ifeq "$(ARCHITECTURE)" "ST20"
	-$(RM) *.lku
	-$(RM) *.dbg
	-$(RM) *.tco
endif
# DO NOT DELETE

else

include $(DVD_MAKE)/builddir.mak

endif

# End