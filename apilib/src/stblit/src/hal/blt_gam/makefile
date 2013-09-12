#########################
# MODULE : gamma.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Debug only : uncomment and update the lines :
#----------------------------------------------

CFLAGS := $(CFLAGS) $(BLT_DEFS)

ifeq "$(DVD_OS)" "LINUX"
ifdef BLIT_FULL_USER
INCLUDE_PATH +=  ../../../../../linux/stblit_ioctl
endif
endif

INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../..

ifeq "$(STBLIT_TEST_FRONTEND)" "TRUE"
CFLAGS := $(CFLAGS) -I../blt_emu -I./ -DSTBLIT_TEST_FRONTEND
endif

ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    ifeq "$(DVD_OS)" "OS40"
        HEADER_IMPORTS += os20emu
    endif
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
CFLAGS := $(CFLAGS) -DSTAPI_INTERRUPT_BY_NUMBER
endif


# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := blt_init.tco blt_be.tco blt_ctl.tco \
blt_pool.tco blt_mem.tco\
blt_job.tco blt_fe.tco blt_slc.tco blt_tab.tco blt_sw.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))


ST20_TARGETS := blt_gam.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

LINUX_IMPORTS := stavmem stos stevt stblit stcommon stgxobj



ifneq "$(ST40_OS)" "OS21"
	ST40_IMPORTS := $(ST20_IMPORTS) os20emu
else
	ST40_IMPORTS := $(ST20_IMPORTS)
endif

ifeq "$(DVD_OS)" "OS21"
ST40_IMPORTS := $(ST40_IMPORTS) stos
endif

ifeq "$(DVD_OS)" "OS20"
ST20_IMPORTS := $(ST20_IMPORTS) stos
endif

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
endif
ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
ifdef BLIT_FULL_USER
$(LINUX_TARGETS): $(LINUX_OBJS)  ../../../../../linux/stblit_ioctl/stblit_ioctl_lib.o
	$(BUILD_LIBRARY)
else
$(LINUX_TARGETS): $(LINUX_OBJS)
	$(BUILD_LIBRARY)
endif
endif




# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning Gamma
	-$(RM) $($(OBJECT_DIRECTORY)_OBJS)
	-$(RM) $($(OBJECT_DIRECTORY)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif


########################

