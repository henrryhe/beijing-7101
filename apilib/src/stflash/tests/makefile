DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# if using os40, add os20emu to header imports
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

ifdef TEST_BOOT_FROM_FLASH
HEX_NAME := flash
endif

ifdef STFLASH_DSV_SUPPORT
CFLAGS += -DSTFLASH_DSV_SUPPORT
endif

ifdef STFLASH_MULTIBANK_SUPPORT
CFLAGS += -DSTFLASH_MULTIBANK_SUPPORT
endif

ifneq (,$(findstring $(DVD_FRONTEND),5514 5516 5517 5518 5528 7710 7200))
else
IMPORTS += stspi
CFLAGS  += -DSTFLASH_SPI_SUPPORT
endif

# Other components this component is dependant upon
ifdef USE_TESTTOOL
ifdef STTBX_NO_UART
IMPORTS += stflash stboot stpio stcommon testtool stos
else
IMPORTS += stflash stboot stuart stpio stcommon testtool stos
endif
else # use_testtool not defined
ifdef STTBX_NO_UART
IMPORTS += stflash stboot stpio stcommon stos
else
IMPORTS += stflash stboot stuart stpio stcommon stos
endif
endif

ifndef DISABLE_TOOLBOX
IMPORTS += sttbx
endif

ifeq "$(DVD_OS)" "OS40"
IMPORTS +=  os20emu
endif

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

ifdef DISABLE_TOOLBOX
TESTDEFS += -DDISABLE_TOOLBOX
else
TESTDEFS += -DSTTBX_PRINT
endif

ifdef USE_TESTTOOL
CFLAGS += -DUSE_TESTTOOL
endif

ifdef TEST_BOOT_FROM_FLASH
CFLAGS += -DTEST_BOOT_FROM_FLASH
endif

ifdef STFLASH_DEBUG
CFLAGS += -DSTFLASH_DEBUG
endif

# Look-up table of platforms & supported flash
ifeq ($(strip $(FLASH_TEST)),)
FLASH_SUPPORTED := :mb282:M29W800T :mb282b:M29W800T :mb275:M29W800T\
          	   :mb314:E28F640 :mb5518:AM29LV160D :mb317a:E28F640 :mb317b:E28F640 \
		   :mediaref:E28F640 :mb361:M58LW032 :mb382:M58LW064D :mb376:M58LW064D \
		   :mb390:M58LW064D :mb394:M29W320DT :espresso:M58LW064D :mb391:M58LW064D\
                   :mb400:M58LW064D :mb411:M58LW064D :mb421:M58LW064D :maly2:M29W800T \
                   :mb426:M29W640D :maly3s:M29W800T :maly2:M29W800T :mb457:M25P80 :mb428:M58LW064D \
                   :mb436:M58LW064D :mb519:M58LT256GS
FLASH_TEST := $(strip \
                    $(foreach i,$(FLASH_SUPPORTED),\
                      $(if $(findstring :$(DVD_PLATFORM):,$(i)),\
                        $(subst :$(DVD_PLATFORM):,,$(i)),)))
endif


ifeq "$(DVD_PLATFORM)" "mb457"
OBJS    = sflashtst$(OBJ_SUFFIX)
TARGETS = sflashtst$(EXE_SUFFIX)
endif

ifneq (,$(findstring $(FLASH_TEST),M25P10 M25P16 M25P80))
OBJS    = sflashtst$(OBJ_SUFFIX)
TARGETS = sflashtst$(EXE_SUFFIX)
else
OBJS    = stflasht$(OBJ_SUFFIX)
TARGETS = stflasht$(EXE_SUFFIX)
endif

# Set compile flags according to $FLASH_TEST
TEST_FLAGS = -DFLASH_TEST_$(FLASH_TEST)


ifndef USE_TESTTOOL
ifdef FLASH_FULLERW
    CFLAGS += -DFLASH_FULLERW
endif
endif

ifdef FLASH_FAST_READ
    CFLAGS += -DFLASH_FAST_READ
endif

ifdef DISABLE_ICACHE
    CFLAGS += -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
    CFLAGS += -DDISABLE_DCACHE
endif


CFLAGS := $(CFLAGS) $(TEST_FLAGS) $(TESTDEFS)

# Objects
ifdef TEST_BOOT_FROM_FLASH

ifdef STFLASH_DSV_SUPPORT
OBJS = dsv_test$(OBJ_SUFFIX) trace$(OBJ_SUFFIX)
else
OBJS = pio_test$(OBJ_SUFFIX) trace$(OBJ_SUFFIX)
endif

endif


include $(DVD_MAKE)/defrules.mak

$(TARGETS): $(OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

ifdef TEST_BOOT_FROM_FLASH
#make hex file
$(HEX_NAME)_INIT := board_init_hex
$(HEX_NAME)_LKFLAGS := -romimage -o $(HEX_NAME).hex
$(HEX_NAME).hex: $(OBJS) $(IMPORT_LIBS) $(DVD_BUILD_DIR)/$(DVD_FRONTEND)/initfuncs$(OBJ_SUFFIX)
	         $(ECHO) Building $@
	         $(LINK_EXECUTABLE)
endif

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $(OBJS)
	-$(RM) *.dbg

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End