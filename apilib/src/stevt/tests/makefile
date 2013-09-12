# vi:noexpandtab
# STEVT ST20/ST40/ST200 makefile

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

#HEADER_IMPORTS := stsys - no longer needed
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    ifeq "$(DVD_OS)" "OS40"
        HEADER_IMPORTS += os20emu
    endif
endif



ifeq ($(DVD_OS),LINUX)

IMPORTS	:= stos stcommon stevt

STTBX_NO_UART := "true"
STEVT_NO_TBX := "true"

else
IMPORTS := stos stcommon stboot stevt

endif

EXPORTS :=
TARGETS := evt_test$(EXE_SUFFIX)

ifndef STEVT_NO_TBX
IMPORTS += sttbx
endif

ifdef STEVT_NO_TBX
CFLAGS :=  $(CFLAGS) -DSTEVT_NO_TBX
endif

ifdef STEVT_NO_PARAMETER_CHECK
CFLAGS += -DSTEVT_NO_PARAMETER_CHECK
endif

# Leave the decision to the user for whther we want UART or not:
ifndef STTBX_NO_UART
ifndef STEVT_NO_TBX
IMPORTS += stuart stpio
endif
endif

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

CFLAGS := $(CFLAGS) -DSTTBX_REPORT -DSTTBX_PRINT

# The test components to be built...

ifeq ($(DVD_OS),LINUX)
TEST_COMPS = evt_test  TComp_1_1 TComp_1_2 TComp_2_1 TComp_2_2 \
             TComp_2_3 TComp_3_1 TComp_3_2 TComp_3_3 TComp_4_1 TComp_4_2 \
             TComp_4_2 TComp_5_1 TComp_5_2 TComp_6_1 TComp_7_1 TComp_8_1 \
             MITests MIDrva MIDrvb TComp_1_3 MultiName ThreadTests ListLock \
             ConcNotify ReEnter
else
TEST_COMPS = evt_test initterm TComp_1_1 TComp_1_2 TComp_2_1 TComp_2_2 \
             TComp_2_3 TComp_3_1 TComp_3_2 TComp_3_3 TComp_4_1 TComp_4_2 \
             TComp_4_2 TComp_5_1 TComp_5_2 TComp_6_1 TComp_7_1 TComp_8_1 \
             MITests MIDrva MIDrvb TComp_1_3 MultiName ThreadTests ListLock \
             ConcNotify ReEnter
endif

OBJS := $(foreach i, $(TEST_COMPS), $(addprefix $i, $(OBJ_SUFFIX)))

include $(DVD_MAKE)/defrules.mak

$(TARGETS): $(OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(EXTRA_LINKING_$(ARCHITECTURE)) 

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(TARGETS)
	-$(RM) *$(OBJ_SUFFIX)
	-$(RM) *$(LIB_SUFFIX)
	-$(RM) *$(EXE_SUFFIX)
	-$(RM) $(subst .lku,.dbg,$(TARGETS))
	-$(RM) $(subst .lku,.map,$(TARGETS))

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End of STEVT ST20/ST40/ST200 makefile
