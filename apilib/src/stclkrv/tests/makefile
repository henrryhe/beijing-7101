DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

# Pick up default rules and variables

include $(DVD_MAKE)/generic.mak

# Other components this component is dependent upon
ifdef DISABLE_TOOLBOX 
IMPORTS := stos stboot stcommon stevt stclkrv
else
ifeq "$(DVD_OS)" "LINUX"
IMPORTS := stos stcommon stevt stclkrv
else
IMPORTS := stos stboot stcommon stevt sttbx stclkrv 
endif
endif

ifeq "$(findstring $(DVD_FRONTEND),7100 7109 5188 7200)" "$(DVD_FRONTEND)"
IMPORTS := $(IMPORTS) stpio
endif

ifndef STCLKRV_NO_PTI

ifneq "$(DVD_OS)" "LINUX"
IMPORTS := $(IMPORTS) testtool
endif

ifeq "$(findstring $(DVD_FRONTEND),5105 5107)" "$(DVD_FRONTEND)"
    IMPORTS := $(IMPORTS) stpti4lt
else
ifeq "$(DVD_FRONTEND)" "7710"
    IMPORTS := $(IMPORTS) stpti4lt stmerge
else
ifeq "$(DVD_FRONTEND)" "5188"
    IMPORTS := $(IMPORTS) stdemux stfdma
else
ifeq "$(DVD_FRONTEND)" "7200"
    IMPORTS := $(IMPORTS) $(DVD_TRANSPORT)
else
    IMPORTS := $(IMPORTS) $(DVD_TRANSPORT) stmerge
endif
endif
endif
endif

endif

EXPORTS = 

# Our code only uses STTBX_DEVICE_TYPE_DCU, so avoid stuart dependency by:
STTBX_NO_UART := 1
export STTBX_NO_UART

# But let's leave the decision to the user:
#ifeq "$(findstring -DSTTBX_NO_UART,$(DVD_CFLAGS))" ""
ifndef STTBX_NO_UART
    IMPORTS += stuart
endif

# Ensure hidden testing function is enabled in STCLKRV

ifdef CLKRV_TESTING
CFLAGS += -DCLKRV_TESTING
endif

# Parse PCR and VIDEO PID environment variables
ifneq ($(strip $(PCR_PID)),)
    TEST_FLAGS = -DPCR_PID=$(PCR_PID)
endif

# pcr pid 1 & 2 for dual decode test
ifeq "$(DVD_FRONTEND)" "7200"
ifeq "$(findstring stpti,$(DVD_TRANSPORT))" "stpti"
ifneq ($(strip $(PCR_PID2)),)
    TEST_FLAGS := $(TEST_FLAGS) -DPCR_PID2=$(PCR_PID2)
endif
ifneq ($(strip $(PCR_PID1)),)
    TEST_FLAGS := $(TEST_FLAGS) -DPCR_PID1=$(PCR_PID1)
endif
endif
endif

ifeq "$(DVD_SERVICE)" "DTV"
TEST_FLAGS := $(TEST_FLAGS) -DSTPTI_DTV_SUPPORT -DSTMERGE_DTV_PACKET
endif

# Set required testing C flags
ifdef DISABLE_TOOLBOX 
CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS) $(TEST_FLAGS)
else
CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS) -DSTTBX_REPORT -DSTTBX_PRINT -DSTTBX_INPUT $(TEST_FLAGS)
endif

ifdef STCLKRV_NO_PTI
CFLAGS += -DSTCLKRV_NO_PTI
endif

ifdef DISABLE_TOOLBOX 
CFLAGS += -DSTCLKRV_NO_TBX
endif

ifdef RESOURCE_USAGE_TESTS
    CFLAGS += -DRESOURCE_USAGE_TESTS
endif

ifdef CLKRV_FSCLKRV_TEST_SD_ONLY
    CFLAGS += -DCLKRV_FSCLKRV_TEST_SD_ONLY
endif    
ifdef SLAVE_FREQ_CHANGE_TEST
    CFLAGS += -DSLAVE_FREQ_CHANGE_TEST
endif

ifdef CLKRV_DETAIL_TEST
    CFLAGS += -DCLKRV_DETAIL_TEST
endif
   
ifdef DISABLE_ICACHE
    CFLAGS := $(CFLAGS) -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
    CFLAGS := $(CFLAGS) -DDISABLE_DCACHE
endif   
   

# Local objects which comprise this component
stclkrvt_OBJS := stclkrvt$(OBJ_SUFFIX)
stclkpti_OBJS := stclkpti$(OBJ_SUFFIX)
stclkrvmi_OBJS := stclkrvmi$(OBJ_SUFFIX)
ifeq "$(DVD_FRONTEND)" "7200"
clkrvdualpti_OBJS := clkrvdualpti$(OBJ_SUFFIX)
endif
# No multiple instance support for 5100\7710 chip.
# stclkrvmi_OBJS :=  stclkrvmi$(OBJ_SUFFIX)
clksimt_OBJS := clksimt$(OBJ_SUFFIX)

ifndef STCLKRV_NO_PTI
ifeq "$(DVD_OS)" "LINUX"
TARGETS =  stclkrvt$(EXE_SUFFIX) stclkpti$(EXE_SUFFIX)
ifeq "$(DVD_FRONTEND)" "7200"
TARGETS := $(TARGETS) clkrvdualpti$(EXE_SUFFIX)
endif
else
TARGETS =  stclkrvt$(EXE_SUFFIX) stclkpti$(EXE_SUFFIX) clksimt$(EXE_SUFFIX) stclkrvmi$(EXE_SUFFIX)
ifeq "$(DVD_FRONTEND)" "7200"
TARGETS := $(TARGETS) clkrvdualpti$(EXE_SUFFIX)
endif
endif
else
TARGETS =  clkrvnopti$(EXE_SUFFIX)
endif

ifndef ST40_TARGETS
  ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
  ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

ifndef ST200_TARGETS
  ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
  ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
  ST200_IMPORTS = $(IMPORTS)
endif

include $(DVD_MAKE)/defrules.mak

#ifeq ($(ARCHITECTURE),LINUX)

clkrvnopti$(EXE_SUFFIX): $(stclkrvt_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

stclkrvt$(EXE_SUFFIX): $(stclkrvt_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

stclkpti$(EXE_SUFFIX): $(stclkpti_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clkrvdualpti$(EXE_SUFFIX): $(clkrvdualpti_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)	

stclkrvmi$(EXE_SUFFIX): $(stclkrvmi_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clksimt$(EXE_SUFFIX): $(clksimt_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
   
clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(stclkrvt_OBJS)
	-$(RM) $(stclkpti_OBJS)
	-$(RM) $(stclkrvmi_OBJS)
	-$(RM) $(clksimt_OBJS)
	-$(RM) $(TARGETS)
	-$(RM) $(subst $(EXE_SUFFIX),.dbg,$(TARGETS))

else

include $(DVD_MAKE)/builddir.mak

endif
