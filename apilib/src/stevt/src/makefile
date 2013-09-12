# vi:noexpandtab
###############################################################################
# File : makefile
#
# Description:
#     Makefile of the STEVT component.
#
# Copyright 2001 STMicroelectronics. All Rights Reserved.
################################################################################

DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ifdef STEVT_NO_TBX
CFLAGS :=  $(CFLAGS) -DSTEVT_NO_TBX
endif

ifdef STEVT_NO_TBX
IMPORTS = stos stcommon 
else
IMPORTS = stos stcommon sttbx
endif

ifdef REPORT
CFLAGS :=  $(CFLAGS) -DSTTBX_REPORT
endif

ifdef STEVT_NO_PARAMETER_CHECK
CFLAGS += -DSTEVT_NO_PARAMETER_CHECK
endif

# Other components this component is dependant upon
TARGETS = $(LIB_PREFIX)stevt$(LIB_SUFFIX)

ifeq "$(DVD_OS)" "OS40"
IMPORTS += os20emu
endif

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
OBJS := event$(OBJ_SUFFIX) stevt$(OBJ_SUFFIX) mem_handler$(OBJ_SUFFIX)

include $(DVD_MAKE)/defrules.mak

$(TARGETS): $(OBJS)
	 @$(ECHO) Building $@
	 $(BUILD_LIBRARY)
clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $(OBJS)
	$(RM) $($(ARCHITECTURE)_TARGETS)


# Local header dependencies
event$(OBJ_SUFFIX): stevtint.h event.h event.c mem_handler.h
stevt$(OBJ_SUFFIX): event.h stevt.c mem_handler.h
mem_handler$(OBJ_SUFFIX): mem_handler.c mem_handler.h

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample ST20/ST40/ST200 makefile
