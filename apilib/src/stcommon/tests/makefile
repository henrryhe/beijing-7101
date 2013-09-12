# Common test harness

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS := stsys
ifeq "$(DVD_BUILDING_IN_VOB)" "false" 
    ifeq "$(DVD_OS)" "OS40" 
        HEADER_IMPORTS += os20emu 
    endif 
endif 
ifndef DISABLE_TOOLBOX
 ST20_IMPORTS := stos stcommon stboot sttbx
 ifndef STTBX_NO_UART
     ST20_IMPORTS += stuart stpio
 endif
else
 ST20_IMPORTS := stos stcommon stboot
endif

ifeq "$(DVD_OS)" "OS40"
 ST40_IMPORTS := $(ST20_IMPORTS) os20emu
else
 ST40_IMPORTS := $(ST20_IMPORTS)
endif
ST200_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS = stcomtst.lku
ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST200_TARGETS = $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))

# Local objects which comprise this component
ST20_OBJS := revtest.tco clocktst.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

# Enable toolbox output
ifndef DISABLE_TOOLBOX
 CFLAGS += -DSTTBX_PRINT
else
 CFLAGS += -DDISABLE_TOOLBOX
endif

ifdef DISABLE_ICACHE
    CFLAGS := $(CFLAGS) -D DISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
    CFLAGS := $(CFLAGS) -D DISABLE_DCACHE
endif

include $(DVD_MAKE)/defrules.mak

#$(ST20_TARGETS): $(ST20_OBJS) $(IMPORT_LIBS)
#	@$(ECHO) Linking $@
#	$(LINK_EXECUTABLE)

#$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS)
#	@$(ECHO) Linking $@
#	$(LINK_EXECUTABLE)
	
#$(ST200_TARGETS): $(ST200_OBJS) $(IMPORT_LIBS)
#	@$(ECHO) Linking $@
#	$(LINK_EXECUTABLE)

stcomtst$(EXE_SUFFIX): $($(ARCHITECTURE)_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) *.dbg
    

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End
