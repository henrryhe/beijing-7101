#####################################
# MODULE : stvtg.lib / libstvtg.a   #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := stos stsys stvtg

# Add dependence on correct stpti vob if STCLKRV_NO_PTI is not set
# stpti4 lite for 7710 and stpti4 for rest
USE_VOS_CLKRV_NO_PTI  = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND),$(USE_VOS_CLKRV_NO_PTI))" "$(DVD_BACKEND)"
ifeq "$(STCLKRV_NO_PTI)" ""
ifneq (,$(findstring $(DVD_FRONTEND),7710))
    HEADER_IMPORTS += $(DVD_TRANSPORT)lt
else
    HEADER_IMPORTS += $(DVD_TRANSPORT)
endif
endif
endif

ST20_IMPORTS := stevt stgxobj stpio stuart sttbx
ST200_IMPORTS = $(ST20_IMPORTS)

ST20_TARGETS := stvtg.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))


LINUX_IMPORTS := stevt stgxobj stpio stcommon sttbx
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST40_IMPORTS = $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))


ST20_OBJS := stvtg.tco vtg_drv.tco



LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

# Define HW cells present
#--------------------------
VTG_SUBDIRS := hal hal/denc
OMEGA2_VFE2_LIKE = 5528
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA2_VFE2_LIKE))" "$(DVD_BACKEND)"
  VTG_OPTIONAL_MODULES := omega2
  VTG_OMEGA2_OPTIONAL_MODULES := vfe
  VTG_DEFS := -D STVTG_VFE2
  VTG_SUBDIRS := $(VTG_SUBDIRS)  hal/omega2
endif


OMEGA2_7015_LIKE = 7015 7020 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA2_7015_LIKE))" "$(DVD_BACKEND)"
  VTG_OPTIONAL_MODULES := omega2
  VTG_OMEGA2_OPTIONAL_MODULES := vos
  VTG_DEFS := -D STVTG_VOS
  VTG_SUBDIRS := $(VTG_SUBDIRS) hal/omega2
ifeq ($(DVD_FRONTEND),5528)
  VTG_OMEGA2_OPTIONAL_MODULES += vfe
  VTG_DEFS += -D STVTG_VFE2
endif
endif

OMEGA2_VFE_LIKE = GX1
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA2_VFE_LIKE))" "$(DVD_BACKEND)"
  VTG_OPTIONAL_MODULES := omega2
  VTG_OMEGA2_OPTIONAL_MODULES := vfe
  VTG_DEFS := -D STVTG_VFE
  VTG_SUBDIRS := $(VTG_SUBDIRS) hal/omega2
endif

OMEGA2_VFE2_LIKE = 5528 5100 5105 5301 5188 5525 5107 5162
ifeq "$(findstring $(DVD_BACKEND), $(OMEGA2_VFE2_LIKE))" "$(DVD_BACKEND)"
  VTG_OPTIONAL_MODULES := omega2
  VTG_OMEGA2_OPTIONAL_MODULES := vfe
  VTG_DEFS := -D STVTG_VFE2
  VTG_SUBDIRS := $(VTG_SUBDIRS) hal/omega2
  VTG_SUBDIRS := $(VTG_SUBDIRS) hal/omega2/vfe
endif

ifneq "$(STVTG_VSYNC_WITHOUT_VIDEO)" ""
  VTG_DEFS := -D STVTG_VSYNC_WITHOUT_VIDEO
endif

NEED_VTG_SHIFT = 5100 5105 5301 5188 5107 5162
ifeq "$(findstring $(DVD_BACKEND),$(NEED_VTG_SHIFT))" "$(DVD_BACKEND)"
        ifeq "$(WA_SHIFT_VTG)" "TRUE"
                DVD_CFLAGS += -DWA_SHIFT_VTG
        endif
endif

ifdef WA_GNBvd50834
DVD_CFLAGS+= -DWA_GNBvd50834
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    DVD_CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
endif

USE_VOS_CLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND),$(USE_VOS_CLKRV))" "$(DVD_BACKEND)"
VTG_DEFS += -D STVTG_USE_CLKRV
endif

ifneq "$(STCLKRV_NO_PTI)" ""
     VTG_DEFS+= -DSTCLKRV_NO_PTI
endif

ifeq "$(DVD_BACKEND)" "7710"
  ifeq "$(STI7710_CUT2x)" "TRUE"
     VTG_DEFS += -DSTI7710_CUT2x
  endif
endif

# Define private key depending on DVD_BACKEND
#--------------------------------------------
VTG_DEFS += -D BE_$(DVD_BACKEND)

# Pass definitions to sub make calls
export VTG_DEFS
export VTG_OPTIONAL_MODULES
export VTG_OMEGA2_OPTIONAL_MODULES

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(VTG_SUBDIRS))

# Sub-libs for HAL :
#--------------------------
SUBDIRS     := hal

# Compile options
#----------------
CFLAGS := $(VTG_DEFS) $(CFLAGS)

# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif


ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(SUBDIR_LIBS)
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
stvtg.tco stvtg.o: vtg_drv.h vtg_ihal.h
vtg_drv.tco vtg_drv.o: vtg_drv.h vtg_ihal.h

