#####################################
# MODULE : hal.lib / libhal.a       #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := stos stsys stvout

ST20_IMPORTS := stdenc stpio stuart sttbx
ST20_TARGETS := hal.lib
ST20_OBJS := hal_hdmi.tco

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

LINUX_IMPORTS := $(ST20_IMPORTS)
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))


# Define HW cells present
#--------------------------

# Pass definitions to sub make calls
#export VOUT_DEFS
#export VOUT_OPTIONAL_MODULES

# Add include path
#----------------------
STVOUT_HALSUBDIRS := .. ../..
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(VOUT_SUBDIRS))

ifeq "$(VOUT_OMEGA2_OPTIONAL_MODULES)" "hdmi_ip"
STVOUT_HALSUBDIRS += hdmi_ip
endif

ifeq "$(VOUT_OMEGA3_OPTIONAL_MODULES)" "hdcp_ip"
STVOUT_HALSUBDIRS += hdcp_ip
endif


INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(STVOUT_HALSUBDIRS))

# Sub-libs for HDMI IP :
#-----------------------
SUBDIRS := hdmi_ip

# Sub-libs for HDCP IP :
#-----------------------
ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
 SUBDIRS += hdcp_ip
endif
ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
     CFLAGS := $(CFLAGS) -DSTVOUT_HDCP_PROTECTION
endif

# Compile options
#----------------
CFLAGS := $(VOUT_DEFS) $(CFLAGS)

ifeq "$(WA_GNBvd44290)" "TRUE"
     CFLAGS := $(CFLAGS) -DWA_GNBvd44290
endif

ifdef WA_GNBvd54182
    CFLAGS +=-DWA_GNBvd54182
endif

ifdef WA_GNBvd56512
    CFLAGS +=-DWA_GNBvd56512
endif



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
hal_hdmi.tco hal_hdmi.o :hal_hdmi.h vout_drv.h hdmi_api.h
