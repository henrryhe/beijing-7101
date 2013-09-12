# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS += stapigat/src stapigat/src/filesystem stcfg stos

# Other components this component is dependant upon
IMPORTS = sttbx stcommon testtool $(DVD_TRANSPORT)
ST40_IMPORTS = $(IMPORTS)

EXPORTS = hdd_filesystem.lib
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(EXPORTS))

# Select pti/link/stpti according to DVD_TRANSPORT
CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)

CFLAGS := $(CFLAGS) -I./stfpga -I.. -I. 

# Uncomment line below to compile for debug (debug info + no optimisation)
ifneq ($(DEBUG),)
    CFLAGS := -O0 -g $(CFLAGS)
endif

# Uncomment line below to compile for debug (stopping error)
#CFLAGS := $(CFLAGS) -D DEBUG
CFLAGS := $(CFLAGS) -D STTBX_REPORT -D STTBX_PRINT -D STTBX_INPUT

ifneq ($(STVID_HDD_TRACE_UART),)
# Define HDD_TRACE_UART to compile hdd.c with UART traces
	CFLAGS := $(CFLAGS) -D HDD_TRACE_UART
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

# Enable DEFINED_51xx flag for 51xx devices family.
 ifneq (,$(findstring "$(DVD_BACKEND)","5100" "5105" "5301" "5107" "5188" "5525" "5162"))
	CFLAGS := $(CFLAGS) -D ST_51xx_Device
 endif


# local objects which comprise this component
ST20_OBJS := hdd.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Target
# ------

ST20_TARGETS := hdd_filesystem.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak


$(ST20_TARGETS): $(ST20_OBJS)
	@$(ECHO) compil HDD
  ifeq ($(strip $(SILENT)),)
	@$(ECHO) Linking $@
  endif
	$(AR) $^ -o $@

$(ST40_TARGETS): $(ST40_OBJS)
  ifeq ($(strip $(SILENT)),)
	@$(ECHO) Linking $@
  endif
	$(BUILD_LIBRARY)

$(CLEAN_CELLS):
	$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(addprefix ./,$(addsuffix /*.tco,$(subst _clean,,$@))))
	$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(addprefix ./,$(addsuffix /*.lib,$(subst _clean,,$@))))

clean_message:
	@$(ECHO) Cleaning HDD

clean: clean_message $(CLEAN_CELLS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

# Local dependancies
hdd.tco: hdd.h

#end of file
