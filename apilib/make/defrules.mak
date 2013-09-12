# defrules.mak file
# 
# Copyright (C) 1999 STMicroelectronics
#
# This file holds all the default targets and rules necessary
# to build the individual components and the tree. 
# All component makefiles include this file so
# a change here is reflected in all components.
#

# Maximum MAKELEVEL limit to prevent inifinite recursion
DVD_MAKELIMIT = 15

# This group of variables must be checked here, rather than in
# generic.mak because they are specified in the makefile,
# after the generic.mak is included.

ifndef ST20_TARGETS
  ST20_TARGETS = $(TARGETS)
endif

ifndef ST20_EXPORTS
  ST20_EXPORTS = $(EXPORTS)
endif

ifndef ST20_IMPORTS
  ST20_IMPORTS = $(IMPORTS)
endif

ifndef SPARC_TARGETS
  ifndef TARGETS
        SPARC_TARGETS = $(ST20_TARGETS)
  else
        SPARC_TARGETS = $(TARGETS)
  endif
endif

ifndef SPARC_EXPORTS
  ifndef EXPORTS
        SPARC_EXPORTS = $(ST20_EXPORTS)
  else
        SPARC_EXPORTS = $(EXPORTS)
  endif
endif


ifndef SPARC_IMPORTS
  ifndef IMPORTS
        SPARC_IMPORTS = $(ST20_IMPORTS)
  else
        SPARC_IMPORTS = $(IMPORTS)
  endif
endif

ifdef $(DVD_OS)_TARGETS
  DEFAULT_TARGETS := $($(DVD_OS)_TARGETS)
  BUILDING_FOR    := $(DVD_OS)
else
  DEFAULT_TARGETS := $($(ARCHITECTURE)_TARGETS)
  BUILDING_FOR    := $(ARCHITECTURE)
endif

ifdef $(DVD_OS)_IMPORTS
  DEFAULT_IMPORTS := $($(DVD_OS)_IMPORTS)
else
  DEFAULT_IMPORTS := $($(ARCHITECTURE)_IMPORTS)
endif

ifdef $(DVD_OS)_EXPORTS
  DEFAULT_EXPORTS := $($(DVD_OS)_EXPORTS)
else
  DEFAULT_EXPORTS := $($(ARCHITECTURE)_EXPORTS)
endif

ifdef DVD_$(DVD_OS)_EXPORTS
  DVD_DEFAULT_EXPORTS := $(DVD_$(DVD_OS)_EXPORTS)
else
  DVD_DEFAULT_EXPORTS := $(DVD_$(ARCHITECTURE)_EXPORTS)
endif

ifdef $(DVD_OS)_CFLAGS
  DEFAULT_CFLAGS := $($(DVD_OS)_CFLAGS)
else
  DEFAULT_CFLAGS := $($(ARCHITECTURE)_CFLAGS)
endif

ifdef $(DVD_OS)_LKFLAGS
  DEFAULT_LKFLAGS := $($(DVD_OS)_LKFLAGS)
else
  DEFAULT_LKFLAGS := $($(ARCHITECTURE)_LKFLAGS)
endif

# The generic CFLAGS are added
DEFAULT_CFLAGS += $(CFLAGS)

# The base LKFLAGS are added
DEFAULT_LKFLAGS += $(LKFLAGS)

ifeq "$(DVD_OS)" "LINUX"
  # This defines the subdirectory structure within EXPORTS to copy LINUX
  # components to
  OS_LIBRARY_SUBDIR  := lib/
  OS_MODULE_SUBDIR   := lib/modules/
  OS_UTIL_SUBDIR     := bin/
  OS_MISC_SUBDIR     := etc/
  OS_TEST_SUBDIR     := share/bin/
  OS_TEST_LIB_SUBDIR := share/lib/
  OS_TEST_MOD_SUBDIR := share/lib/modules/
endif

# Set up appropriate include path
ifeq "$(IN_PLACE_INCLUDES)" "true"

  # In-place includes are quite complex (to maintain backward compatibility)

  # First, in compatibility mode, add base header imports
  ifeq "$(DVD_MAKE_VERSION)" "1"
    # Remove the duplicated items from the path
    DVD_TEMP := $(filter-out $(HEADER_IMPORTS),$(DVD_BASE_HEADER_IMPORTS))
    DVD_TEMP := $(filter-out $(DEFAULT_IMPORTS),$(DVD_TEMP))

    ifeq "$(DVD_BUILDING_IN_VOB)" "false"
      INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,$(DVD_TEMP)))
    else
      INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/*-prj-,$(DVD_TEMP)))
    endif
  endif

  # This is just to deal with in-place include of PTI (not STPTI)
  ifeq "$(findstring pti,$(DEFAULT_IMPORTS))" "pti"
    IMPORTS_LIST := $(DEFAULT_IMPORTS) pti/harm/src/pti
  else
    IMPORTS_LIST := $(DEFAULT_IMPORTS)
  endif

  # Next, add header imports and regular imports (header imports will probably
  # not be defined when using an old make file).
  ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,$(HEADER_IMPORTS)))
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,$(IMPORTS_LIST)))
  else
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/*-prj-,$(HEADER_IMPORTS)))
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/*-prj-,$(IMPORTS_LIST)))
  endif
else
  # Non in-place headers (which happens when DVD_INCLUDE is set) is simpler
  # because that is the main part of the include path.
  INCLUDE_PATH += $(DVD_INCLUDE)
  ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,$(HEADER_IMPORTS)))
  endif
  ifeq "$(DVD_EXPORT_HEADERS)" "true"
    INCLUDE_PATH += $(DVD_INCLUDE_EXPORTS)
  endif
endif

INCLUDE_PATH := $(strip $(DVD_BUILD_DIR) $(INCLUDE_PATH) $(CHIP_DIR) $(BOARD_DIR) \
                        $(PLATFORM_DIR) $(SYSTEM_INCLUDE_DIR))

# Add the subdirs onto the include path
INCLUDE_PATH := $(strip $(addprefix $(DVD_BUILD_DIR)/,$(SUBDIRS)) $(INCLUDE_PATH))

ifeq "$(DVD_MAKE_VERSION)" "1"
  INCLUDES += $(BASE_INCLUDES)
endif

# Set up the appropriate library path
ifeq "$(DVD_EXPORT_LIBS)" "true"
  # The last one in the list is for *.cfg files
  LIBRARY_PATH = $(strip $(LINK_PATH) $(SUBDIR_LIBRARY_PATH) $(LIBRARY_DIR) $(CFG_PATH))
else
  # Not exporting the libraries, so build up full path from imports
  ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    LIBRARY_PATH_PREFIX := $(DVD_ROOT)/
  else
    LIBRARY_PATH_PREFIX := $(DVD_ROOT)/*-prj-
  endif

  LIBRARY_PATH = $(strip $(LINK_PATH) $(SUBDIR_LIBRARY_PATH) \
                         $(foreach COMPONENT,$(DEFAULT_IMPORTS), \
                           $(if $(filter-out pti,$(COMPONENT)),\
			        $(if $(wildcard $(LIBRARY_PATH_PREFIX)$(COMPONENT)/objs/$(OBJECT_DIRECTORY)), \
				     $(wildcard $(LIBRARY_PATH_PREFIX)$(COMPONENT)/objs/$(OBJECT_DIRECTORY)), \
				     $(wildcard $(LIBRARY_PATH_PREFIX)$(COMPONENT))),\
                                $(if $(wildcard $(LIBRARY_PATH_PREFIX)$(COMPONENT)/harm/src/pti),\
                                     $(wildcard $(LIBRARY_PATH_PREFIX)$(COMPONENT)/harm/src/pti),\
                                     $(wildcard $(LIBRARY_PATH_PREFIX)$(COMPONENT))\
                                 )\
                            )\
                          ) $(CFG_PATH))
endif

ifeq "$(DVD_MAKE_VERSION)" "1"
  ifneq "$(LIBRARIES)" "$(addprefix $(LIBRARY_PREFIX),$(LIBRARY_PATH))"
    LIBRARIES += $(addprefix $(LIBRARY_PREFIX),$(LIBRARY_PATH))
  endif
endif

ifneq "$(wildcard $(DVD_USER_CONFIG))" ""
  CFG_PATH += $(DVD_USER_CONFIG)/chip $(DVD_USER_CONFIG)/board $(DVD_USER_CONFIG)/platform
endif
CFG_PATH += $(CHIP_DIR) $(BOARD_DIR) $(PLATFORM_DIR) $(DVD_TARGET_PATH)
CFG_PATH := $(strip $(CFG_PATH))

# Search for headers in central include dir and local dirs
SPACE := $(NOTHING) $(NOTHING)
VPATH_INCLUDE_PATH := $(subst $(SPACE),:,$(INCLUDE_PATH))

ifneq "$(DVD_MAKE_VERSION)" "1"
  vpath %.h  $(VPATH_INCLUDE_PATH)
else
  EX_VPATH=$(subst :, ,$(strip $(subst $(INCLUDE_PREFIX), ,$(INCLUDES))))
  vpath %.h  $(VPATH_INCLUDE_PATH):$(EX_VPATH)
endif

# For object dir build, need vpath to locate the C files.
vpath %.c  $(DVD_BUILD_DIR)

.PHONY: default start finish
.PHONY: error_checks copy_libraries copy_headers
.PHONY: copy_modules copy_utils copy_misc copy_tests copy_test_mods



ifeq "$(ADD_AUTOMATIC_DEPENDS)" "true"
  EXTRA_TARGETS := $(IMPORT_LIBS)
endif

ifeq "$(TOPLEVEL_AUTOMATIC_DEPENDS)" "true"
  ifdef IN_OBJECT_DIR
    ifeq "$(MAKELEVEL)" "1"
      EXTRA_TARGETS := $(IMPORT_LIBS)
    endif
  else
    ifeq "$(MAKELEVEL)" "0"
      EXTRA_TARGETS := $(IMPORT_LIBS)
    endif
  endif
endif

default: error_checks start $(DEFAULT_TARGETS) $(EXTRA_TARGETS) finish

# Provide sub-directory library build support.
# Libraries are named according to sub-directory name.
SUBDIR_LIBS := $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(SUBDIRS)))
SUBDIR_CLEAN := $(addsuffix _clean,$(SUBDIRS))

# Set up subdir library path to include the subdirs
SUBDIR_LIBRARY_PATH = $(foreach COMPONENT,$(SUBDIRS), \
			   $(if $(wildcard $(DVD_BUILD_DIR)/$(COMPONENT)/objs/$(OBJECT_DIRECTORY)), \
				$(wildcard $(DVD_BUILD_DIR)/$(COMPONENT)/objs/$(OBJECT_DIRECTORY)), \
				$(wildcard $(DVD_BUILD_DIR)/$(COMPONENT))))

$(SUBDIR_LIBS):
	$(MAKE) -C $(DVD_BUILD_DIR)/$(LIBNAME_FROM_AT)

$(SUBDIR_CLEAN):
	$(MAKE) -C $(DVD_BUILD_DIR)/$(subst _clean,,$@) clean

# Ensure environment variables are set
#
# NB: No need to check DVD_MAKE as if it's not set
# correctly then this file will not run
#
# DVD_ROOT is a special case as it maybe empty
# indicating DVD_ROOT is the root directory
# but it MUST still be defined

ifneq ($(strip $(DVD_ROOT)),)
  ifeq ($(wildcard $(DVD_ROOT)/),)
    $(error DVD_ROOT invalid ($(DVD_ROOT)))
  endif
endif

error_checks:
  ifeq "$(DVD_EXPORT_LIBS)" "true"
    ifeq ($(wildcard $(DVD_DEFAULT_EXPORTS)/),)
        # Make the directory.
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_DEFAULT_EXPORTS)) > $(NULL)
    endif
    ifneq "$(OS_LIBRARY_SUBDIR)" ""
      ifeq ($(wildcard $(DVD_DEFAULT_EXPORTS)/$(OS_LIBRARY_SUBDIR)),)
      	# Make the directory.
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_DEFAULT_EXPORTS)/$(OS_LIBRARY_SUBDIR)) > $(NULL)
    endif
  endif
    ifneq "$(OS_MODULE_SUBDIR)" ""
      ifeq ($(wildcard $(DVD_DEFAULT_EXPORTS)/$(OS_MODULE_SUBDIR)),)
        # Make the directory.
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_DEFAULT_EXPORTS)/$(OS_MODULE_SUBDIR)) > $(NULL)
      endif
    endif
    ifneq "$(OS_UTIL_SUBDIR)" ""
      ifeq ($(wildcard $(DVD_DEFAULT_EXPORTS)/$(OS_UTIL_SUBDIR)),)
        # Make the directory.
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_DEFAULT_EXPORTS)/$(OS_UTIL_SUBDIR)) > $(NULL)
      endif
    endif
    ifneq "$(OS_MISC_SUBDIR)" ""
      ifeq ($(wildcard $(DVD_DEFAULT_EXPORTS)/$(OS_MISC_SUBDIR)),)
        # Make the directory.
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_DEFAULT_EXPORTS)/$(OS_MISC_SUBDIR)) > $(NULL)
      endif
    endif
    ifneq "$(OS_TEST_SUBDIR)" ""
      ifeq ($(wildcard $(DVD_DEFAULT_EXPORTS)/$(OS_TEST_SUBDIR)),)
        # Make the directory.
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_DEFAULT_EXPORTS)/$(OS_TEST_SUBDIR)) > $(NULL)
      endif
    endif
    ifneq "$(OS_TEST_MOD_SUBDIR)" ""
      ifeq ($(wildcard $(DVD_DEFAULT_EXPORTS)/$(OS_TEST_MOD_SUBDIR)),)
        # Make the directory.
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_DEFAULT_EXPORTS)/$(OS_TEST_MOD_SUBDIR)) > $(NULL)
      endif
    endif
  endif
  ifeq "$(DVD_EXPORT_HEADERS)" "true"
    ifeq ($(wildcard $(DVD_INCLUDE_EXPORTS)/),)
        # Make the directory.
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_INCLUDE_EXPORTS)) > $(NULL)
    endif
  endif

# Start by echoing the TARGETS file name
start:
	@$(ECHO) ---- Building $(DEFAULT_TARGETS) ----

# Do the export phase, if required
finish: copy_libraries copy_headers copy_modules copy_utils copy_misc copy_tests copy_test_mods
	@$(ECHO) ---- $(DEFAULT_TARGETS) complete ----

ifeq "$(DVD_EXPORT_LIBS)" "true"
EXPORTED_LIBRARIES := $(addprefix $(DVD_DEFAULT_EXPORTS)/$(OS_LIBRARY_SUBDIR),$(EXPORT_LIBS))
copy_libraries: $(EXPORTED_LIBRARIES)

EXPORTED_MODULES := $(addprefix $(DVD_DEFAULT_EXPORTS)/$(OS_MODULE_SUBDIR),$(EXPORT_MODS))
copy_modules: $(EXPORTED_MODULES)

EXPORTED_UTILS := $(addprefix $(DVD_DEFAULT_EXPORTS)/$(OS_UTIL_SUBDIR),$($(DVD_OS)_UTILS_EXPORTS))
copy_utils: $(EXPORTED_UTILS)

EXPORTED_MISC := $(addprefix $(DVD_DEFAULT_EXPORTS)/$(OS_MISC_SUBDIR),$($(DVD_OS)_MISC_EXPORTS))
copy_misc: $(EXPORTED_MISC)

EXPORTED_TESTS := $(addprefix $(DVD_DEFAULT_EXPORTS)/$(OS_TEST_SUBDIR),$($(DVD_OS)_TEST_EXPORTS))
copy_tests: $(EXPORTED_TESTS)

EXPORTED_TEST_MODULES := $(addprefix $(DVD_DEFAULT_EXPORTS)/$(OS_TEST_MOD_SUBDIR),$($(DVD_OS)_TEST_MODULE_EXPORTS))
copy_test_mods: $(EXPORTED_TEST_MODULES)
else
copy_libraries:;
copy_modules:;
copy_utils:;
copy_misc:;
copy_tests:;
copy_test_mods:;
endif

$(EXPORTED_LIBRARIES) $(EXPORTED_MODULES) $(EXPORTED_UTILS) $(EXPORTED_MISC) $(EXPORTED_TESTS) $(EXPORTED_TEST_MODULES): FORCE_COPY
	@$(ECHO) Exporting $(@F)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(@F)) \
              $(subst $(BAD_SLASH),$(GOOD_SLASH),$@) > $(NULL)

FORCE_COPY:

ifeq "$(DVD_EXPORT_HEADERS)" "true"
EXPORTED_HEADERS := $(addprefix $(DVD_INCLUDE_EXPORTS)/,$(EXPORT_HDRS))
copy_headers: $(EXPORTED_HEADERS)
else
copy_headers:;
endif

$(EXPORTED_HEADERS): $(EXPORT_HDRS)
	@$(ECHO) Exporting $(@F)
  ifeq "$(DVD_BUILDING_IN_VOB)" "false"
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_INCLUDE)/$(@F)) \
              $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_INCLUDE_EXPORTS)) > $(NULL)
  else
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_BUILD_DIR)/$(@F)) \
              $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_INCLUDE_EXPORTS)) > $(NULL)
  endif

# Generic rule for imported libraries

ifeq "$(SUPPRESS_IMPORT_DEPENDS)" "false"
$(IMPORT_LIBS):
  ifeq ($(MAKELEVEL),$(DVD_MAKELIMIT))
	@$(ECHO) MAKELEVEL limit reached - cannot sub-make $@
  else
	$(MAKE) -C $(wildcard $(DVD_ROOT)/*prj-$(LIBNAME_FROM_AT_F)) \
		$(wildcard $(DVD_ROOT)/$(LIBNAME_FROM_AT_F))
  endif
else
# Don't build dependencies - assumes they are already done
$(IMPORT_LIBS):;
endif

# List of directories for cleaning
CLEAN_IMPORTS = $(addsuffix _clean,$(DEFAULT_IMPORTS) $(NON_REF_DIRS))

.PHONY: clean clean_libs clean_idb clean_imports $(CLEAN_IMPORTS)

ifndef SUPPRESS_CLEAN_ALL
.PHONY: clean_all
# Rule to clean all imported components and central dirs
clean_all: clean clean_imports clean_libs clean_idb
endif

clean_idb:
ifeq "$(DVD_CODETEST)" "TRUE"
	$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(LIBRARY_DIR)/*$(IDB_SUFFIX)) 
endif

# Rule to remove all exported headers and libs
clean_libs:
ifeq "$(DVD_EXPORT_LIBS)" "true"
  ifneq "$(DVD_OS)" "LINUX"
	@$(ECHO) Cleaning library directory
	$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(LIBRARY_DIR)/$(LIB_PREFIX)*$(LIB_SUFFIX))
	$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(LIBRARY_DIR)/*$(EXE_SUFFIX))
  ifeq "$(ARCHITECTURE)" "ST20"
	$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(LIBRARY_DIR)/*.cfg)
  endif
endif
endif

clean_imports: $(CLEAN_IMPORTS)

$(CLEAN_IMPORTS):
ifeq ($(MAKELEVEL),$(DVD_MAKELIMIT))
	@$(ECHO) MAKELEVEL limit reached - cannot sub-make $@
else
	-$(MAKE) -kC $(wildcard $(DVD_ROOT)/*prj-$(subst _clean,,$@)) \
		     $(wildcard $(DVD_ROOT)/$(subst _clean,,$@)) clean
endif

# Run targets
ifneq "$(DVD_OS)" "LINUX"
  # LINUX does not have an exe suffix and also does not have a run rule
  RUN_TARGETS := $(filter %$(EXE_SUFFIX),$(DEFAULT_TARGETS))
endif
FIRST_TARGET := $(word 1,$(RUN_TARGETS))

ifneq "$(DVD_MAKE_VERSION)" "1"

# Define this to suppress the run and default automatic targets
ifndef SUPPRESS_RUN_TARGETS

run: $(FIRST_TARGET)_RUN

$(addsuffix _RUN,$(RUN_TARGETS)):
ifeq "$(origin TARGET)" "undefined"
	@$(ECHO) "run must have TARGET defined"
	@$(ECHO) "e.g. gmake run TARGET=jei"
else
	@$(ECHO) Running $(subst _RUN,,$@) on $(TARGET)
	$(RUN_OBJECT)
endif

debug: $(FIRST_TARGET)_DEBUG_RUN

$(addsuffix _DEBUG_RUN,$(RUN_TARGETS)):
ifeq "$(origin TARGET)" "undefined"
	@$(ECHO) "debug must have TARGET defined"
	@$(ECHO) "e.g. gmake debug TARGET=jei"
else
	@$(ECHO) Running $(subst _DEBUG_RUN,,$@) with debugger on $(TARGET)
	$(DEBUG_RUN_OBJECT)
endif

endif

else

# A bit of a bodge - because each makefile used to define its own run
# function and we have moved the location of the cfg files from DVD_MAKE,
# we update TARGET to include an extra path specifier.

override TARGET := $(TARGET) $(addprefix -l ,$(CFG_PATH))

endif # DVD_MAKE_VERSION != 1

# Fake rules to stop make searching for implicit 
# rules on files it cannot remake

makefile %.mak: ;

# EOF




