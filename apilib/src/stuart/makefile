DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

## For OS40 we use os20emu -- this must be added as a header import to
## avoid problems when performing a tree build
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

IMPORTS := stpio stos stfdma
EXPORTS := stuart.h $(LIB_PREFIX)stuart$(LIB_SUFFIX)
TARGETS := $(LIB_PREFIX)stuart$(LIB_SUFFIX)

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

OBJS := stuart$(OBJ_SUFFIX) uartdrv$(OBJ_SUFFIX) uarttimer$(OBJ_SUFFIX) 


ifdef STUART_DMA_SUPPORT
OBJ += uartdma$(OBJ_SUFFIX)
endif

# Build options
ifdef STUART_STATISTICS
CFLAGS += -DSTUART_STATISTICS=$(STUART_STATISTICS)
endif

# Build options
ifdef STUART_TIMER_TASK_PRIORITY
CFLAGS += -DSTUART_TIMER_TASK_PRIORITY=$(STUART_TIMER_TASK_PRIORITY)
endif

ifdef STUART_TIMER_STACK_SIZE
CFLAGS += -DSTUART_TIMER_STACK_SIZE=$(STUART_TIMER_STACK_SIZE)
endif

# Build options
ifdef STAPI_INTERRUPT_BY_NUMBER
CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER=$(STAPI_INTERRUPT_BY_NUMBER)
endif

include $(DVD_MAKE)/defrules.mak
$(LIB_PREFIX)stuart$(LIB_SUFFIX): $(OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(OBJS)
	-$(RM) $(TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif
