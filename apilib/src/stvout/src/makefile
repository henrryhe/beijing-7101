#####################################
# MODULE : stvout.lib / libstvout.a #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := stos stsys stvout
ST20_IMPORTS := stdenc stpio stuart sttbx stpwm
ST200_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS := stvout.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))


ST20_OBJS := stvout.tco vout_drv.tco


ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

LINUX_IMPORTS := stdenc stcommon stpio sttbx
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))


# In kernel mode, compile all files to create final .ko
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))


# Define HW cells present
#--------------------------
VOUT_SUBDIRS := hal/denc
VOUT_OPTIONAL_MODULES := denc
DIGITAL_OUT_ON = 5514 5516 5517 5528 5100 5107 5162
ifeq "$(findstring $(DVD_BACKEND), $(DIGITAL_OUT_ON))" "$(DVD_BACKEND)"
  VOUT_OPTIONAL_MODULES := $(VOUT_OPTIONAL_MODULES) sddig
  VOUT_DEFS := -D STVOUT_SDDIG
  VOUT_SUBDIRS := $(VOUT_SUBDIRS) hal/sddig
  DVO_ON = 5528 5100 5107 5162
  ifeq "$(findstring $(DVD_BACKEND), $(DVO_ON))" "$(DVD_BACKEND)"
    VOUT_DEFS := $(VOUT_DEFS) -D STVOUT_DVO
  endif
endif

OMEGA2_7020_LIKE = 7015 7020
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA2_7020_LIKE))" "$(DVD_BACKEND)"
  VOUT_OPTIONAL_MODULES := $(VOUT_OPTIONAL_MODULES) hdout svm
  VOUT_DEFS := -D STVOUT_HDOUT -D STVOUT_SVM
  VOUT_SUBDIRS := $(VOUT_SUBDIRS) hal/hdout hal/svm
ifeq ($(DVD_FRONTEND), 5528)
  VOUT_OPTIONAL_MODULES := $(VOUT_OPTIONAL_MODULES) sddig
  VOUT_SUBDIRS := $(VOUT_SUBDIRS) hal/sddig
  VOUT_DEFS := $(VOUT_DEFS) -D STVOUT_DVO
endif
endif

OMEGA2_HDDAC_LIKE = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA2_HDDAC_LIKE))" "$(DVD_BACKEND)"
  VOUT_OPTIONAL_MODULES := $(VOUT_OPTIONAL_MODULES) hdout
  VOUT_DEFS := $(VOUT_DEFS) -D STVOUT_HDOUT -D STVOUT_HDDACS
  VOUT_SUBDIRS := $(VOUT_SUBDIRS) hal/hdout
endif

OMEGA2_HDMI_LIKE = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA2_HDMI_LIKE))" "$(DVD_BACKEND)"
  VOUT_OMEGA2_OPTIONAL_MODULES := hdmi_ip
  VOUT_DEFS := $(VOUT_DEFS) -D STVOUT_HDMI
  VOUT_SUBDIRS := $(VOUT_SUBDIRS) hdmi/hal
  ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
    VOUT_OMEGA3_OPTIONAL_MODULES := hdcp_ip
  endif
endif

ifeq ($(DVD_BACKEND),7710)
  ifeq "$(STI7710_CUT2x)" "TRUE"
     VOUT_DEFS := $(VOUT_DEFS) -D STI7710_CUT2x
  endif
endif

ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
     CFLAGS := $(CFLAGS) -DSTVOUT_HDCP_PROTECTION
endif

ifdef WA_GNBvd54182
    CFLAGS +=-DWA_GNBvd54182
endif

ifdef WA_GNBvd56512
    CFLAGS +=-DWA_GNBvd56512
endif


# Pass definitions to sub make calls
export VOUT_DEFS
export VOUT_OPTIONAL_MODULES

# export Sub-libs for HDMI :
#--------------------------
export VOUT_OMEGA2_OPTIONAL_MODULES
export VOUT_OMEGA3_OPTIONAL_MODULES

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(VOUT_SUBDIRS))


ifeq "$(DVD_OS)" "LINUX"
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,linux)
endif

# Sub-libs for HAL :
#--------------------
################################"
SUBDIRS     := hal

##############################
# Sub-libs for HDMI :
#--------------------
OMEGA2_HDMI_LIKE = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA2_HDMI_LIKE))" "$(DVD_BACKEND)"
 ifneq "$(DVD_OS)" "LINUX"
  SUBDIRS := $(SUBDIRS) hdmi
 endif
endif

# Compile options
#----------------
CFLAGS := $(VOUT_DEFS) $(CFLAGS)

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
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

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(SUBDIR_LIBS)
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
stvout.tco stvout.o: vout_drv.h denc_out.h dac_out.h
vout_drv.tco vout_drv.o: vout_drv.h denc_out.h

