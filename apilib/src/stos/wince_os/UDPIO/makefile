# Sample ST20 makefile

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

COMPONENT_NAME := UDPIO
ifeq "$(DVD_OS)" "OS40"
 ST40_IMPORTS := os20emu
endif
ST20_TARGETS := $(COMPONENT_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST20_EXPORTS := 
ST40_EXPORTS :=  

ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

# Local objects which comprise this component
ST20_OBJS =  StapiConsoleApi.tco StapiConsoleDrvUDP.tco UDP_IO.tco
CFLAGS += 
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

include $(DVD_MAKE)/defrules.mak

$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)

clean: 
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)


# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End
