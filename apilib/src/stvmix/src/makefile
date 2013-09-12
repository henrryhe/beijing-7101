########################
# MODULE : stvmix.lib  #
########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Header dependant components
#----------------------------
HEADER_IMPORTS := stsys stos stvmix

# Library direct dependant components
#------------------------------------
ST20_IMPORTS := stgxobj stlayer stvout stvtg sttbx stevt
# Indirect dependancies are stpio stuart stavmem


ST200_IMPORTS := $(ST20_IMPORTS) os20emu


ifneq (,$(findstring $(DVD_BACKEND), 5100))
    #Chroma Luma Delay WA is enabled only if WA_CLDELAY_USED is defined
    ifeq "$(WA_CLDELAY_USED)" "TRUE"
        DVD_CFLAGS += -DWA_GNBvd31390
    endif
endif

ifneq (,$(findstring $(DVD_BACKEND), 5105))
    ifdef SELECT_DEVICE_STB5118
        DVD_CFLAGS += -DSELECT_DEVICE_STB5118
    endif
endif


TARGET_WITH_STCLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND),$(TARGET_WITH_STCLKRV))" "$(DVD_BACKEND)"
ST20_IMPORTS += stclkrv
ST40_IMPORTS += stclkrv
LINUX_IMPORTS += stclkrv
endif


# Define the targets and exported files
#--------------------------------------
ST20_TARGETS := stvmix.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

LINUX_IMPORTS := stgxobj stvout stvtg stcommon stevt stlayer
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))


ifneq (,$(findstring $(DVD_BACKEND), 5100 5105 5301 5188 5525 5107 5162))
ST20_OBJS := stvmix_wrap_disp.tco
ST20_IMPORTS := stdisp
ST200_IMPORTS := stdisp

else
ST20_OBJS := stvmix.tco vmix_drv.tco
#endif

# Define include for subdirectories
#----------------------------------
#INCLUDE_PATH +=  $(DVD_BUILD_DIR)/hal

# Sub-libs for HAL :
#--------------------------
SUBDIRS := hal

# Compilation for 55XX omega1 type
ifneq (,$(findstring $(DVD_FRONTEND),5510 5512 5508 5518 5514 5516 5517))
VMIX_CFLAGS +=-DSTVMIX_OMEGA1
VMIX_HAL += HAL_OMEGA1
else
ifneq (,$(findstring $(DVD_BACKEND),5510 5512 5508 5518 5514 5516 5517))
VMIX_CFLAGS +=-DSTVMIX_OMEGA1
VMIX_HAL += HAL_OMEGA1
endif
endif

# Compilation for 7015 & ST40 GX1
# 7015 chip cannot be considered as a CPU
ifneq (,$(findstring $(DVD_FRONTEND),GX1 NGX1))
VMIX_CFLAGS +=-DSTVMIX_GAMMA
VMIX_HAL += HAL_GAMMA
else
ifneq (,$(findstring $(DVD_BACKEND),7015 GX1 NGX1))
VMIX_CFLAGS +=-DSTVMIX_GAMMA
VMIX_HAL += HAL_GAMMA
endif
endif

# Compilation for generic compositor 7020 & 5528 & 7710
# 7020 chip cannot be considered as a CPU

ifneq (,$(findstring $(DVD_BACKEND),5528 7710 7100 7109 7200))
VMIX_CFLAGS +=-DSTVMIX_GENGAMMA -DSTVMIX_GENGAMMA_$(DVD_BACKEND)
VMIX_HAL += HAL_GENGAMMA
endif






ifeq "$(DVD_BACKEND)" "7710"
  ifeq "$(STI7710_CUT2x)" "TRUE"
     VMIX_CFLAGS += -DSTI7710_CUT2x
  endif
endif

ifneq (,$(findstring $(DVD_BACKEND),7020))
ifneq (,$(findstring $(DVD_FRONTEND),5528))
VMIX_CFLAGS +=-DSTVMIX_GENGAMMA_$(DVD_FRONTEND)
endif
VMIX_CFLAGS +=-DSTVMIX_GENGAMMA -DSTVMIX_GENGAMMA_$(DVD_BACKEND)
VMIX_HAL += HAL_GENGAMMA
endif

ifdef SERVICE_DIRECTV
    VMIX_CFLAGS += -DSERVICE_DIRECTV
endif


# export for other sub makefiles
export VMIX_CFLAGS
export VMIX_HAL

CFLAGS := $(VMIX_CFLAGS) $(CFLAGS)
endif

ifdef MAXVIEWPORTS
  CFLAGS += -DMAXVIEWPORTS=$(MAXVIEWPORTS)
endif

ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


# Pick up standars targets
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

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning stvmix.lib
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else

include $(DVD_MAKE)/builddir.mak

endif

# Local dependancies
ifneq (,$(findstring $(DVD_BACKEND),5100 5105 5301 5188 5107 5162))
stvmix_wrap_disp.tco stvmix_wrap_disp.o : stvmix_wrap_disp.h
else
stvmix.tco stvmix.o vmix_drv.tco vmix_drv.o: stvmix.h vmix_drv.h
endif




#end of file





