DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
COMPONENT_NAME := stnet
HEADER_IMPORTS := stinj stmerge $(DVD_TRANSPORT)
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

ifdef STNET_COMPLETE_SUPPORT
CFLAGS += -DSTNET_COMPLETE_SUPPORT
endif

# For OS21 receiver, STACK bypass is the default rule
ifneq "$(DVD_OS)" "LINUX"
CFLAGS += -DOSPLUS_INPUT_STREAM_BYPASS
endif

# OPSPLUS include path
ifdef OSPLUS_ROOT
 CFLAGS += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(OSPLUS_ROOT)/include)
 CFLAGS += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(OSPLUS_ROOT)/include/nexgen)
 CFLAGS += -DNG_COMP_SH4GCC -DNG_RTOS -DNG_CPU_$(ARCHITECTURE) -D_POSIX_SOURCE
endif

ifdef STNET_DEBUG
 CFLAGS += -DSTTBX_REPORT -DSTTBX_PRINT
endif

ifdef STNET_TRACE
  CFLAGS += -DENABLE_TRACE
endif

OBJS := stnet$(OBJ_SUFFIX)
ifeq "$(DVD_OS)" "LINUX"
OBJS += net_ioctl$(OBJ_SUFFIX)
endif

ifdef STNET_COMPLETE_SUPPORT
OBJS += net_receiver$(OBJ_SUFFIX) net_transmitter$(OBJ_SUFFIX) lists_manager$(OBJ_SUFFIX)
ifneq "$(DVD_OS)" "LINUX"
OBJS += nginit$(OBJ_SUFFIX)
endif
endif
# lists_manager.o

# Options:
#

INCLUDE_PATH += $(DVD_BUILD_DIR)/..

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
