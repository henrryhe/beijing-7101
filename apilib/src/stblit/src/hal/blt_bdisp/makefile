#########################
# MODULE : bdisp.lib #
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
CFLAGS := $(CFLAGS) -I../blt_emu -I./ -D STBLIT_TEST_FRONTEND
endif

ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    ifeq "$(DVD_OS)" "OS40"
        HEADER_IMPORTS += os20emu
    endif
endif

ifeq "$(STBLIT_SLICE_DEMO)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_SLICE_DEMO
endif

ifeq "$(STBLIT_SYNCHRONE_MODE)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_SYNCHRONE_MODE
endif

ifeq "$(STBLIT_USE_COLOR_KEY_FOR_BDISP)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_USE_COLOR_KEY_FOR_BDISP
endif

ifeq "$(STBLIT_ENABLE_BASIC_TRACE)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_ENABLE_BASIC_TRACE
endif

ifeq "$(STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_ENABLE_DATA_POOL_FREE_ELEMENT_CHECK
ifdef STBLIT_FREE_ELEMENT_TIME_OUT
    DVD_CFLAGS +=-DSTBLIT_FREE_ELEMENT_TIME_OUT=$(STBLIT_FREE_ELEMENT_TIME_OUT)
endif
endif

ifeq "$(STBLIT_HW_BLITTER_RESET_DEBUG)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_HW_BLITTER_RESET_DEBUG
ifdef STBLIT_HW_BLITTER_RESET_CHECK_TIME_OUT
    DVD_CFLAGS +=-DSTBLIT_HW_BLITTER_RESET_CHECK_TIME_OUT=$(STBLIT_HW_BLITTER_RESET_CHECK_TIME_OUT)
endif
endif

ifeq "$(STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION
endif

ifeq "$(STBLIT_DEBUG_GET_STATISTICS)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_DEBUG_GET_STATISTICS
endif

ifeq "$(STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
endif

ifeq "$(STBLIT_USE_MEMORY_TRACE)" "TRUE"
CFLAGS := $(CFLAGS) -D STBLIT_USE_MEMORY_TRACE
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
CFLAGS := $(CFLAGS) -DSTAPI_INTERRUPT_BY_NUMBER
endif

#used to enable a work around for missing blit completion interrupt issue.
CFLAGS := $(CFLAGS) -DSTBLIT_ENABLE_MISSING_INTERRUPTIONS_WA

ifdef STBLIT_DUMP_NODES_LIST
CFLAGS := $(CFLAGS) -DSTBLIT_DUMP_NODES_LIST
endif

ifneq (,$(findstring $(DVD_BACKEND),5100 5105 5188 5107))
ifdef STBLIT_SINGLE_BLIT_DCACHE_NODES_ALLOCATION
CFLAGS := $(CFLAGS) -DSTBLIT_SINGLE_BLIT_DCACHE_NODES_ALLOCATION
endif

ifdef STBLIT_JOB_DCACHE_NODES_ALLOCATION
CFLAGS := $(CFLAGS) -DSTBLIT_JOB_DCACHE_NODES_ALLOCATION
endif
endif

# For 7109 (secured)
ifneq ($(DVD_SECURED_CHIP),)
CFLAGS := $(CFLAGS) -D DVD_SECURED_CHIP
endif


# Local objects which comprise this component
#---------------------------------------------

# ST20 chips (5100,5105)
ST20_OBJS := bdisp_init.tco bdisp_fe.tco bdisp_mem.tco \
             bdisp_be.tco bdisp_pool.tco bdisp_tab.tco bdisp_job.tco bdisp_slc.tco bdisp_flt.tco
ST20_TARGETS := blt_bdisp.lib

# ST40 chips (7109)
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))



ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))


LINUX_IMPORTS := stavmem stevt stblit stcommon stgxobj stos

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

# ST200 chips (5301)
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ifeq "$(BUILDING_FOR)" "ST200"
	ST200_IMPORTS := $(ST20_IMPORTS)
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
ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
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
	@$(ECHO) Cleaning BDisp
	-$(RM) $($(OBJECT_DIRECTORY)_OBJS)
	-$(RM) $($(OBJECT_DIRECTORY)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif


########################

