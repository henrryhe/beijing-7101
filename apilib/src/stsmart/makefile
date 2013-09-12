DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
COMPONENT_NAME := stsmart
HEADER_IMPORTS :=
IMPORTS := stpio stuart stcommon sttbx stevt stos
EXPORTS := $(COMPONENT_NAME).h $(LIB_PREFIX)$(COMPONENT_NAME)$(LIB_SUFFIX)
TARGETS := $(LIB_PREFIX)$(COMPONENT_NAME)$(LIB_SUFFIX)

ifndef ST40_TARGETS
  ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
  ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
  ST40_IMPORTS = $(IMPORTS)
endif

ifndef ST200_TARGETS
  ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
  ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
  ST200_IMPORTS = $(IMPORTS)
endif

OBJS := scapi$(OBJ_SUFFIX) scdrv$(OBJ_SUFFIX) sct0$(OBJ_SUFFIX) sct1$(OBJ_SUFFIX) scio$(OBJ_SUFFIX)

# Options:
#
# STSMART_DEBUG - turn on trace debug information
# STSMART_DEBOUNCE_TIMEOUT - number of clock ticks used to delay before
#                            sampling card presence pio pin.  The value
#                            is defined in the driver file but may be
#                            overridden.

ifdef STSMART_DEBUG
  CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_REPORT -DSTSMART_DEBUG
endif

ifdef STSMART_DEBOUNCE_TIMEOUT
  CFLAGS += -DSMART_DEBOUNCE_TIMEOUT=$(STSMART_DEBOUNCE_TIMEOUT)
endif

ifdef SMART_T1SIMTEST
  CFLAGS += -DSMART_T1TEST
endif

ifdef STSMART_DETECT_INVERTED
  CFLAGS += -DSTSMART_DETECT_INVERTED
endif

ifdef STSMART_EVENTMGR_TASK_PRIORITY
  CFLAGS += -DSTSMART_EVENTMGR_TASK_PRIORITY=$(STSMART_EVENTMGR_TASK_PRIORITY)
endif

ifdef STSMART_WARM_RESET
    CFLAGS += -DSTSMART_WARM_RESET
endif    

ifdef DVD_USE_GLOBAL_PRIORITIES
    CFLAGS += -DDVD_USE_GLOBAL_PRIORITIES
endif

ifdef STSMART_INTERFACE_ST8024
    CFLAGS += -DSTSMART_INTERFACE_ST8024
endif

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

$(LIB_PREFIX)$(COMPONENT_NAME)$(LIB_SUFFIX): $(OBJS)
	$(BUILD_LIBRARY)

clean: 
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(TARGETS) $(OBJS)

else

include $(DVD_MAKE)/builddir.mak

endif

# end
