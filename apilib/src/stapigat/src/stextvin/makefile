#########################
# MODULE : stextvin.lib #
#########################

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

# local objects which comprise this component
ST20_OBJS := evin.tco evin_mc.tco extvi_pr.tco
ifeq "$(DVD_OS)" "LINUX"
ST40_OBJS := $(call ST20OBJ_TO_LINUXOBJ, $(ST20_OBJS))
else
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ, $(ST20_OBJS))
endif
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ, $(ST20_OBJS))

ST20_TARGETS := stextvin.lib
ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
else
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
endif
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

# Pick up standard targets
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

clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) *$(OBJ_SUFFIX)

else

include $(DVD_MAKE)/builddir.mak

endif

# Local Dependancies
evin.tco evin.o: evin.c evin.h stextvin.h
evin_mc.tco evin_mc.o: evin_mc.c stextvin.h
extvi_pr.tco extvi_pr.o: extvi_pr.c extvi_reg.h extvi_pr.h
#end of file

