# generic.mak file
#
# Copyright (C) 1999 STMicroelectronics
#
# This file holds all the default rules and variable expansions which
# drive the makesystem. All component makefiles include this file so
# a change here is reflected in all components.
#

# Determine the build environment
include $(DVD_MAKE)/environment.mak

# Include default DVD platform configuration
include $(DVD_MAKE)/sysconf.mak

# Include the multi-chip platform support
include $(DVD_MAKE)/platform.mak

# This file describes the toolchain in use
include $(DVD_MAKE)/toolset.mak

# Define Platform and Frontend CPU device
CFLAGS += -D$(DVD_PLATFORM) -DST_$(DVD_FRONTEND)
CFLAGS += -DARCHITECTURE_$(ARCHITECTURE) -DST_$(DVD_OS)

ifeq "$(DVD_CODETEST)" "TRUE"
  CFLAGS := $(CFLAGS)  -CTexclude-from-tagging=stboot.c    
endif

# Define Backend device if different from frontend
ifneq "$(DVD_FRONTEND)" "$(DVD_BACKEND)"
  CFLAGS := $(CFLAGS) -DST_$(DVD_BACKEND)
endif

# Check for CPU clock override
ifdef DVD_FRONTEND_CPUCLOCK
  CFLAGS := $(CFLAGS) -DCPUCLOCK$(DVD_FRONTEND_CPUCLOCK)
endif

# Define parameters according the DVD_FRONTEND
LINK_DEVICES = 5508 5518 5580
ifeq "$(findstring $(DVD_FRONTEND), $(LINK_DEVICES))" "$(DVD_FRONTEND)"
  CFLAGS := $(CFLAGS) -DST_$(DVD_SERVICE) -DSTLINK
endif

ifdef UNIFIED_MEMORY
  CFLAGS += -DUNIFIED_MEMORY
endif

# Set the selected transport type
OPTIONAL_CFLAGS += -DDVD_TRANSPORT_$(DVD_TRANSPORT_UPPERCASE)

ifeq "$(DVD_TRANSPORT)" "stpti4"
  # Add STPTI too for compatibility
  OPTIONAL_CFLAGS += -DDVD_TRANSPORT_STPTI
endif

# Split EXPORTS into headers and libraries
EXPORT_HDRS = $(filter %.h,$(DEFAULT_EXPORTS))
EXPORT_LIBS = $(strip $(filter $(LIB_PREFIX)%$(LIB_SUFFIX),$(DEFAULT_EXPORTS)))

ifeq "$(filter $(DVD_OS),LINUX)" "$(DVD_OS)"
  # Kernel module exports
  EXPORT_MODS = $(strip $(filter $(MOD_PREFIX)%$(MOD_SUFFIX),$(DEFAULT_EXPORTS)))
endif

# Generate imported headers and their location
IMPORT_HDRS = $(addsuffix .h,$(DEFAULT_IMPORTS))

IMPORT_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(DEFAULT_IMPORTS)))

# Central include directory for imported headers
INCLUDE_DIR = $(DVD_MAKE)

# Construct a list of header file include paths
BASE_INCLUDES = $(addprefix $(INCLUDE_PREFIX),$(INCLUDE_PATH))

# Central directory for exported libs
ifeq "$(filter $(DVD_OS),LINUX)" "$(DVD_OS)"
  LIBRARY_DIR = $(DVD_DEFAULT_EXPORTS)/$(OS_LIBRARY_SUBDIR) $(DVD_DEFAULT_EXPORTS)/$(OS_TEST_LIB_SUBDIR)
else
  LIBRARY_DIR = $(DVD_DEFAULT_EXPORTS)
endif

# Construct a list of library include paths
LIBRARIES = $(addprefix $(LIBRARY_PREFIX),$(LIBRARY_PATH))


# Support for GCC preprocessor checking
ADD_GCC_SUPPORT=no
ifdef GCC_CHECK_SA
  ADD_GCC_SUPPORT=yes
endif
ifdef GCC_CHECK
  ADD_GCC_SUPPORT=yes
endif
ifdef GCC_DEP
  ADD_GCC_SUPPORT=yes
endif

ifeq "$(ADD_GCC_SUPPORT)" "yes"
  ifeq "$(OSTYPE)" "cygwin"
    ST20_TOOLSET_PATH := -I$(shell cygpath -u $$ST20ROOT)/include
    SPARC_TOOLSET_PATH := -I$(shell cygpath -u $$SPARCROOT)/include
    ST40_TOOLSET_PATH := -I$(shell cygpath -u $$ST40ROOT)/include
    ST200_TOOLSET_PATH := -I$(shell cygpath -u $$ST200ROOT)/include
  else
    ST20_TOOLSET_PATH := -I$(ST20ROOT)/include
    SPARC_TOOLSET_PATH := -I$(SPARCROOT)/include
    ST40_TOOLSET_PATH := -I$(ST40ROOT)/include
    ST200_TOOLSET_PATH := -I$(ST200ROOT)/include
  endif

  ifeq "$(ARCHITECTURE)" "ST40"
    DCU_TOOLSET_PATH := -nostdinc $(ST40_TOOLSET_PATH)
  else
    ifeq "$(ARCHITECTURE)" "ST20"
      DCU_TOOLSET_PATH := -nostdinc $(ST20_TOOLSET_PATH)
    else
      ifeq "$(ARCHITECTURE)" "ST200"
        DCU_TOOLSET_PATH := -nostdinc $(ST200_TOOLSET_PATH)
      else
        DCU_TOOLSET_PATH :=
      endif
    endif
  endif

  ifdef GCC_CHECK_SA
    ifeq "$(DVD_TOOLSET)" "ST20"
    CP=echo 
    AR=echo 
    CC=gcc
    CFLAGS += $(DCU_TOOLSET_PATH)
    endif
    CFLAGS += -funsigned-char -msoft-float -fconserve-space  -Wparentheses
    CFLAGS += -fvtable-thunks -Wunused -Wreturn-type -Wchar-subscripts -Wimplicit -Wuninitialized 
# SPARC Option
#   CFLAGS += -mv8 -mflat
# Option inconnue ?
#   CFLAGS += -fembedded-cxx 
  endif 

  ifdef GCC_CHECK 
    ifeq "$(DVD_TOOLSET)" "ST20"
    CP=echo 
    AR=echo 
    CC=gcc 
    CFLAGS += $(DCU_TOOLSET_PATH) # -Wtraditional 
    endif
    CFLAGS += -Wall -Wno-unknown-pragmas -W -ansi -Wparentheses -Wcomment -Wformat
    CFLAGS += -Wshadow -Wbad-function-cast -Wpointer-arith -pedantic
    CFLAGS += -Wcast-qual -Wcast-align -Wwrite-strings -Wsign-compare 
    CFLAGS += -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations 
    CFLAGS += -Wmissing-noreturn -Wredundant-decls -Wnested-externs 
  endif 

  ifdef GCC_DEP
    ifeq "$(DVD_TOOLSET)" "ST20"
    CP=echo 
    AR=echo 
    CC=gcc 
    CFLAGS += $(DCU_TOOLSET_PATH)
    endif
    CFLAGS += -MG -MMD -M
  endif

endif   # GCC preprocessing options

ifeq "$(DVD_TOOLSET)" "SPARC"
  ## Add this line for Scientific Atlanta wrappers development
  CFLAGS := $(CFLAGS) 		-nostdinc             \
                                -mv8                  \
                                -mflat                \
                                -msoft-float          \
                                -funsigned-char       \
                                -fconserve-space      \
                                -fembedded-cxx        \
                                -fvtable-thunks       \
                                -fleading-underscore  \
                                -fpermissive          \
                                -fno-const-strings
  #                             -Wall

  INCLUDES = $(INCLUDE_PREFIX)$(INCLUDE_DIR) $(INCLUDE_PREFIX)$(DIR_OS)/include \
             $(INCLUDE_PREFIX)$(DRIVER_DIR)/include \
             $(INCLUDE_PREFIX)$(DRIVER_DIR)/src/video/digital/graphics
endif # SPARC environment only!
