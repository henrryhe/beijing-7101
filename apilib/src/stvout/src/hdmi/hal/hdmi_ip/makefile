##################################################
# MODULE : hdmi_ip.lib / libhdmi_ip.a for stvout #
##################################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Add include path
#---------------------
INCLUDE_PATH += $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/../../..

# Define the targets
#---------------------------------------------
HEADER_IMPORTS := stos stsys stvout

ST20_IMPORTS := stdenc stpio stuart sttbx
ST20_TARGETS := hdmi_ip.lib
ST20_OBJS := hdmi_ip.tco

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

LINUX_IMPORTS := $(ST20_IMPORTS)
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))



# Compile options
#----------------
CFLAGS := $(VOUT_DEFS) $(CFLAGS)
ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
     CFLAGS := $(CFLAGS) -DSTVOUT_HDCP_PROTECTION
endif

ifeq "$(WA_GNBvd44290)" "TRUE"
     CFLAGS := $(CFLAGS) -DWA_GNBvd44290
endif

ifdef WA_GNBvd54182
    CFLAGS +=-DWA_GNBvd54182
endif

# Pick up standards rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
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
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependencies
hdmi_ip.tco hdmi_ip.o : hdmi_ip.h hdmi_reg.h
