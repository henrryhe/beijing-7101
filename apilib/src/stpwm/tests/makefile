#TEST makefile.
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

ifneq "$(DVD_OS)" "LINUX"

ifndef DISABLE_TOOLBOX
IMPORTS = sttbx
endif

ifndef STTBX_NO_UART
IMPORTS := $(IMPORTS) stuart
endif

# Other components this component is dependant upon
IMPORTS := stpwm $(IMPORTS) stboot stpio  stcommon  stos

else
IMPORTS := stos stcommon stpio stpwm

endif

TARGETS := stpwmtst$(EXE_SUFFIX)


# ST40
ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

# ST200
ifndef ST200_TARGETS
ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
ST200_IMPORTS = $(IMPORTS)
endif

# local objects which comprise this component
OBJS := stpwmtst$(OBJ_SUFFIX)

# If the environment variable STPWM_USES_PIO has been set
# then it will determine whether or not the PWM test harnesses
# initialize PIO or not. Otherwise STPWM_USES_PIO will
# be set according to DVD_FRONTEND, defaulting to STi5510
ifeq ($(strip $(STPWM_USES_PIO)),)
    ifeq ($(DVD_FRONTEND),5508)
        STPWM_USES_PIO = 0
    else 
      ifeq ($(DVD_FRONTEND),5518)
         STPWM_USES_PIO = 0
      else
         STPWM_USES_PIO = 1  #Default for 5510/12 and TP3
      endif
    endif  
endif


CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STPWM_USES_PIO=$(STPWM_USES_PIO)

ifdef DISABLE_TOOLBOX
CFLAGS := $(CFLAGS) -DDISABLE_TOOLBOX
endif

ifdef DISABLE_ICACHE
      CFLAGS += -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
      CFLAGS += -DDISABLE_DCACHE
endif

include $(DVD_MAKE)/defrules.mak

# This builds this component
stpwmtst$(EXE_SUFFIX): $(OBJS) $(IMPORT_LIBS) $(SUBDIR_LIBS)
	      @$(ECHO) Linking $@
	      $(LINK_EXECUTABLE)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) *.dbg

else

include $(DVD_MAKE)/builddir.mak

endif
