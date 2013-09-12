########################
# MODULE : snake.lib #
########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Header dependant components
#----------------------------
# Following for testcfg.h
HEADER_IMPORTS := $(STAPIGAT_INCLUDE)
# Library direct dependant components
#------------------------------------
ST20_IMPORTS := sttbx stos
ST40_IMPORTS := sttbx os20emu stos
ST200_IMPORTS := sttbx stos

ST20_TARGETS := snake.lib
ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
else
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
endif
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_OBJS := blt_drawing.tco blt_engine.tco blt_wrapper.tco
ifeq "$(DVD_OS)" "LINUX"
ST40_OBJS := $(call ST20OBJ_TO_LINUXOBJ, $(ST20_OBJS))
else
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ, $(ST20_OBJS))
endif
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ, $(ST20_OBJS))

CFLAGS := $(CFLAGS) -D STTBX_PRINT

# Pick up standars targets
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
ifeq "$(ARCHITECTURE)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(ARCHITECTURE)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)
endif

#clean:
#        @$(ECHO) Cleaning snake
#        -$(RM) $($(ARCHITECTURE)_OBJS)
#        -$(RM) $($(ARCHITECTURE)_TARGETS)
#        -$(RM) *$(OBJ_SUFFIX)
else

include $(DVD_MAKE)/builddir.mak

endif

# Local Dependancies
#dumpreg.lib: dumpreg.tco
#dumpreg.tco: dumpreg.c dumpreg.h testcfg.h

#end of file
