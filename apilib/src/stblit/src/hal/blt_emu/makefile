#########################
# MODULE : emu.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Debug only : uncomment and update the lines :
#----------------------------------------------

#CFLAGS := $(CFLAGS) -DSTTBX_REPORT -DSTTBX_INPUT

ifeq "$(DVD_BACKEND)" "GX1"
CFLAGS := $(CFLAGS) -fsigned-char  $(BLT_DEFS)
else
  ifeq "$(ARCHITECTURE)" "ST200"
    CFLAGS := $(CFLAGS) -fsigned-char  $(BLT_DEFS)
  else
    ifeq "$(ARCHITECTURE)" "ST40"
      CFLAGS := $(CFLAGS) -fsigned-char  $(BLT_DEFS)
    else
      CFLAGS := $(CFLAGS) -fshift -fsigned-char  $(BLT_DEFS)
    endif
  endif
endif

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := blt_emu.tco gam_emul_blitter_core.tco  \
gam_emul_global.tco gam_emul_share.tco gam_emul_util.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
ST20_TARGETS := blt_emu.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
#ifneq "$(ST40_OS)" "OS21"
#        ST40_IMPORTS := $(ST20_IMPORTS) os20emu
#else
	ST40_IMPORTS := $(ST20_IMPORTS)
#endif
ST200_IMPORTS := $(ST20_IMPORTS)

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif
ifeq "$(ARCHITECTURE)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)
endif

# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning Gamma
	-$(RM) $($(OBJECT_DIRECTORY)_OBJS)
	-$(RM) $($(OBJECT_DIRECTORY)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif


########################

