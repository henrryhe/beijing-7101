DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
include $(DVD_MAKE)/generic.mak


HEADER_IMPORTS := stos stcommon

ifneq "$(DVD_OS)" "LINUX"

ifdef STTBX_NO_UART
IMPORTS := stos stboot stmerge sttbx stcommon testtool stpio stevt
else
IMPORTS := stos stboot stmerge sttbx stcommon testtool stuart stpio stevt
endif

ifndef DVD_TRANSPORT
  DVD_TRANSPORT = stpti4
endif

else
IMPORTS := stos stcommon stevt testtool
### stpti4
endif

ifeq "$(DVD_PLATFORM)" "mb391"
IMPORTS := $(IMPORTS) stpti4lt
else
IMPORTS := $(IMPORTS) $(DVD_TRANSPORT)
endif

ifeq "$(DVD_FRONTEND)" "5528"
    IMPORTS := $(IMPORTS) stgpdma
endif

ifneq (,$(findstring $(DVD_FRONTEND),5100 7710 7100 5301 7109))
#    IMPORTS := $(IMPORTS) stfdma
endif

ifdef STMERGE_USE_TUNER
    IMPORTS := $(IMPORTS) sti2c sttuner
endif

IMPORTS := $(IMPORTS) stmerge

ifdef STMERGE_USE_TUNER
TSMERGETEST_CFLAGS += -DSTMERGE_USE_TUNER
TSIN_PID := 0x280
SWTS_PID := 0x280
endif

TARGETS = stmergetst$(EXE_SUFFIX)

ifdef STMERGE_1394_PRODUCER_CONSUMER
TARGETS += stmerge1394$(EXE_SUFFIX)
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

ifdef STMERGE_1394_PRODUCER_CONSUMER
TSMERGETEST_CFLAGS += -DSTMERGE_1394_PRODUCER_CONSUMER
endif

ifdef STMERGE_DEFAULT_PARAMETERS
TSMERGETEST_CFLAGS += -DSTMERGE_DEFAULT_PARAMETERS
endif

ifdef STMERGE_DTV_PACKET
TSMERGETEST_CFLAGS += -DSTMERGE_DTV_PACKET
endif

ifdef USE_DMA_FOR_SWTS
TSMERGETEST_CFLAGS += -DUSE_DMA_FOR_SWTS
endif

ifdef STMERGE_NO_TAG
TSMERGETEST_CFLAGS += -DSTMERGE_NO_TAG
endif

ifdef POLL_PTI_ERROR_EVENTS
TSMERGETEST_CFLAGS += -DPOLL_PTI_ERROR_EVENTS
endif

ifdef RUNNING_FROM_STEM
TSMERGETEST_CFLAGS += -DRUNNING_FROM_STEM
endif

ifdef STMERGE_USE_TUNER
ifdef C_BAND_TESTING
TSMERGETEST_CFLAGS += -DC_BAND_TESTING
endif
ifdef LNB_21
TSMERGETEST_CFLAGS += -DLNB_21
endif
endif

ifdef STMERGE_CHANNEL_CHANGE
TSMERGETEST_CFLAGS += -DSTMERGE_CHANNEL_CHANGE -DSTMERGE_USE_TUNER
endif

ifdef MAX_INJECTION_TIME
TSMERGETEST_CFLAGS += -DMAX_INJECTION_TIME=$(MAX_INJECTION_TIME)
endif

ifdef STMERGE_PARALLEL_INPUT
TSMERGETEST_CFLAGS += -DSTMERGE_PARALLEL_INPUT
endif

ifdef SWTS_MANUAL_PACE
TSMERGETEST_CFLAGS += -DSWTS_MANUAL_PACE
endif

ifdef STMERGE_INTERRUPT_SUPPORT
TSMERGETEST_CFLAGS += -DSTMERGE_INTERRUPT_SUPPORT
endif

# for cache enable/disable
ifdef DISABLE_ICACHE
      CFLAGS +=-DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
      CFLAGS +=-DDISABLE_DCACHE
endif

CFLAGS := $(CFLAGS) $(TSMERGETEST_CFLAGS) $(TUNER_CFLAGS) -DSTTBX_PRINT -DSTTBX_REPORT

ifdef STMERGE_USE_TUNER
ifdef STMERGE_DTV_PACKET
stmergetst_OBJS += $(DVD_BUILD_DIR)/services/DTV/lists$(OBJ_SUFFIX)
else
stmergetst_OBJS += $(DVD_BUILD_DIR)/services/DVB/lists$(OBJ_SUFFIX)
endif
endif

ifdef STMERGE_USE_TUNER
stmergetst_OBJS += stmergetst$(OBJ_SUFFIX) tuner$(OBJ_SUFFIX) stmergeerr$(OBJ_SUFFIX) \
                   inject_swts$(OBJ_SUFFIX) stmergefn$(OBJ_SUFFIX)
else
ifeq "$(DVD_OS)" "LINUX"
stmergetst_OBJS += stmergetst$(OBJ_SUFFIX) stmergeerr$(OBJ_SUFFIX) stmergefn$(OBJ_SUFFIX)
else
stmergetst_OBJS += stmergetst$(OBJ_SUFFIX) stmergeerr$(OBJ_SUFFIX) inject_swts$(OBJ_SUFFIX) stmergefn$(OBJ_SUFFIX)
endif
endif

ifdef STMERGE_1394_PRODUCER_CONSUMER
ifdef STMERGE_USE_TUNER
stmerge1394_OBJS = stmergetst$(OBJ_SUFFIX) tuner$(OBJ_SUFFIX) stmergeerr$(OBJ_SUFFIX) \
                   inject_swts$(OBJ_SUFFIX) stmergefn$(OBJ_SUFFIX)
else

stmerge1394_OBJS = stmergetst$(OBJ_SUFFIX) stmergeerr$(OBJ_SUFFIX) \
                  inject_swts$(OBJ_SUFFIX) stmergefn$(OBJ_SUFFIX)
endif
endif

include $(DVD_MAKE)/defrules.mak

stmergetst$(EXE_SUFFIX): $(stmergetst_OBJS)  $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

stmerge1394$(EXE_SUFFIX): $(stmerge1394_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(TARGETS)
	-$(RM) *$(OBJ_SUFFIX)
	-$(RM) *$(LIB_SUFFIX)
	-$(RM) *$(EXE_SUFFIX)
	-$(RM) $(subst .lku,.dbg,$(TARGETS))
	-$(RM) $(subst .lku,.map,$(TARGETS))

#local dependancies
ifdef STMERGE_USE_TUNER
stmergetst$(OBJ_SUFFIX): stmergetst.h tuner.h stmergeerr.h  inject_swts.h stmergefn.h
else
stmergetst$(OBJ_SUFFIX): stmergetst.h stmergeerr.h  inject_swts.h stmergefn.h
endif

stmergeerr$(OBJ_SUFFIX): stmergetst.h stmergeerr.h  inject_swts.h stmergefn.h

else

include $(DVD_MAKE)/builddir.mak

endif

