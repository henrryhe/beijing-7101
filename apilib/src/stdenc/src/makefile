#####################################
# MODULE : stdenc.lib / libstdenc.a #
#####################################
# Add for make system version 2 support

DVD_MAKE_VERSION :=2

ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := stos stsys stdenc
ST20_IMPORTS := sttbx stuart stpio
ST40_IMPORTS := $(ST20_IMPORTS)
ST200_IMPORTS := $(ST20_IMPORTS)

LINUX_IMPORTS := stcommon

ST20_TARGETS := stdenc.lib

LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_OBJS := stdenc.tco denc_drv.tco
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


# Define HW cells present
#--------------------------

STDENC_SUBDIRS := hal hal/reg hal/reg/rbus

ifeq "$(DVD_PLATFORM)" "mb295"
ST20_IMPORTS += sti2c
STDENC_SUBDIRS += hal/reg/i2c
DENC_OPTIONAL_MODULES := i2c
DENC_DEFS := -D STDENC_I2C

endif

NEED_DENC_MAIN_COMPENSATION = 5100 5105 5188
ifeq "$(findstring $(DVD_BACKEND),$(NEED_DENC_MAIN_COMPENSATION))" "$(DVD_BACKEND)"
        DVD_CFLAGS += -DDENC_MAIN_COMPENSATION_ENABLED
        ifeq "$(findstring $(DENC_MAIN_COMPENSATION_VALUE),2)" "2"
               DVD_CFLAGS += -DDENC_MAIN_COMPONSATE_2PIXEL
        endif
        ifeq "$(findstring $(DENC_MAIN_COMPENSATION_VALUE),1.5)" "1.5"
               DVD_CFLAGS += -DDENC_MAIN_COMPONSATE_1_5PIXEL
        endif
        ifeq "$(findstring $(DENC_MAIN_COMPENSATION_VALUE),1)" "1"
               DVD_CFLAGS += -DDENC_MAIN_COMPONSATE_1PIXEL
        endif
        ifeq "$(findstring $(DENC_MAIN_COMPENSATION_VALUE),0.5)" "0.5"
               DVD_CFLAGS += -DDENC_MAIN_COMPONSATE_0_5PIXEL
        endif
endif
NEED_DENC_AUX_COMPENSATION = 5100
ifeq "$(findstring $(DVD_BACKEND),$(NEED_DENC_AUX_COMPENSATION))" "$(DVD_BACKEND)"
 #       DVD_CFLAGS += -DDENC_AUX_COMPENSATION_ENABLED
        ifeq "$(findstring $(DENC_AUX_COMPENSATION_VALUE),2)" "2"
               DVD_CFLAGS += -DDENC_AUX_COMPONSATE_2PIXEL
        endif
        ifeq "$(findstring $(DENC_AUX_COMPENSATION_VALUE),1.5)" "1.5"
               DVD_CFLAGS += -DDENC_AUX_COMPONSATE_1_5PIXEL
        endif
        ifeq "$(findstring $(DENC_AUX_COMPENSATION_VALUE),1)" "1"
               DVD_CFLAGS += -DDENC_AUX_COMPONSATE_1PIXEL
        endif
        ifeq "$(findstring $(DENC_AUX_COMPENSATION_VALUE),0.5)" "0.5"
               DVD_CFLAGS += -DDENC_AUX_COMPONSATE_0_5PIXEL
        endif
endif

# Pass definitions to sub make calls
export DENC_DEFS
export DENC_OPTIONAL_MODULES

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(STDENC_SUBDIRS))

# Sub-libs for HAL :
#--------------------------
SUBDIRS     := hal

# Compile options
#----------------
CFLAGS := $(DENC_DEFS) $(CFLAGS)

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

STDENC_DEPEND := denc_drv.h denc_hal.h reg_rbus.h

ifeq "$(DENC_OPTIONAL_MODULES)" "i2c"
  STDENC_DEPEND += reg_i2c.h
endif
stdenc.tco stdenc.o: $(STDENC_DEPEND)
denc_drv.tco denc_drv.o: $(STDENC_DEPEND)

