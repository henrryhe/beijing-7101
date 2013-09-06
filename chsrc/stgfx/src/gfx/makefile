# vi:noexpandtab
######################
# MODULE : stgfx.lib #
######################


DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ifdef OSD_WRAPPER
  DVD_CFLAGS := $(DVD_CFLAGS) -DOSD_WRAPPER
endif

HEADER_IMPORT += stblit stgxobj sttbx stavmem

SUB_COMPONENT := $(notdir $(DVD_BUILD_DIR))

ST20_TARGETS := $(SUB_COMPONENT).lib
ST20_OBJS    := stgfx_init.tco gfx_common.tco

PATH_SRC_DIR := $(dir $(DVD_BUILD_DIR))
   
INCLUDE_PATH := $(PATH_SRC_DIR)/bdf $(PATH_SRC_DIR)txt $(PATH_SRC_DIR)olf $(INCLUDE_PATH)

# Scalability support
include $(PATH_SRC_DIR)/scaling.mak

# Some files are turned entirely on/off by scalability options:
#  (others have #ifdefs in the code)
ifdef STGFX_INCLUDE_LINES
ST20_OBJS += gfx_line.tco gfx_polygon.tco
endif
ifdef STGFX_INCLUDE_ARCS
ST20_OBJS += gfx_arc.tco gfx_sector.tco gfx_roundrect.tco
endif
ifdef STGFX_INCLUDE_ARCSLINES
# Turn on joins, caps, etc
ST20_OBJS += gfx_tools.tco gfx_conic.tco gfx_texture.tco
endif
ifdef STGFX_INCLUDE_TEXT
ST20_OBJS += gfx_text.tco
endif
ifdef STGFX_INCLUDE_COPY
ST20_OBJS += gfx_copy.tco
endif
ifdef STGFX_INCLUDE_FLOODFILL
ST20_OBJS += gfx_seedfill.tco
endif

ST40_IMPORTS := os20emu
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

$(ST20_TARGETS): $(ST20_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS)
	 @$(ECHO)  Building $@
	 $(BUILD_LIBRARY)

clean:
	@echo Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

# Local Dependancies


else

include $(DVD_MAKE)/builddir.mak

endif
