############################
# MODULE : parser.lib      #
############################

# video specific variables
VID_OTHER_MODULES_DEPENDENCIES =
ifneq ($(findstring inject,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) inject
endif
ifneq ($(findstring fgt,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) fgt
endif

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := vc1parser.tco bdu.tco parsebdu.tco

ST20_TARGETS := parser.lib
ST20_IMPORTS := sttbx stevt stfdma stavmem stos

ifdef VALID_TOOLS
     ST20_IMPORTS += valitls
else
     ifdef STVID_TRACE_BUFFER_UART
         INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../../tests/src
     endif
endif


ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(ST20_EXPORTS)
ST40_IMPORTS := $(ST20_IMPORTS)

# General rules
CFLAGS := $(VID_DEFS) $(CFLAGS)

ifdef STVID_PARSER_DO_NOT_LOOP_ON_STREAM
  CFLAGS += -DSTVID_PARSER_DO_NOT_LOOP_ON_STREAM
endif
ifdef STVID_PARSER_CHECK
  CFLAGS += -DSTVID_PARSER_CHECK
endif
ifndef STUB_INJECT
    ifndef STVID_NOT_USE_VC1_MP_SC_SIGNATURE
        CFLAGS += -DSTVID_USE_VC1_MP_SC_SIGNATURE
    endif
endif
ifdef STVID_PARSER_DUMP
	CFLAGS := $(CFLAGS) -DSTVID_PARSER_DUMP
endif

CFLAGS += $(addprefix -D ST_,$(VID_OPTIONAL_MODULES))

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../../.. $(DVD_BUILD_DIR)/../../../..
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../inject
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../fgt

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

clean_message: FORCE
	@$(ECHO) Cleaning VC1 Video CODEC $(ST20_TARGETS)

clean: clean_message $(SUBDIR_CLEAN)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

FORCE:


# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
vc1parser.tco vc1parser.o : ./vc1parser.h
bdu.tco       bdu.o : ./vc1parser.h
parsebdu.tco  parsebdu.o : ./vc1parser.h

#end of file
