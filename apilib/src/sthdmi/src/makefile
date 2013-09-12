#####################################
# MODULE : sthdmi.lib / libsthdmi.a #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------

HEADER_IMPORTS := stsys sthdmi stos staudlx stclkrv $(DVD_TRANSPORT)
ST20_IMPORTS := stdenc stpio stuart sttbx stpwm
ST20_TARGETS := sthdmi.lib
ST20_OBJS := sthdmi.tco hdmi_drv.tco

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

LINUX_IMPORTS := stdenc stcommon staudlx
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

# In kernel mode, compile all files to create final .ko
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))


# Sub-libs for HDMI :
#--------------------

OMEGA_HDMI_LIKE = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA_HDMI_LIKE))" "$(DVD_BACKEND)"

ifeq "$(DVD_OS)" "LINUX"
ifdef USER_WRAPPER
SUBDIRS     := linux
else
SUBDIRS := $(SUBDIRS) source sink
endif
else
SUBDIRS := $(SUBDIRS) source sink
endif
endif

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(SUBDIRS))

ifeq "$(DVD_OS)" "LINUX"
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,linux)
endif


# Compile options
#----------------
CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT

# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif


# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
sthdmi.tco sthdmi.o: hdmi_drv.h
hdmi_drv.tco hdmi_drv.o: hdmi_drv.h

