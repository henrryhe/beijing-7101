########################
# MODULE : stvmix.lib  #
########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Header dependant components
#----------------------------
HEADER_IMPORTS := stsys stos stvmix
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

# Library direct dependant components
#------------------------------------
ST20_IMPORTS := stgxobj stlayer stvout stevt stvtg sttbx
# Indirect dependancies are stpio stuart stavmem
ST200_IMPORTS := $(ST20_IMPORTS) os20emu


# Concat include path
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/..

ST20_TARGETS := hal.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

LINUX_IMPORTS := stgxobj stlayer stvout stevt stvtg stcommon
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST40_IMPORTS := $(ST20_IMPORTS) os20emu
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

# Objects in function of target
#------------------------------
ifneq (,$(findstring HAL_OMEGA1,$(VMIX_HAL)))
ST20_OBJS += mix55xx.tco
endif

ifneq (,$(findstring HAL_GAMMA,$(VMIX_HAL)))
ST20_OBJS += mixgamma.tco
endif

ifneq (,$(findstring HAL_GENGAMMA,$(VMIX_HAL)))
ST20_OBJS += gengamma.tco
endif

LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

CFLAGS := $(VMIX_CFLAGS) $(CFLAGS)

# Pick up standars targets
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)
endif


clean:
	@$(ECHO) Cleaning hal
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)
	-$(RM) *$(OBJ_SUFFIX)
else

include $(DVD_MAKE)/builddir.mak

endif

# Local dependancies
gengamma.tco gengamma.o: stvmix.h vmix_drv.h gengamma.h
mixgamma.tco mixgamma.o: stvmix.h vmix_drv.h mixgamma.h
mix55xx.tco mix55xx.o: stvmix.h vmix_drv.h mix55xx.h

#end of file
