########################
# MODULE : stlayer.lib #
########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
COMPONENT_NAME := stlayer
ST20_TARGETS := $(COMPONENT_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))



# Sub-libs used in stlayer library:
#   - old archi : the graphic part is not linked.
#   - new archi : the graphic part is linked.
#   - GX1       : the video part is not linked.
#-----------------------------------------------
ifeq "$(DVD_OS)" "LINUX"
# In kernel mode, compile all files to create final .ko
ifeq  "$(DVD_OLD_GFX_API)" "TRUE"
    SUBDIRS := api video # no graphics ! #
else
        ifeq "$(DVD_BACKEND)" "GX1"
                SUBDIRS := api graphics # no video ! #
         else
                ifneq (,$(findstring $(DVD_BACKEND),5100 5105 5525 5188 5301 5107 5162))
                        SUBDIRS := api laycompo
                else
                        SUBDIRS := api video graphics # general case (all other cases)
                endif
        endif
endif
else
ifeq  "$(DVD_OLD_GFX_API)" "TRUE"
    SUBDIRS := api video # no graphics ! #
else
ifeq "$(DVD_BACKEND)" "GX1"
    SUBDIRS := api graphics # no video ! #
else
ifneq (,$(findstring $(DVD_BACKEND),5100 5105 5525 5188 5301 5107 5162))
    SUBDIRS := api laycompo
else
    SUBDIRS := api video graphics # general case (all other cases)
endif
endif
endif
endif

ifeq "$(DVD_OS)" "LINUX"
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,linux)
endif

# Only enable assert() calls if DEBUG=1
ifeq ($(DEBUG),)
    DVD_CFLAGS += -DNDEBUG
endif

ifneq ($(STVID_USE_CRC),)
    DVD_CFLAGS += -DSTLAYER_USE_CRC
endif

ifneq ($(VIDEO_DEBUG_DEINTERLACING_MODE),)
    DVD_CFLAGS += -DVIDEO_DEBUG_DEINTERLACING_MODE
endif

ifneq ($(STVID_DEBUG_GET_DISPLAY_PARAMS),)
    DVD_CFLAGS += -DSTVID_DEBUG_GET_DISPLAY_PARAMS
endif

ifneq ($(STVID_USE_FMD),)
    DVD_CFLAGS += -DSTLAYER_USE_FMD
endif

ifneq ($(STVID_USE_FGT),)
    DVD_CFLAGS += -DST_XVP_ENABLE_FLEXVP -DST_XVP_ENABLE_FGT
endif

# Set task priorities according to environment variables, but don't overwrite if they are already set in DVD_CFLAGS
ifneq ($(STLAYER_GAMMAGFX_TASK_PRIORITY),)
  ifeq ($(findstring STLAYER_GAMMAGFX_TASK_PRIORITY,$(DVD_CFLAGS)),)
    DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_GAMMAGFX_TASK_PRIORITY=$(STLAYER_GAMMAGFX_TASK_PRIORITY)
  endif
endif
ifneq ($(STLAYER_OSDFRAME_TASK_PRIORITY),)
  ifeq ($(findstring STLAYER_OSDFRAME_TASK_PRIORITY,$(DVD_CFLAGS)),)
    DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_OSDFRAME_TASK_PRIORITY=$(STLAYER_OSDFRAME_TASK_PRIORITY)
  endif
endif
ifneq ($(STLAYER_OSDTOPBOT_TASK_PRIORITY),)
  ifeq ($(findstring STLAYER_OSDTOPBOT_TASK_PRIORITY,$(DVD_CFLAGS)),)
    DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_OSDTOPBOT_TASK_PRIORITY=$(STLAYER_OSDTOPBOT_TASK_PRIORITY)
  endif
endif
ifneq ($(STLAYER_SUBPICTURE_TASK_PRIORITY),)
  ifeq ($(findstring STLAYER_SUBPICTURE_TASK_PRIORITY,$(DVD_CFLAGS)),)
    DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_SUBPICTURE_TASK_PRIORITY=$(STLAYER_SUBPICTURE_TASK_PRIORITY)
  endif
endif
ifneq ($(STLAYER_VIDEO_HAL_TASK_PRIORITY),)
  ifeq ($(findstring STLAYER_VIDEO_HAL_TASK_PRIORITY,$(DVD_CFLAGS)),)
    DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_VIDEO_HAL_TASK_PRIORITY=$(STLAYER_VIDEO_HAL_TASK_PRIORITY)
  endif
endif

ifeq "$(DVD_BACKEND)" "7109"
    ifneq ($(DVD_SECURED_CHIP),)
        DVD_CFLAGS := $(DVD_CFLAGS) -DDVD_SECURED_CHIP
    endif
endif

# Pick up standard rules
#------------------------
include $(DVD_MAKE)/defrules.mak

# Build the sub-libs api + video (+ graphics)
#--------------------------------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif


# Delete all objects and libs
#-----------------------------
clean: $(SUBDIR_CLEAN)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

