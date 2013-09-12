# kbuild.mak file
#
# Copyright (C) 2006 STMicroelectronics
#
# This adds support for the KBUILD system

-include $(PLATFORM_DIR)/$(DVD_PLATFORM).mak
include $(DVD_MAKE)/sysconf.mak
include $(DVD_MAKE)/environment.mak
include $(DVD_MAKE)/toolset.mak

# Set up appropriate include path
ifeq "$(IN_PLACE_INCLUDES)" "true"
  # Doing in-place header include
  ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,$(HEADER_IMPORTS)))
  else
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/*-prj-,$(HEADER_IMPORTS)))
  endif
else
  # Non in-place headers (which happens when DVD_INCLUDE is set)
  INCLUDE_PATH += $(DVD_INCLUDE)
  ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,$(HEADER_IMPORTS)))
  endif
  ifeq "$(DVD_EXPORT_HEADERS)" "true"
    INCLUDE_PATH += $(DVD_INCLUDE_EXPORTS)
  endif
endif

INCLUDE_PATH := $(strip $(DVD_BUILD_DIR) $(INCLUDE_PATH) $(CHIP_DIR) \
                        $(BOARD_DIR) $(PLATFORM_DIR) $(SYSTEM_INCLUDE_DIR))

DVD_INCLUDE_PATH := $(addprefix -I,$(INCLUDE_PATH))

KBUILD_CFLAGS := -D$(DVD_PLATFORM) -DST_$(DVD_FRONTEND)
KBUILD_CFLAGS += -DARCHITECTURE_$(ARCHITECTURE) -DST_$(DVD_OS)
# These should be removed in favour of ST_LINUX
KBUILD_CFLAGS += -DST_OSLINUX
KBUILD_CFLAGS += -Wall

ifneq "$(DVD_FRONTEND)" "$(DVD_BACKEND)"
  KBUILD_CFLAGS += -DST_$(DVD_BACKEND)
endif

# Kernel build needs these defined otherwise it will build using gcc
# and not gcc for the target
ifeq "$(ARCHITECTURE)" "ST40"
  CROSS_COMPILE   := sh4-linux-
  ARCH            := sh
else
  ifeq "$(ARCHITECTURE)" "ST200"
    CROSS_COMPILE   := st200
    ARCH            := $(error not defined)
  endif
endif

export CROSS_COMPILE ARCH


# End
