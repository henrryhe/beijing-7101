# environment.mak file
#
# Copyright (C) 1999 STMicroelectronics
#
# This file determines the type of build and sets up various
# variables based on options in the build environment, rather
# than the makefile.
#

# Figure out if we are running on a Unix or PC host
# If the value of $SHELL includes the sequence
# sh.exe then assume we are building on a
# PC host using gmake, otherwise assume a Unix host.
#

UNIX_SLASH := /
DOS_SLASH := $(subst /,\,$(UNIX_SLASH))

ifeq ($(strip $(DVD_HOST)),)
  ifneq (,$(findstring sh.exe,$(SHELL)))
    DVD_HOST := pc
  else
    DVD_HOST := unix
  endif
endif

ifeq ($(DVD_HOST),unix)
  SLASH := $(UNIX_SLASH)
  GOOD_SLASH := $(UNIX_SLASH)
  BAD_SLASH := $(DOS_SLASH)
  ifeq ($(DVD_TOOLSET),SPARC)
    DVD_BUILD_DIR := C:$(shell pwd)
  else
    DVD_BUILD_DIR := $(shell pwd)
  endif
else
  ifeq "$(DVD_HOST)" "pc-cygwin"
    SLASH := $(UNIX_SLASH)
    GOOD_SLASH := $(UNIX_SLASH)
    BAD_SLASH := $(DOS_SLASH)
    DVD_BUILD_DIR := $(shell pwd)
    DVD_ROOT := $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_ROOT))
  else
  SLASH := $(DOS_SLASH)
  GOOD_SLASH := $(DOS_SLASH)
  BAD_SLASH := $(UNIX_SLASH)
  DVD_BUILD_DIR := $(shell cd)
endif
endif

ifneq "$(origin IN_OBJECT_DIR)" "undefined"
  # Building within an object directory for this target.
  # In this case we override the build dir with the
  # parent directory (the one with the source in).
  DVD_BUILD_DIR := $(DVD_PARENT_DIR)

  # Make sure sub-make calls don't inherit the
  # command-line parameters.
  MAKEOVERRIDES =
  unexport IN_OBJECT_DIR
  unexport DVD_PARENT_DIR
endif

# Settings:
# 
#     DVD_HOST = pc/win98   Make will build using DOS commands and paths
#     DVD_HOST = unix       Make will build using UNIX commands and paths

ifeq "$(DVD_HOST)" "unix"
        CAT     := cat
        CD      := cd
        CLS     := clear
        CP      := cp -fp
        DIFF    := cmp
        ECHO    := echo
        NULL    := /dev/null
        RM      := rm -f
        RMDIR   := rmdir
        MKDIR   := mkdir -p
        FULLRM  := rm -rf
        RENAME  := mv
        LINT    := flint
else
  ifeq "$(DVD_HOST)" "pc-cygwin"
        CAT     := cat
        CD      := cd
        CP      := cp -fp
        DIFF    := cmp
        ECHO    := echo
        NULL    := NUL
        RM      := rm -f
        RMDIR   := rmdir
        MKDIR   := mkdir -p
        FULLRM  := rm -rf
        RENAME  := mv
  else
  ifeq "$(DVD_HOST)" "pc"
        CAT     := type
        CD      := cd
        CLS     := cls
        CP	:= cmd /c copy
        DIFF    := fc /B
        ECHO    := cmd /c echo
        NULL    := NUL:
        RM      := -del
        RMDIR   := cmd /c rmdir
        MKDIR   := mkdir
        FULLRM  := cmd /c rmdir /s /q
        RENAME  := cmd /c rename
  else
    # For win98/win95 we remove all cmd /c parts
        CAT     := type
        CD      := cd
        CLS     := cls
        CP	:= copy
        DIFF    := fc /B
        ECHO    := echo
        NULL    := NUL:
        RM      := -del
        RMDIR   := rmdir
        MKDIR   := mkdir
        FULLRM  := rmdir /s /q
        RENAME  := rename
  endif
  endif

  ifndef LINT_PATH
        LINT_PATH := c:$(SLASH)lint
  endif
        LINT    := $(LINT_PATH)$(SLASH)lint-nt
endif

ifeq "$(DVD_TOOLSET)" "SPARC"
        CP      := cp -fp
        RENAME  := mv -f
endif

# Default the architecture
ifndef ARCHITECTURE
  ARCHITECTURE := ST20
endif

ifneq "$(filter $(ARCHITECTURE),ST20 ST40 ST200 SPARC)" "$(ARCHITECTURE)"
  $(error Invalid ARCHITECTURE ($(ARCHITECTURE)))
endif

# Default DVD_OS
ifndef DVD_OS
  ifeq "$(ARCHITECTURE)" "ST20"
    DVD_OS := OS20
  endif
  ifeq "$(ARCHITECTURE)" "SPARC"
    DVD_OS := OS20
  endif
  ifeq "$(ARCHITECTURE)" "ST40"
    DVD_OS := OS21
  endif
  ifeq "$(ARCHITECTURE)" "ST200"
    DVD_OS := OS21
  endif
endif

ifneq "$(filter $(DVD_OS),OS20 OS21 LINUX)" "$(DVD_OS)"
  $(error Invalid DVD_OS ($(DVD_OS)))
endif

# Create lowercase versions of the same
ifeq "$(DVD_OS)" "OS20"
  DVD_OS_LC := os20
endif
ifeq "$(DVD_OS)" "OS21"
  DVD_OS_LC := os21
endif
ifeq "$(DVD_OS)" "LINUX"
  DVD_OS_LC := linux
endif

# Default the toolset if one is not defined
ifeq "$(origin DVD_TOOLSET)" "undefined"
  ifeq "$(filter $(DVD_OS),LINUX)" "$(DVD_OS)"
    # For these rtos we set the TOOLSET differently
    DVD_TOOLSET := $(DVD_OS)
  else
  DVD_TOOLSET := $(ARCHITECTURE)
endif
endif

#
# Check the setting of some basic configuration variables.
# From that we also set certain variables which effect the
# overall behaviour of the make system.
#

ifeq "$(origin DVD_ROOT)" "undefined"
  DVD_ROOT := $(dir $(DVD_MAKE))src
endif

ifeq "$(origin DVD_MAKE_VERSION)" "undefined"
  DVD_MAKE_VERSION := 1
endif

# Check that the make version is recognised.

ifeq "$(findstring $(DVD_MAKE_VERSION),1 2)" ""
  $(error Unrecognised DVD_MAKE_VERSION ($(DVD_MAKE_VERSION)))
endif

ifeq "$(origin DVD_$(ARCHITECTURE)_EXPORTS)" "undefined" 
  ifdef DVD_EXPORTS
    DVD_$(ARCHITECTURE)_EXPORTS := $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_EXPORTS))
  endif
endif

ifeq "$(origin DVD_$(ARCHITECTURE)_EXPORTS)" "undefined"
  # lib directory at the same level as the src dir
  DVD_EXPORTS := $(dir $(DVD_ROOT))lib
  DVD_$(ARCHITECTURE)_EXPORTS := $(DVD_EXPORTS)
  DVD_EXPORT_LIBS := false
else
  DVD_EXPORT_LIBS := true
endif

ifeq "$(origin DVD_INCLUDE_EXPORTS)" "undefined"
  DVD_INCLUDE_EXPORTS := $(DVD_MAKE)
  DVD_EXPORT_HEADERS := false
else
  DVD_EXPORT_HEADERS := true
endif

# Check whether we are building in a VOB or tree environment
DVD_TEMP := $(wildcard $(DVD_ROOT)/*-prj-*)
ifeq "$(DVD_TEMP)" ""
  DVD_BUILDING_IN_VOB := false
else
  DVD_BUILDING_IN_VOB := true
endif

# Check for in-place include location (if a specific path is not provided)
ifeq "$(origin DVD_INCLUDE)" "undefined"
  ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    # If not building in VOB, the only way this will work is if we
    # have an include directory. We try find this at DVD_ROOT/include
    DVD_INCLUDE := $(wildcard $(dir $(DVD_ROOT))include)
    ifeq "$(DVD_INCLUDE)" ""
      $(error DVD_INCLUDE must be set for non-vob build)
    endif

    # We have manager to set the include directory, so not in-place build
    IN_PLACE_INCLUDES := false
  else
    IN_PLACE_INCLUDES := true
  endif
else
  IN_PLACE_INCLUDES := false
endif

# Set up include info depending whether we are building in VOB
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  # Release build
  ifeq "$(origin DVD_CONFIG)" "undefined"
    # *.cfg files in config dir at same level as src
    DVD_CONFIG := $(wildcard $(dir $(DVD_ROOT))config)
  endif

  PLATFORM_DIR := $(DVD_CONFIG)/platform
  CHIP_DIR := $(DVD_CONFIG)/chip
  BOARD_DIR := $(DVD_CONFIG)/board
else
  # VOB build
  ifndef SYSTEM_INCLUDE_DIR
    SYSTEM_INCLUDE_DIR := $(wildcard $(DVD_ROOT)/*prj-include)
  endif
  PLATFORM_DIR := $(wildcard $(DVD_ROOT)/*-prj-platform)
  ifeq "$(PLATFORM_DIR)" ""
    $(error Failed to find platform config directory)
  endif

  CHIP_DIR := $(wildcard $(DVD_ROOT)/*-prj-chip)
  ifeq "$(CHIP_DIR)" ""
    $(error Failed to find chip config directory)
  endif

  BOARD_DIR := $(wildcard $(DVD_ROOT)/*-prj-board)
  ifeq "$(BOARD_DIR)" ""
    $(error Failed to find board config directory)
  endif

  ifndef DVD_CONFIG
    # *.cfg files in boards directory
    DVD_CONFIG := $(BOARD_DIR)
  endif
  ifeq "$(DVD_CONFIG)" ""
    $(error Failed to find DVD_CONFIG directory)
  endif
endif

ifndef DVD_TARGET_PATH
  DVD_TARGET_PATH := .
endif

ifdef DVD_DEPENDS
  ifeq "$(DVD_DEPENDS)" "no"
    SUPPRESS_IMPORT_DEPENDS := true
    ADD_AUTOMATIC_DEPENDS := false
    TOPLEVEL_AUTOMATIC_DEPENDS := false
  else
    ifeq "$(DVD_DEPENDS)" "yes"
      SUPPRESS_IMPORT_DEPENDS := false
      ADD_AUTOMATIC_DEPENDS := false
      TOPLEVEL_AUTOMATIC_DEPENDS := false
    else
      ifeq "$(DVD_DEPENDS)" "top"
        SUPPRESS_IMPORT_DEPENDS := false
        ADD_AUTOMATIC_DEPENDS := false
        TOPLEVEL_AUTOMATIC_DEPENDS := true
      else
        ifeq "$(DVD_DEPENDS)" "all"
          SUPPRESS_IMPORT_DEPENDS := false
          ADD_AUTOMATIC_DEPENDS := true
          TOPLEVEL_AUTOMATIC_DEPENDS := true
        else
          $(error Invalid DVD_DEPENDS ($(DVD_DEPENDS))- options yes, no and all (default yes))
        endif
      endif
    endif
  endif
else
  SUPPRESS_IMPORT_DEPENDS := false
  ADD_AUTOMATIC_DEPENDS := false
  TOPLEVEL_AUTOMATIC_DEPENDS := false
endif

ifdef DVD_BUILD_VARIANT
  OBJECT_DIRECTORY := $(DVD_BUILD_VARIANT)
else
  ifeq "$(DVD_OS)" "LINUX"
    OBJECT_DIRECTORY := $(DVD_OS)
  else
  OBJECT_DIRECTORY := $(ARCHITECTURE)
  endif
endif

# End of file
