#########################
# MODULE : gxobj_test.lku #
#########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS := stsys stapigat/src/patch/ stapigat/src
ifeq "$(DVD_OS)" "LINUX"
HEADER_IMPORTS += stapigat/src/linux
endif

ST20_IMPORTS := stos stcommon stboot stavmem sttbx testtool stgxobj



ST20_OBJS := gxobj_test.tco api_cmd.tco startup.tco
ifneq "$(DVD_OS)" "LINUX"
ST20_OBJS += clavmem.tco cltbx.tco cltst.tco clboot.tco
endif


TARGET_WITH_STCFG = 5514
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STCFG))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stgpdma stevt
  ST20_OBJS += clgpdma.tco clevt.tco
endif


ST200_IMPORTS = $(ST20_IMPORTS)

# Object for st40

ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

ifeq "$(DVD_OS)" "LINUX"
LINUX_IMPORTS := stos stcommon testtool stgxobj stapigat stavmem
ST20_OBJS += cltst.tco
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

  # STAVMEM is special in linux - no library is created so we must not add
  # the library to the link command. The following link option suppresses this.
  # This only works currently on LINUX.
  LINUX_libstavmem.a_NOLINK := 1

endif



ST40_IMPORTS  = $(ST20_IMPORTS)
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


# For output of toolbox
CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT -D STTBX_NO_UART
DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART
ifeq "$(DVD_BACKEND)" "7020"
stboot.lib : DVD_CFLAGS := $(DVD_CFLAGS) -DSTBOOT_DISABLE_RAM_TESTS
endif

ifeq "$(DVD_OS)" "LINUX"
DVD_CFLAGS := $(DVD_CFLAGS)  -DSTTBX_PRINT
endif

#
CFLAGS += $(CFLAGS) -DSTB_FRONTEND_$(DVD_FRONTEND) -DSTB_BACKEND_$(DVD_BACKEND)
FLAGS_FOR_SUBDIRS := -DSTB_FRONTEND_$(DVD_FRONTEND) -DSTB_BACKEND_$(DVD_BACKEND)


# debug / cache related flags
ifneq ($(DEBUG),)
  ifneq "$(BUILDING_FOR)" "ST200"
    STGXOBJ_ICACHE_DISABLED := 1
  endif
  STGXOBJ_DCACHE_DISABLED := 1
endif
ifeq ($(STGXOBJ_DCACHE_DISABLED),)
  CFLAGS := $(CFLAGS) -D DCACHE_ENABLE
endif
ifeq ($(STGXOBJ_ICACHE_DISABLED),)
  CFLAGS := $(CFLAGS) -D ICACHE_ENABLE
endif

stavmem.lib libstavmem.a: \
        STAVMEM_NO_GPDMA=1
        STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA

ifeq "$(DVD_OS)" "LINUX"
stapigat.lib libstapigat.a: \
    DVD_CFLAGS :=$(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stgxobj/tests/src  $(LINUX_IMPORTS)
    STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST40
endif

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

# Targets
ST20_TARGETS  := gxobj_test.lku
ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))



LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))
ST40_TARGETS  := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))


#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build general application for DRAM
gxobj_test_INIT := board_init
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)  $(IMPORT_LIBS)
	$(LINK_EXECUTABLE)
endif
ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(IMPORT_LIBS) $(ST40_OBJS)
	$(LINK_EXECUTABLE)
endif
ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(IMPORT_LIBS) $(ST200_OBJS)
	$(LINK_EXECUTABLE)
endif
ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(IMPORT_LIBS) $(LINUX_OBJS)
	$(LINK_EXECUTABLE)
endif


clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS) $($(BUILDING_FOR)_TARGETS_OTHER)
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample ST20/ST40/ST200 makefile
