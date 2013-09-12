###################################
# MODULE : sttbx.lib / libsttbx.a #
###################################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Dependencies
ST20_IMPORTS :=
ifeq ($(STTBX_NO_UART),)
	ST20_IMPORTS := stpio stuart
endif
ST40_IMPORTS := $(ST20_IMPORTS)
ST200_IMPORTS := $(ST20_IMPORTS)
LINUX_IMPORTS := $(ST20_IMPORTS) stcommon

# Define the targets
#-------------------
ST20_TARGETS := sttbx.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST20_OBJS := tbx_init.tco tbx_out.tco tbx_rep.tco tbx_time.tco tbx_in.tco tbx_buff.tco tbx_filt.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

CFLAGS += -D STTBX_MODULE_ID=28
ifneq ($(STTBX_NO_UART),)
	CFLAGS += -D STTBX_NO_UART
endif
ifneq ($(DVD_POSIX),)
        CFLAGS += -D CONFIG_POSIX
endif

include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
$(ST20_TARGETS): $(ST20_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif
ifeq "$(ARCHITECTURE)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif
ifeq "$(ARCHITECTURE)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif

# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# Local dependancies
tbx_buff.tco tbx_buff.o:tbx_init.h
tbx_init.tco tbx_init.o:tbx_init.h
tbx_out.tco tbx_out.o:tbx_init.h tbx_out.h
tbx_rep.tco tbx_rep.o:tbx_init.h tbx_out.h
tbx_time.tco tbx_time.o:
tbx_in.tco tbx_in.o:tbx_init.h tbx_out.h

#end
