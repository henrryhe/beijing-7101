DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

IMPORTS := sti2c stpio
EXPORTS := ste2p.h $(LIB_PREFIX)ste2p$(LIB_SUFFIX)
TARGETS := $(LIB_PREFIX)ste2p$(LIB_SUFFIX)

ifndef STE2P_SUPPORTED_DEVICE
STE2P_SUPPORTED_DEVICE=0xA0
endif

CFLAGS += -DSTE2P_SUPPORTED_DEVICE=$(STE2P_SUPPORTED_DEVICE)

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

ifeq "$(DVD_OS)" "OS40"
ST40_IMPORTS := os20emu
endif

ifeq "$(DVD_OS)" "OS40"
ST40_IMPORTS := os20emu
endif

# objects we depend on.
OBJS    := ste2p$(OBJ_SUFFIX) 

include $(DVD_MAKE)/defrules.mak

$(LIB_PREFIX)ste2p$(LIB_SUFFIX): $(OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(OBJS)
	-$(RM) $(TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif
