DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS += stpio stos stpwm 

# Other components this component is dependant upon
ST20_IMPORTS = stpio
ST40_IMPORTS := $(ST20_IMPORTS) os20emu
ST200_IMPORTS := stpio

IMPORTS := stpio stos
EXPORTS := stpwm.h $(LIB_PREFIX)stpwm$(LIB_SUFFIX)
TARGETS := $(LIB_PREFIX)stpwm$(LIB_SUFFIX)

# ST40
ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

# ST200
ifndef ST200_TARGETS
ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
ST200_IMPORTS = $(IMPORTS)
endif

ifdef WA_GNBvd54182
    CFLAGS +=-DWA_GNBvd54182
endif

ifdef WA_GNBvd56512
    CFLAGS +=-DWA_GNBvd56512
endif

# local objects which comprise this component
OBJS := stpwm$(OBJ_SUFFIX)

include $(DVD_MAKE)/defrules.mak

$(LIB_PREFIX)stpwm$(LIB_SUFFIX): $(OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)	 
                    
clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_OBJS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif



