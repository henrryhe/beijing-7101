DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
ifeq "$(DVD_OS)" "OS40"
IMPORTS += os20emu
endif

ifneq (,$(findstring $(DVD_FRONTEND),5514 5516 5517 5518 5528 7710 7200))
IMPORTS += sttbx stos
else
IMPORTS += stpio stspi stos
FLASH_CFLAGS += -DSTFLASH_SPI_SUPPORT
endif

ifdef STFLASH_DSV_SUPPORT
EXPORTS := stflash.h stdsv.h $(LIB_PREFIX)stflash$(LIB_SUFFIX)
else
EXPORTS := stflash.h $(LIB_PREFIX)stflash$(LIB_SUFFIX)
endif

# local objects which comprise this component
ifdef STFLASH_KRYPTO_SUPPORT
OBJS = stflash$(OBJ_SUFFIX) stkrypto$(OBJ_SUFFIX) hal$(OBJ_SUFFIX)
else
OBJS = stflash$(OBJ_SUFFIX) hal$(OBJ_SUFFIX)
endif

ifdef STFLASH_DSV_SUPPORT
OBJS = stflash$(OBJ_SUFFIX) stdsv$(OBJ_SUFFIX) hal$(OBJ_SUFFIX)
else
OBJS = stflash$(OBJ_SUFFIX) hal$(OBJ_SUFFIX)
endif


TARGETS := $(LIB_PREFIX)stflash$(LIB_SUFFIX)

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

ifdef STFLASH_FAST_WRITE
    FLASH_CFLAGS += -DSTFLASH_FAST_WRITE
endif

ifdef STFLASH_MULTIBANK_SUPPORT
    FLASH_CFLAGS += -DSTFLASH_MULTIBANK_SUPPORT
endif

CFLAGS += $(FLASH_CFLAGS)

# if using os40, add os20emu to header imports
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

include $(DVD_MAKE)/defrules.mak

$(TARGETS): $(OBJS)
	$(BUILD_LIBRARY)

clean:
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $(OBJS)

# Local header dependencies
else
include $(DVD_MAKE)/builddir.mak
endif
# End