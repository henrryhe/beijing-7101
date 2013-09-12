#########################################
# MODULE : avm_test.lib / libavm_test.a #
#########################################

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Dependencies
HEADER_IMPORTS := stsys stapigat/src stos

ST20_IMPORTS := stapigat stavmem stboot stcommon sttbx testtool
ifeq ($(STAVMEM_NO_GPDMA),)
  ifeq "$(DVD_FRONTEND)" "5514"
    STAVMEM_USE_GPDMA := 1
  endif
  ifeq "$(DVD_FRONTEND)" "5528"
        STAVMEM_USE_GPDMA := 1
  endif

# Use the GPDMA simulator to enable test GPDMA with simulator on target without no GPDMA hardware.
  ifdef STGPDMA_SIM
    STAVMEM_USE_GPDMA := 1

  endif
endif

ifeq ($(STAVMEM_NO_DEBUG_MEMORY_STATUS),)
  ifeq "$(DVD_FRONTEND)" "5514"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5528"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

 ifeq "$(DVD_FRONTEND)" "7100"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5516"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5517"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5100"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "7020"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "TP3"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "7710"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5105"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5301"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "7109"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "7200"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5188"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif
  ifeq "$(DVD_FRONTEND)" "5525"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5107"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif


endif

ifneq ($(STAVMEM_USE_GPDMA),)
  ST20_IMPORTS := $(ST20_IMPORTS) stgpdma stevt
endif

#ifneq ($(STAVMEM_USE_MEMORY_STATUS),)
#   ST20_IMPORTS := $(ST20_IMPORTS)
#endif


ifeq ($(STAVMEM_NO_FDMA),)
  ifeq "$(DVD_FRONTEND)" "5517"
    STAVMEM_USE_FDMA := 1
  endif

  ifeq "$(DVD_FRONTEND)" "5100"
    STAVMEM_USE_FDMA := 1
  endif

  ifeq "$(DVD_FRONTEND)" "7710"
    STAVMEM_USE_FDMA := 1
  endif

  ifeq "$(DVD_FRONTEND)" "7100"
    STAVMEM_USE_FDMA := 1
  endif

  ifeq "$(DVD_FRONTEND)" "7109"
    STAVMEM_USE_FDMA := 1
  endif

  ifeq "$(DVD_FRONTEND)" "7200"
    STAVMEM_USE_FDMA := 1
  endif

  ifeq "$(DVD_FRONTEND)" "5105"
    STAVMEM_USE_FDMA := 1
  endif

  ifeq "$(DVD_FRONTEND)" "5301"
    STAVMEM_USE_FDMA := 1
  endif

  #ifeq "$(DVD_FRONTEND)" "5188"
  #  STAVMEM_USE_FDMA := 1
  #endif

  ifeq "$(DVD_FRONTEND)" "5525"
    STAVMEM_USE_FDMA := 1
  endif

  #ifeq "$(DVD_FRONTEND)" "5107"
  #  STAVMEM_USE_FDMA := 1
  #endif

endif
ifneq ($(STAVMEM_USE_FDMA),)
    ST20_IMPORTS := $(ST20_IMPORTS) stfdma stos
endif

# STI2C is needed for 7020 STEM board
I2C_BACKENDS = 7020
ifeq "$(findstring $(DVD_BACKEND), $(I2C_BACKENDS))" "$(DVD_BACKEND)"
    7020_DIRECT_PLATFORMS = mb290 mb295
    ifneq "$(findstring $(DVD_PLATFORM), $(7020_DIRECT_PLATFORMS))" "$(DVD_PLATFORM)"
        ST20_IMPORTS := $(ST20_IMPORTS) stpio sti2c
    endif
endif

ST40_IMPORTS := stapigat stavmem stboot stcommon sttbx testtool stos


ifneq ($(STAVMEM_USE_GPDMA),)
  ST40_IMPORTS := $(ST40_IMPORTS) stevt stgpdma
endif

ifneq ($(STAVMEM_USE_FDMA),)
  ST40_IMPORTS := $(ST40_IMPORTS) stfdma
endif

ST200_IMPORTS := $(ST20_IMPORTS)

ST20_OBJS := avm_test.tco avm_spd.tco avm_mac.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

# Copy methods target dependent
#------------------------------


ifeq "$(DVD_FRONTEND)" "5510"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5512"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5508"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
  ifeq ($(STAVMEM_NO_1DBM),)
      STAVMEM_USE_1DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5518"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
  ifeq ($(STAVMEM_NO_1DBM),)
      STAVMEM_USE_1DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5514"
# use of GPDMA method already selected at beginning of makefile
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5516"
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5517"
# use of FDMA method already selected at beginning of makefile
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "TP3"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
endif

# Reset stapigat flag
STAPIGAT_FLAGS :=

ifneq ($(STAVMEM_NO_COPY_FILL),)
  CFLAGS := $(CFLAGS) -D STAVMEM_NO_COPY_FILL
endif

# Compilation flags always set
#-----------------------------
ifneq ($(ST_OSWINCE),)
      CFLAGS := $(CFLAGS) -D ST_OSWINCE
endif

ifneq ($(DVD_SECURED_CHIP),)
#    	DVD_CFLAGS := $(DVD_CFLAGS) -D DVD_SECURED_CHIP
      CFLAGS := $(CFLAGS) -D DVD_SECURED_CHIP
else
	ifeq ($(STAVMEM_NO_COPY_FILL),)
		CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_C_STANDARD_FUNCTIONS
	endif
endif
ifneq ($(STAVMEM_NO_STMES),)
      CFLAGS := $(CFLAGS) -D STAVMEM_NO_STMES
endif


ifeq "$(ARCHITECTURE)" "ST200"
	CFLAGS := $(CFLAGS) -D TESTED_PLATFORM=$(DVD_PLATFORM) -D TESTED_CHIP=$(DVD_FRONTEND)
else
	CFLAGS := $(CFLAGS) -D TESTED_PLATFORM=\"$(DVD_PLATFORM)\" -D TESTED_CHIP=\"$(DVD_FRONTEND)\"
endif
#CFLAGS := $(CFLAGS) -D STAVMEM_TESTS_CACHE_OK -D DCACHE_TESTS_SDRAM
# For output of toolbox
CFLAGS := $(CFLAGS) -D STTBX_INPUT -D STTBX_NO_UART
ifneq ($(STAVMEM_DEBUG),)
        CFLAGS := $(CFLAGS) -D STTBX_PRINT
endif

DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART


# Compilation flag chosen
#------------------------
ifeq ($(STAVMEM_NO_COPY_FILL),)
   ifeq ($(DVD_SECURED_CHIP),)
	ifneq ($(STAVMEM_USE_BMP),)
	   CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
	endif

	ifneq ($(STAVMEM_USE_1DBM),)
	   CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
	endif

	ifneq ($(STAVMEM_USE_2DBM),)
	   CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
	endif

	ifneq ($(STAVMEM_USE_GPDMA),)
	  CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_GPDMA
	  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -D STAVMEM_MEM_ACCESS_GPDMA
	endif
   endif
	#ifneq ($(STAVMEM_USE_MEMORY_STATUS),)
	#  CFLAGS := $(CFLAGS) -D STAVMEM_DEBUG_MEMORY_STATUS
	#  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -D STAVMEM_DEBUG_MEMORY_STATUS
	#endif

	ifneq ($(STAVMEM_USE_FDMA),)
	  CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_FDMA
	  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -D STAVMEM_MEM_ACCESS_FDMA
	endif

	ifneq ($(STAVMEM_TEST_DIRECT_PICK),)
	  CFLAGS := $(CFLAGS) -D STAVMEM_TEST_DIRECT_PICK
	endif
endif

# Use the GPDMA simulator
ifneq ($(STGPDMA_SIM),)
  CFLAGS := $(CFLAGS) -D STGPDMA_SIM
endif

ifneq ($(TESTTOOL_BATCH_MODE),)
  CFLAGS := $(CFLAGS) -D TESTTOOL_BATCH_MODE
endif

ifneq ($(DEBUG),)
  STAVMEM_ICACHE_DISABLED := 1
  STAVMEM_DCACHE_DISABLED := 1
endif
ifeq ($(STAVMEM_DCACHE_DISABLED),)
  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -D DCACHE_ENABLE
endif
ifeq ($(STAVMEM_ICACHE_DISABLED),)
  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -D ICACHE_ENABLE
endif
ifeq "$(ARCHITECTURE)" "ST200"
	STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

stapigat.lib libstapigat.a :\
    DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_INPUT
ifneq ($(STAVMEM_DEBUG),)
    DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_PRINT
endif
    STAPIGAT_INCLUDE := stavmem/tests/src
    STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS)
    STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS)
    STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS)

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200

sti2c.lib: DVD_CFLAGS = $(DVD_CFLAGS) -DSTI2C_MASTER_ONLY

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

# Define the targets
#-------------------

ST20_TARGETS := avm_test.lku
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))

# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
avm_test_INIT := board_init
#
BOARDSPECS=
ifeq "$(ARCHITECTURE)" "ST40"
	ifeq "$(DVD_BACKEND)" "7200"
		ifndef MULTICOM_COMPANION_CONFIG
			export MULTICOM_COMPANION_CONFIG=0x07
		endif
		export USE_MBOARD=board_$(DVD_PLATFORM)_lmi0_$(MULTICOM_COMPANION_CONFIG)
		BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7200
	endif
endif

ifeq "$(ARCHITECTURE)" "ST20"
LKFLAGS = -M $(basename $@).map
endif
ifeq "$(ARCHITECTURE)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(ARCHITECTURE)" "ST40"

ifneq ($(DVD_SECURED_CHIP),)
        ifneq ($(ST_OSWINCE),)
                FULLDIR := $(DVD_ROOT)/dvdgr-prj-stavmem
                LDOPTS += $(FULLDIR)/libstmeswince.a
        else
                FULLDIR := $(DVD_ROOT)/dvdgr-prj-stavmem
                LDOPTS += $(FULLDIR)/libstmes.a
        endif
endif

$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(BOARDSPECS)
# -Xlinker -Map=../../avm_test$(MPFX).map :added to generate .map file
endif

ifeq "$(ARCHITECTURE)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $(avm_test_$(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
ifeq "$(ARCHITECTURE)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(ARCHITECTURE)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(ARCHITECTURE)_TARGETS))
endif

# Local header dependencies
avm_test.tco avm_test.o : startup.h avm_test.h clavmem.h
avm_spd.tco avm_spd.o   : avm_test.h avm_spd.h
avm_mac.tco avm_mac.o   : avm_test.h avm_mac.h

else
include $(DVD_MAKE)/builddir.mak
endif

#end
