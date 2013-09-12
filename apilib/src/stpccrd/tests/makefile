DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
ifneq "$(DVD_OS)" "LINUX"
ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),7710 5100 5301 5105 5107)"
ST20_IMPORTS := stboot stos stevt stpio sttbx stcommon stpccrd
else
ST20_IMPORTS := stboot stos stevt stpio sttbx stcommon sti2c stpccrd
endif

else
ST20_IMPORTS := stos stcommon stpccrd
endif


ST20_TARGETS := pccrdtst.lku

ifneq "$(DVD_OS)" "LINUX"
ifndef STTBX_NO_UART
ST20_IMPORTS += stuart
endif
endif

ifndef STPCCRD_DISABLE_TESTTOOL
ST20_IMPORTS += testtool
endif


ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))

# cache control
ifdef DISABLE_ICACHE
CFLAGS += -DDISABLE_ICACHE
endif
ifdef DISABLE_DCACHE
CFLAGS += -DDISABLE_DCACHE
endif

ifdef STPCCRD_VCC_5V
CFLAGS += -DSTPCCRD_VCC_5V
endif

ifdef STPCCRD_USE_ONE_MODULE
CFLAGS += -DSTPCCRD_USE_ONE_MODULE
endif

ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),5514 5516 5517 7100 7109)"
 ifdef STPCCRD_HOT_PLUGIN_ENABLED
   CFLAGS += -DSTPCCRD_HOT_PLUGIN_ENABLED
 endif
endif

ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),5518 )"
  ifdef STPCCRD_HOT_PLUGIN_ENABLED
     $(error  : STPCCRD_HOT_PLUGIN not supported for 5518 )
  endif
endif

ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),5514 5516 5517 7100 7109)"
 ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
	CFLAGS += -DSTPCCRD_HOT_PLUGIN_WITH_STEVT
 endif
endif

ifdef STPCCRD_DISABLE_TESTTOOL
	CFLAGS += -DSTPCCRD_DISABLE_TESTTOOL
endif

ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),5514 5518 5528 5100)"
 ifeq "$(DVD_FRONTEND)" "5514"
  ifdef STPCCRD_BOARD
   ifneq ("$(STPCCRD_BOARD)","MOCHA")
     $(error  :invalid reference platform $(STPCCRD_BOARD))
   endif
  else
     $(error  :Need to specify reference platform )
  endif
 else
  ifeq "$(DVD_FRONTEND)" "5518"
   ifdef STPCCRD_BOARD
    ifneq ("$(STPCCRD_BOARD)","TYLKO")
     $(error  :invalid reference platform $(STPCCRD_BOARD))
    endif
   else
     $(error  :Need to specify reference platform )
   endif
  endif
 endif

 ifeq "$(DVD_FRONTEND)" "5528"
  ifdef STPCCRD_BOARD
   ifneq ("$(STPCCRD_BOARD)","ESPRESSO")
    $(error  :invalid reference platform $(STPCCRD_BOARD))
   endif
  else
    $(error  :Need to specify reference platform )
  endif
 endif

 ifeq "$(DVD_FRONTEND)" "5100"
  ifdef STPCCRD_BOARD
   ifneq ("$(STPCCRD_BOARD)","VESTEL")
    $(error  :invalid reference platform $(STPCCRD_BOARD))
   endif
  else
    $(error  :Need to specify reference platform )
  endif
 endif

CFLAGS += -D$(STPCCRD_BOARD)

 ifndef STPCCRD_DVBCI_CONTROLLER
   STPCCRD_DVBCI_CONTROLLER := STV0700
 else
  ifneq "$(STPCCRD_DVBCI_CONTROLLER)" "$(findstring $(STPCCRD_DVBCI_CONTROLLER),STV0700 STV0701)"
   $(error  :Unable to identify $(STPCCRD_DVBCI_CONTROLLER))
  endif
 endif

 CFLAGS += -D$(STPCCRD_DVBCI_CONTROLLER)

endif #ifeq (5514 5518)

# local objects which comprise this component
ST20_OBJS := pccrdtst.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

CFLAGS += -DSTTBX_PRINT -DSTTBX_REPORT -DSTTBX_INPUT

include $(DVD_MAKE)/defrules.mak

$(ST20_TARGETS): $(ST20_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) *.dbg

pccrdtst.tco: pccrdtst.c pccrdtst.h

else
include $(DVD_MAKE)/builddir.mak
endif
