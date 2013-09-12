#**********************************************************************
#
#           Makefile for Ref sync application 
#           ********** Linux ONLY ***********
#
#**********************************************************************

USER_WRAPPER=1
KERNEL_MODE=

export KERNEL_MODE
export USER_WRAPPER

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
include $(DVD_MAKE)/generic.mak

APP_NAME := ref_sync

HEADER_IMPORTS := stsys stapigat/src stvid/tests

IMPORTS := stos stcommon stevt testtool stdenc
IMPORTS += stgxobj stvtg stvmix stlayer stvout stclkrv 

IMPORTS += $(DVD_TRANSPORT)
IMPORTS += stmerge

ifneq ($(STVID_ENABLE_AUDIO),)
   IMPORTS += staudlx
   USE_AUDIO = TRUE
endif

# Add STVID as last IMPORTS
IMPORTS += stvid

ST40_IMPORTS = $(IMPORTS)


# Sources of the components
# -------------------------

# Object for LINUX
ST40_OBJS := $(APP_NAME).o
ST40_OBJS += ref_init.o ref_term.o

# Flags and options
# -----------------
STAPIGAT_FLAGS :=
STTBX_FLAGS :=

SUBDIRS += $(VIDTEST_MODULES)
STAPIGAT_INCLUDE_PATH := $(addprefix -I stvid/tests/src/,$(VIDTEST_MODULES))

STAPIGAT_INCLUDE_PATH += stfdma
STAPIGAT_INCLUDE_PATH += stvid/tests
STAPIGAT_INCLUDE_PATH += stvid/tests/integration/zapsync

STAPIGAT_FLAGS += -DSTTBX_NO_UART

# For compilation of pti+link or stpti
CFLAGS += $(OPTIONAL_CFLAGS)

# for stavmem kernel mode
CFLAGS += -DAVMEM_KERNEL_MODE

ifneq ($(USE_AUDIO),)
    CFLAGS += -D USE_AUDIO
    STAPIGAT_FLAGS += -D USE_AUDIO
endif

STTBX_FLAGS += -DSTTBX_NO_UART
STAPIGAT_FLAGS += -DSTTBX_NO_UART

# DVD_SERVICE specific
CFLAGS += -D STPTI_DVB_SUPPORT
STAPIGAT_FLAGS += -D STPTI_DVB_SUPPORT
STPTI_OPTIONS += -D STPTI_DVB_SUPPORT
STCLKRV_OPTIONS += -D STPTI_DVB_SUPPORT

# CLKRV
STAPIGAT_FLAGS += -DSTVTG_USE_CLKRV

# Specific compilation flags
# --------------------------
# Specific compilation flags
stapigat.lib libstapigat.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
	STAPIGAT_INCLUDE := stvid/tests/src $(STAPIGAT_INCLUDE_PATH)
	STAPIGAT_IMPORTS_ST20 := $(IMPORTS)
	STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS)
	STAPIGAT_IMPORTS_ST20 += dumpreg
	
export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40

$(DVD_TRANSPORT).lib lib$(DVD_TRANSPORT).a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STPTI_OPTIONS)
        STPTI_CONFIG := $(STPTI_OPTIONS)

stclkrv.lib libstclkrv.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCLKRV_OPTIONS)

DVD_CFLAGS := $(DVD_CFLAGS) -D$(LINUX_ARCHITECTURE)

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS
export STPTI_CONFIG

# For output of toolbox
CFLAGS += -DSTTBX_PRINT -DSTTBX_INPUT

# For warnings
CFLAGS += -Wall

# Target
# ------

ST20_TARGETS = $(APP_NAME).lku
ST40_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

$($(ARCHITECTURE)_TARGETS): $(IMPORT_LIBS) $($(ARCHITECTURE)_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

imports:
	@$(ECHO) IMPORTS: $(IMPORTS)
	@$(ECHO) ST40_IMPORTS: $(ST40_IMPORTS)
	@$(ECHO) IMPORT_LIBS: $(IMPORT_LIBS)

no_imports: $(VIDEOLIB) $($(ARCHITECTURE)_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	$(CC) $(LIBRARIES) -T $(DVD_PLATFORM).cfg \
        $($(ARCHITECTURE)_LKFLAGS) $(vid_test_LKFLAGS) $(vid_test_$(ARCHITECTURE)_LKFLAGS) \
        $^ $(IMPORT_LIBS) $(OSLIBS) -p $(vid_test_INIT) -o $($(ARCHITECTURE)_TARGETS)

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS) $($(ARCHITECTURE)_TARGETS_OTHER)
	-$(RM) ..\..\vid_test.map

else

include $(DVD_MAKE)/builddir.mak

endif
