#########################################
# MODULE : hal.lib / libhal.a for stvtg #
#########################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Add include path
#---------------------
STVTG_HALSUBDIRS := .. denc
ifeq "$(findstring vos, $(VTG_OMEGA2_OPTIONAL_MODULES))" "vos"
STVTG_HALSUBDIRS += omega2 omega2/vos
endif
ifeq "$(findstring vfe, $(VTG_OMEGA2_OPTIONAL_MODULES))" "vfe"
STVTG_HALSUBDIRS += omega2 omega2/vfe
endif
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(STVTG_HALSUBDIRS))

# Define the targets
#---------------------------------------------
HEADER_IMPORTS := stos stsys stvtg

# Add dependence on correct stpti vob if STCLKRV_NO_PTI is not set
# stpti4 lite for 7710 and stpti4 for rest
USE_VOS_CLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND),$(USE_VOS_CLKRV))" "$(DVD_BACKEND)"
ifeq "$(STCLKRV_NO_PTI)" ""
ifneq (,$(findstring $(DVD_FRONTEND),7710))
    HEADER_IMPORTS += $(DVD_TRANSPORT)lt
else
    HEADER_IMPORTS += $(DVD_TRANSPORT)
endif
endif
endif

ST20_IMPORTS := stevt stgxobj stpio stuart sttbx
ST200_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS := hal.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_OBJS := vtg_ihal.tco
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

LINUX_IMPORTS := stevt stgxobj stpio stcommon sttbx
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


# Sub-libs :
#-------------------------------------------------
SUBDIRS     := denc $(VTG_OPTIONAL_MODULES)

# Compile options
#----------------
CFLAGS := $(VTG_DEFS) $(CFLAGS)

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
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
STVTG_DEPEND := vtg_drv.h vtg_ihal.h vtg_hal1.h
ifeq "$(findstring vos, $(VTG_OMEGA2_OPTIONAL_MODULES))" "vos"
STVTG_DEPEND += vtg_hal2.h hal_vos.h clk_gen.h
endif
ifeq "$(findstring vfe, $(VTG_OMEGA2_OPTIONAL_MODULES))" "vfe"
STVTG_DEPEND += vtg_hal2.h hal_vfe.h
endif
vtg_ihal.tco vtg_ihal.o: $(STVTG_DEPEND)
