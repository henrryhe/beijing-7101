#####################################
# MODULE : hdmi.lib / libhdmi.a     #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := stos stsys stvout

ST20_IMPORTS := stdenc stpio stuart sttbx sti2c
ST20_TARGETS := hdmi.lib
ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
ST20_OBJS := hdmi_pro.tco hdcp.tco hdmi_api.tco hdmi_int.tco
else
ST20_OBJS := hdmi.tco hdmi_api.tco hdmi_int.tco
endif

ifeq "$(STVOUT_CEC_CAPTURE)" "TRUE"
        ST20_OBJS :=$(ST20_OBJS) cec.tco
else
ifeq "$(STVOUT_CEC_PIO_COMPARE)" "TRUE"
        ST20_OBJS :=$(ST20_OBJS) cec.tco
endif
endif

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

LINUX_IMPORTS := $(ST20_IMPORTS)
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))


# Add include path
#----------------------
INCLUDE_PATH += $(DVD_BUILD_DIR)/..

ifeq "$(VOUT_OMEGA2_OPTIONAL_MODULES)" "hdmi_ip"
STVOUT_HALSUBDIRS += hal hal/hdmi_ip
endif

ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
ifeq "$(VOUT_OMEGA3_OPTIONAL_MODULES)" "hdcp_ip"
STVOUT_HALSUBDIRS += hal/hdcp_ip
endif
endif

INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(STVOUT_HALSUBDIRS))

# Sub-libs for HAL :
#-------------------
SUBDIRS := hal

# Compile options
#----------------
CFLAGS := $(VOUT_DEFS) $(CFLAGS)

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
endif

ifeq "$(WA_GNBvd44290)" "TRUE"
     CFLAGS := $(CFLAGS) -DWA_GNBvd44290
   ifdef TRACE_PIO
     CFLAGS +=-DTRACE_PIO
   endif
endif

ifeq "$(WA_WITH_PREIT)" "TRUE"
     CFLAGS := $(CFLAGS) -DWA_WITH_PREIT
endif

ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
     CFLAGS := $(CFLAGS) -DSTVOUT_HDCP_PROTECTION
endif

ifeq "$(HDMI_AUDIO_256FS)" "TRUE"
     CFLAGS := $(CFLAGS) -DHDMI_AUDIO_256FS
endif

ifdef WA_GNBvd54182
    CFLAGS +=-DWA_GNBvd54182
    ifdef TRACE_PIO
    CFLAGS +=-DTRACE_PIO
    endif
endif

ifdef WA_GNBvd56512
   CFLAGS +=-DWA_GNBvd56512
    ifdef TRACE_PIO
    CFLAGS +=-DTRACE_PIO
    endif
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

ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
hdmi_api.tco hdmi_api.o : hdmi.h hdcp.h hdmi_api.h vout_drv.h  hal_hdmi.h hdmi_ip.h
hdmi_pro.tco hdmi.o :hdmi.h vout_drv.h
hdcp.tco hdcp.o :hdcp.h vout_drv.h
else
hdmi_api.tco hdmi_api.o : hdmi.h hdmi_api.h vout_drv.h  hal_hdmi.h hdmi_ip.h
hdmi.tco hdmi.o :hdmi.h vout_drv.h
endif

ifeq "$(STVOUT_CEC_CAPTURE)" "TRUE"
cec.tco cec.o : cec.h
else
ifeq "$(STVOUT_CEC_PIO_COMPARE)" "TRUE"
cec.tco cec.o : cec.h
endif
endif
