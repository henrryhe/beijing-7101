# toolset.mak
# 
# Copyright (C) 1999-2004 STMicroelectronics
#
# This file sets various variables based on the toolset used to build
# the system. Strictly speaking we should use OBJ_SUFFIX, LIB_SUFFIX,
# etc in the definition of the toolset build variables and rules, but
# since we know which toolset we are defining them for, we don't; this
# makes it more readable.

# This file will set the following variables to drive make:
#
#	CC		The C Compiler
#	AR		The archiver/librarian tool
#	LINK		The linker
#	PROCESSOR	The target processor
#	INCLUDE_PREFIX 	Prefix to compiler for include paths (-I or -J).
#       LIBRARY_PREFIX  Prefix to compiler for library paths
#       CFLAGS          Flags passed to the C Compiler
#       LKFLAGS         The flags passed to the link phase

#       OBJ_SUFFIX      The suffix for a compiled C file
#       LIB_PREFIX      The prefix for a static library
#       LIB_SUFFIX      The suffix for a static library
#       EXE_SUFFIX      The suffix for an executable

################################################################
# For special mapping of platform to config file

CONFIG_MAP := :mb317a:mb317a_$(DVD_FRONTEND).cfg \
              :mb317b:mb317b_$(DVD_FRONTEND).cfg \
              :mediaref:mediaref_$(DVD_FRONTEND).cfg \
              :mb421:mb421_host \
              :mb426:mb426_host \
              :mb428:mb428_host


# Special mappings for platform/frontend combination
mb411_7109_mboard := mb411stb7109

MAPPED_MBOARD := $($(DVD_PLATFORM)_$(DVD_FRONTEND)_mboard)

ifneq "$(strip $(MAPPED_MBOARD))" ""
  USE_MBOARD := $(MAPPED_MBOARD)
else
  USE_MBOARD := $(DVD_PLATFORM)
endif


ifndef SPECIAL_CONFIG_FILE
  SPECIAL_CONFIG_FILE := $(strip \
                            $(foreach i,$(CONFIG_MAP),\
                                 $(if $(findstring :$(DVD_PLATFORM):,$(i)),\
                                        $(subst :$(DVD_PLATFORM):,,$(i)),)))
endif

ifndef OPTIONAL_CONFIG_FILE
  OPTIONAL_CONFIG_MAP := :mb361_5517:mb361_5517$(if $(UNIFIED_MEMORY),_um).cfg
  OPTIONAL_CONFIG_FILE := $(strip \
                              $(foreach i,$(OPTIONAL_CONFIG_MAP),\
                                   $(if $(findstring :$(DVD_PLATFORM)_$(DVD_FRONTEND):,$(i)),\
                                          $(subst :$(DVD_PLATFORM)_$(DVD_FRONTEND):,,$(i)),)))
endif

###############################################################
# ST20 (OS20)

ifeq "$(DVD_TOOLSET)" "ST20"

# ST20 toolset

ifeq "$(DVD_CODETEST)" "TRUE"
CC	       = ctcc
else
CC             = st20cc
endif

AR	       = st20libr
LINK           = st20cc
RUN            = st20run
ifneq "$(wildcard $(ST20ROOT)/bin/st20dev.exe)" ""
  DB           = st20dev -g
else
  DB           = st20run -g
endif

ifndef PROCESSOR
  ifeq "$(findstring $(DVD_FRONTEND),7710 5105 5700 5188 5107)" "$(DVD_FRONTEND)"
    ifndef GCC_CHECK
      ifndef GCC_CHECK_SA
    PROCESSOR  = -c1
    PROC_CFLAGS += -finl-timeslice
      endif
    endif
    PROC_DEFINE := C1
    # Not a great idea to put this here, but stsys is used by so many things...
    CFLAGS     += -DSTSYS_NO_PRAGMA
  else
    ifndef GCC_CHECK
      ifndef GCC_CHECK_SA
    PROCESSOR  = -c2
      endif
    endif
    PROC_DEFINE := C2
  endif
endif

INCLUDE_PREFIX = -I
LIBRARY_PREFIX = -L

OBJ_SUFFIX     = .tco
LIB_PREFIX     =
LIB_SUFFIX     = .lib
EXE_SUFFIX     = .lku
IDB_SUFFIX     = .idb

# Set-up the C Flags. These may modified if required.
# Note that if using '-g' (or DEBUG) it may be advisable to use '-O0'
# (see Toolset User Manual for further information).

PROC_CFLAGS += $(PROCESSOR) -c

# Build for debugging if required
ifdef DEBUG
  ifndef OPTLEVEL
    OPTLEVEL := 0
  endif
  PROC_CFLAGS += -g
else
  ifndef OPTLEVEL
    OPTLEVEL := 2
  endif
endif
PROC_CFLAGS += -O$(OPTLEVEL)
CFLAGS += -DPROCESSOR_$(PROC_DEFINE) $(DVD_CFLAGS) $(PROC_CFLAGS)

## The following is to get around the problem introduced in
## os20 v2.7.0. (R1.8.1), of separated libraries
## Problem is not present when using -runtime os20

ifeq "$(DVD_OS20)" "RUNTIME"
  OS20LIBS = -runtime os20
else
  OS20LIB = os20.lib

  ifneq "$(wildcard $(ST20ROOT)/lib/os20ilc1.lib)" ""
    ## After 1.8.0 these were made into separate libraries
    ## So we add then here.
    ifndef OS20_ILC_LIB
      OS20_ILC_LIB := os20ilc1.lib
    endif
    ifndef OS20_IC_LIB
      ifeq "$(findstring $(DVD_FRONTEND),7710 5105 5700 5188 5107)" "$(DVD_FRONTEND)"
        OS20_IC_LIB := os20intc2.lib
      else
        OS20_IC_LIB := os20intc1.lib
      endif
    endif

    ## Special cases for interrupt level controllers
    ## First check that we have not set the ILC library in the environment
    ifeq "$(origin OS20_ILC_LIB)" "file"
      ifeq "$(DVD_FRONTEND)" "5518"
        # Documentation says this should be ilc2, but that does not work
        OS20_ILC_LIB := os20ilc1.lib
      endif
      ifeq "$(findstring $(DVD_FRONTEND),5514 5516 5517 5528 5100 7710 5105 5700 5188 5107)" "$(DVD_FRONTEND)"
        OS20_ILC_LIB := os20ilc3.lib
      endif
    endif
  endif

  OS20LIBS = $(strip $(OS20LIB) $(OS20_IC_LIB) $(OS20_ILC_LIB))
  ifdef USE_DEBUG_KERNEL
    OS20LIBS := $(addprefix debug/,$(OS20LIBS))
  endif
endif


# By default OSLIBS is set to OS20LIBS - can be overridden.
OSLIBS = $(OS20LIBS)

LINT_COMMAND = $(LINT) -zero '-passes(2)' -i$(DVD_MAKE) std.lnt '+os($(LINT_OUTPUT))'\
      $(subst -D$(SPACE),-D,$(filter-out -cpp,$(filter-out $(PROC_CFLAGS),$(DEFAULT_CFLAGS))))\
      $($(basename $@)_CFLAGS) $($(basename $@)_ST20_CFLAGS)\
      -i$(ST20ROOT)/include $(INCLUDES) $(BASE_INCLUDES) $<

define COMPILE_C
$(if $(LINT_OUTPUT),echo $(LINT_COMMAND) >> $(LINT_OUTPUT)
$(LINT_COMMAND))
$(CC) $< $(DEFAULT_CFLAGS) $($(basename $@)_CFLAGS) $($(basename $@)_ST20_CFLAGS)\
      $(INCLUDES) $(BASE_INCLUDES) -o $@
endef

ifeq ($(DVD_HOST),unix)

# A unix build tool has no problem
define BUILD_LIBRARY
$(AR) $(LIBRARIES) $(filter %.tco %.lib,$^) -o $@
endef

else

# PC build has a problem with importing a group of libraries into a library, so
# we use the libraries from their original location.
define BUILD_LIBRARY
$(AR) $(filter %.tco,$^) \
      $(foreach LIB,$(filter %.lib,$^),\
                $(if $(wildcard $(addsuffix /$(LIB),$(LIBRARY_PATH))),\
                     $(firstword $(wildcard $(addsuffix /$(LIB),$(LIBRARY_PATH)))),\
                     $(error Failed to find library $(LIB) on path ($(LIBRARY_PATH))))) \
      -o $@
endef

endif   # DVD_HOST==unix

# If SPECIAL_CONFIG_FILE is set, use that otherwise check for an optional
# config file. If that does not exist, use the default one, based on the
# platform.
CONFIG_FILE = $(if $(SPECIAL_CONFIG_FILE),\
                      $(SPECIAL_CONFIG_FILE),\
                      $(if $(strip $(notdir $(wildcard $(addsuffix /$(OPTIONAL_CONFIG_FILE),\
                                                                    $(CFG_PATH)\
                                                        )\
                                             )\
                                    )\
                            ),\
                              $(OPTIONAL_CONFIG_FILE),\
                              $(DVD_PLATFORM)$(if $(UNIFIED_MEMORY),_um).cfg\
                       )\
               )

ifdef GENERATE_MAP
  EXTRA_LINK_FLAGS = -M $(basename $@).map
endif

define LINK_EXECUTABLE
$(if $($(basename $@)_INIT),,$(if $(DVD_LINK_INIT),,$(error entry point for $@ must be specified in $(basename $@)_INIT variable)))
$(CC) $(LIBRARIES) -T $(CONFIG_FILE) \
      $(DEFAULT_LKFLAGS) $(EXTRA_LINK_FLAGS) $($(basename $@)_LKFLAGS) $($(basename $@)_ST20_LKFLAGS) \
      $^ $(OSLIBS) -p $(if $($(basename $@)_INIT),$($(basename $@)_INIT),$(DVD_LINK_INIT)) -o $@
endef

define RUN_OBJECT
$(RUN) $(addprefix -l ,$(CFG_PATH)) -i $(CONFIG_FILE) -t $(TARGET) $(subst _RUN,,$@) \
      $(ST20_RUNARGS) $(DVD_RUNARGS)
endef

define DEBUG_RUN_OBJECT
$(DB) $(addprefix -l ,$(CFG_PATH)) -i $(CONFIG_FILE) -t $(TARGET) $(subst _DEBUG_RUN,,$@) \
      $(ST20_RUNARGS) $(DVD_RUNARGS)
endef

# Default rule to build a .tco object file
%.tco: %.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)


# Default rule to build a library file from a single .tco file
%.lib: %.tco
	@$(ECHO) Building library $@
	$(BUILD_LIBRARY)

endif   # ST20 toolset variables and rules

###############################################################
# SPARC

ifeq "$(DVD_TOOLSET)" "SPARC"

# Sparc toolset
CC             = sparc-elf-gcc
AR	       = munge sparc-elf-ar
LINK           = sparc-elf-ld -L$(CYGNUS_ROOT)/lib/gcc-lib/sparc-elf/2.9-gnupro-99r1p1/soft/libgcc.a
PROCESSOR      =
INCLUDE_PREFIX = -I
LIBRARY_PREFIX = -L

OBJ_SUFFIX     = .tco
LIB_PREFIX     =
LIB_SUFFIX     = .lib
EXE_SUFFIX     = .lku
IDB_SUFFIX     = .idb

# Set-up the C Flags. These may modified if required.
# Note that if using '-g' (or DEBUG) it may be advisable to use '-O0'
# (see Toolset User Manual for further information).

CFLAGS	+= $(DVD_CFLAGS) $(PROCESSOR) -c

# Build for debugging if required
ifdef DEBUG
  ifndef OPTLEVEL
    OPTLEVEL := 0
  endif
  CFLAGS += -g
else
  ifndef OPTLEVEL
    OPTLEVEL := 2
  endif
endif
CFLAGS += -O$(OPTLEVEL)

## The following is to get around the problem introduced in
## os20 v2.7.0. (R1.8.1), of separated libraries
## Problem is not present when using -runtime os20

ifeq "$(DVD_OS20)" "RUNTIME"
  OS20LIBS = -runtime os20
else
  OS20LIB = os20.lib

  ifneq "$(wildcard $(SPARCROOT)/lib/os20ilc1.lib)" ""
    ## After 1.8.0 these were made into separate libraries
    ## So we add then here.
    ifndef OS20_ILC_LIB
      OS20_ILC_LIB := os20ilc1.lib
    endif
    ifndef OS20_IC_LIB
      ifeq "$(findstring $(DVD_FRONTEND),7710 5105 5700 5188 5107)" "$(DVD_FRONTEND)"
        OS20_IC_LIB := os20intc2.lib
      else
        OS20_IC_LIB := os20intc1.lib
      endif
    endif

    ## Special cases for interrupt level controllers
    ## First check that we have not set the ILC library in the environment
    ifeq "$(origin OS20_ILC_LIB)" "file"
      ifeq "$(DVD_FRONTEND)" "5518"
        # Documentation says this should be ilc2, but that does not work
        OS20_ILC_LIB := os20ilc1.lib
      endif
      ifeq "$(findstring $(DVD_FRONTEND),5514 5516 5517 5528 5100 7710 5105 5188 5107)" "$(DVD_FRONTEND)"
        OS20_ILC_LIB := os20ilc3.lib
      endif
    endif
  endif

  OS20LIBS = $(strip $(OS20LIB) $(OS20_IC_LIB) $(OS20_ILC_LIB))
  ifdef USE_DEBUG_KERNEL
    OS20LIBS := $(addprefix debug/,$(OS20LIBS))
  endif
endif


# By default OSLIBS is set to OS20LIBS - can be overridden.
OSLIBS = $(OS20LIBS)

define COMPILE_C
$(CC) $< $(DEFAULT_CFLAGS) $($(basename $@)_CFLAGS) $($(basename $@)_ST20_CFLAGS)\
      $(INCLUDES) $(BASE_INCLUDES) -o $@
endef

ifeq ($(DVD_HOST),unix)

# A unix build tool has no problem
define BUILD_LIBRARY
$(AR) $(filter %.tco %.lib,$^) -o $@
endef

else

# PC build has a problem with importing a group of libraries into a library, so
# we use the libraries from their original location.
define BUILD_LIBRARY
$(AR) $(filter %.tco,$^) \
      $(foreach LIB,$(filter %.lib,$^),\
                $(if $(wildcard $(addsuffix /$(LIB),$(LIBRARY_PATH))),\
                     $(firstword $(wildcard $(addsuffix /$(LIB),$(LIBRARY_PATH)))),\
                     $(error Failed to find library $(LIB) on path ($(LIBRARY_PATH))))) \
      -o $@
endef

endif   # DVD_HOST==unix

CONFIG_FILE = $(if $(SPECIAL_CONFIG_FILE),$(SPECIAL_CONFIG_FILE),\
                                          $(DVD_PLATFORM)$(if $(UNIFIED_MEMORY),_um).cfg)
ifdef GENERATE_MAP
  EXTRA_LINK_FLAGS = -M $(basename $@).map
endif

define LINK_EXECUTABLE
$(if $($(basename $@)_INIT),,$(if $(DVD_LINK_INIT),,$(error entry point for $@ must be specified in $(basename $@)_INIT variable)))
$(CC) $(LIBRARIES) -T $(CONFIG_FILE) \
      $(DEFAULT_LKFLAGS) $(EXTRA_LINK_FLAGS) $($(basename $@)_LKFLAGS) $($(basename $@)_SPARC_LKFLAGS) \
      $^ $(OSLIBS) -p $(if $($(basename $@)_INIT),$($(basename $@)_INIT),$(DVD_LINK_INIT)) -o $@
endef

define RUN_OBJECT
$(RUN) $(addprefix -l ,$(CFG_PATH)) -i $(CONFIG_FILE) -t $(TARGET) $(subst _RUN,,$@) \
      $(SPARC_RUNARGS) $(DVD_RUNARGS)
endef

define DEBUG_RUN_OBJECT
$(DB) $(addprefix -l ,$(CFG_PATH)) -i $(CONFIG_FILE) -t $(TARGET) $(subst _DEBUG_RUN,,$@) \
      $(SPARC_RUNARGS) $(DVD_RUNARGS)
endef

# Default rule to build a .tco object file
%.tco: %.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)


# Default rule to build a library file from a single .tco file
%.lib: %.tco
	@$(ECHO) Building library $@
	$(BUILD_LIBRARY)

endif   # SPARC toolset variables and rules

###############################################################
# ST40 (OS21)

ifeq "$(DVD_TOOLSET)" "ST40"

CC		= sh4gcc
## STFAE
CC_PLUS	        = sh4g++
AR		= sh4ar
LD		= sh4gcc
RUN		= sh4gdb
DB		= sh4gdb -w
LINK            = sh4g++
INCLUDE_PREFIX  = -I
LIBRARY_PREFIX  = -L

OBJ_SUFFIX      = .o
LIB_PREFIX      = lib
LIB_SUFFIX      = .a
EXE_SUFFIX      = .exe

# added for TLM native: patch the st40 toolchain
ifdef NATIVE_COMPIL 
CC      = gcc
AR      = ar
LD      = gcc
LINK    = gcc
EXE_SUFFIX      = .so
OS_INCLUDES     = -I${THD_OS21}/include/
DVD_CFLAGS      += -DNATIVE_CORE -D_GNU_SOURCE -D__SH4__
endif

# Default is to run from region P1 (cached)
ifndef OS21_REGION
OS21_REGION     := p1
endif

CFLAGS	+= $(DVD_CFLAGS)
CFLAGS += -Wall ## for generating all warnings

# Build for debugging if required
ifeq "$(DEBUG)" "1"
  ifndef OPTLEVEL
    OPTLEVEL := 0
  endif
  CFLAGS += -g
else
  ifndef OPTLEVEL
    OPTLEVEL := 2
  endif
endif

ifndef OS21_RUNTIME_LIB
  ifdef USE_DEBUG_KERNEL
    OS21_RUNTIME_LIB := os21_d
  else
    OS21_RUNTIME_LIB := os21
  endif
endif

ifdef GENERATE_MAP
  EXTRA_LINK_FLAGS = -Xlinker -Map=$(basename $@).map
endif

CFLAGS += -O$(OPTLEVEL) -fno-strict-aliasing -DOS21_RUNTIME -DST40_OS21
ifndef NATIVE_COMPIL
CFLAGS += -mruntime=$(OS21_RUNTIME_LIB)
endif

LDOPTS  = -mboard=$(USE_MBOARD)$(if $($(basename $@)_REGION),$($(basename $@)_REGION),$(OS21_REGION)) \
          -mruntime=$(OS21_RUNTIME_LIB) -lm

ifdef DEBUG
   LDOPTS += -g
endif

AROPTS	= -r

LINT_COMMAND = $(LINT) -zero '-passes(2)' -i$(DVD_MAKE) std.lnt '+os($(LINT_OUTPUT))'\
      $(subst -D$(SPACE),-D,$(filter-out -cpp -O2 -O0 -fno-strict-aliasing -mruntime=$(OS21_RUNTIME_LIB),$(filter-out $(PROC_CFLAGS),$(DEFAULT_CFLAGS))))\
      -D__IEEE_LITTLE_ENDIAN\
      $($(basename $@)_CFLAGS) $($(basename $@)_ST40_CFLAGS)\
      -i$(ST40ROOT)/sh-superh-elf/include -i$(wildcard $(ST40ROOT)/lib/gcc/sh-superh-elf/*/include) $(INCLUDES) $(BASE_INCLUDES) $<

define COMPILE_C
$(if $(LINT_OUTPUT),echo $(LINT_COMMAND) >> $(LINT_OUTPUT)
$(LINT_COMMAND))
$(CC) $(DEFAULT_CFLAGS) $($(basename $@)_CFLAGS) $($(basename $@)_ST40_CFLAGS) \
      $(INCLUDES) $(BASE_INCLUDES) $(OS_INCLUDES) -o $@ -c $<
endef

# The build library function is complex because the librarian cannot include
# a library in a library. We have to extract the libraries into a temporary
# directory and then insert those files into the library.

define FIRST_LIB_IN_PATH
$(if $(wildcard $(addsuffix /$(LIB),$(LIBRARY_PATH))),\
     $(firstword $(wildcard $(addsuffix /$(LIB),$(LIBRARY_PATH)))),\
     $(error Failed to find library $(LIB) on path ($(LIBRARY_PATH))))
endef

#			$(AR) -x $(FIRST_LIB_IN_PATH)
#			$(AR) -q $@ $(shell $(AR) -t $(FIRST_LIB_IN_PATH))
#			$(RM) $(shell $(AR) -t $(FIRST_LIB_IN_PATH))

#
# Add object files and sub-libraries to library
#
# The adding of library files as opposed to object files is not supported
# using command line parameters. Library files can be added to library files
# using the 'ar' scripting language. A simple line of script is piped to
# the 'ar' program for each library file that needs to be added to another
# library file.  First we remove the existing library, then create the
# new library with the scripting language.  Then, we add all objects to the
# library.  Finally, using the scripting language, we add all libraries to the
# library.
#
#
#	@$(AR) -d $@
define BUILD_LIBRARY
$(if $(wildcard $@),@$(RM) $@)
$(if $(wildcard ar.mac),@$(RM) ar.mac)
$(if $(filter lib%.a,$^),
@echo CREATE $@ > ar.mac
@echo SAVE >> ar.mac
@echo END >> ar.mac
@$(AR) -M <ar.mac
)
$(if $(filter %.o,$^),@$(AR) -q $@ $(filter %.o,$^))
$(if $(filter lib%.a,$^),@echo OPEN $@ > ar.mac
$(foreach LIB,$(filter lib%.a,$^),
@echo ADDLIB $(FIRST_LIB_IN_PATH) >> ar.mac
)
@echo SAVE >> ar.mac
@echo END >> ar.mac
@$(AR) -M <ar.mac
@$(RM) ar.mac
)
$(AR) -s $@
endef

ifdef NATIVE_COMPIL
# stop the link at shared-lib stage
define LINK_EXECUTABLE
$(LINK) $(LIBRARIES) \
      $(DEFAULT_LKFLAGS) $(EXTRA_LINK_FLAGS) $($(basename $@)_LKFLAGS) $($(basename $@)_ST40_LKFLAGS) \
      -L$(TLM_TMP)/OS21/lib/native \
      $(filter %.o,$^) \
	   -shared -Wl,--start-group -L${THD_OS21}/lib/native $(addprefix -l,$(patsubst lib%.a,%,$(filter lib%.a,$(notdir $^)))) -los21 -l$(DVD_PLATFORM) -Wl,--end-group \
	  $(LDOPTS) -o $(basename $@).so
endef
else
define LINK_EXECUTABLE
$(LINK) $(LIBRARIES) \
      $(DEFAULT_LKFLAGS) $(EXTRA_LINK_FLAGS) $($(basename $@)_LKFLAGS) $($(basename $@)_ST40_LKFLAGS) \
      $(filter %.o,$^) \
      -Wl,--start-group $(addprefix -l,$(patsubst lib%.a,%,$(filter lib%.a,$(notdir $^)))) -Wl,--end-group \
      $(LDOPTS) -o $(basename $@).exe
endef
endif

CONFIG_FILE = $(if $(SPECIAL_CONFIG_FILE),$(SPECIAL_CONFIG_FILE),$(DVD_PLATFORM))

define RUN_OBJECT
-@$(RM) runit.cmd > $(NULL) 2>&1
@$(ECHO) $(CONFIG_FILE) $(TARGET) > runit.cmd
@$(ECHO) load >> runit.cmd
@$(ECHO) console off >> runit.cmd
@$(ECHO) continue >> runit.cmd
$(RUN) $(subst _RUN,,$@) -x runit.cmd $(ST40_RUNARGS) $(DVD_RUNARGS)
-@$(RM) runit.cmd > $(NULL) 2>&1
endef


define DEBUG_RUN_OBJECT
-@$(RM) runit.cmd > $(NULL) 2>&1
@$(ECHO) $(CONFIG_FILE) $(TARGET) >> runit.cmd
@$(ECHO) symbol-file $(subst _DEBUG_RUN,,$@) >> runit.cmd
@$(ECHO) load $(subst _DEBUG_RUN,,$@) >> runit.cmd
@$(ECHO) console off >> runit.cmd
@$(ECHO) break main >> runit.cmd
@$(ECHO) continue >> runit.cmd
$(DB) -x runit.cmd $(ST40_RUNARGS) $(DVD_RUNARGS)
-@$(RM) runit.cmd > $(NULL) 2>&1
endef


# Default rule to build a .o object file
%.o: %.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)


# Default rule to build a library file from a single .o file
lib%.a: %.o
	@$(ECHO) Building library $@
	$(BUILD_LIBRARY)

endif   # ST40_OS21 toolset variables and rules

###############################################################
# ST200 (OS21)

ifeq "$(DVD_TOOLSET)" "ST200"

CC		= st200cc
AR		= st200ar
LD		= st200ld
RUN		= st200gdb -nw
DB		= st200gdb
LINK            = st200c++
INCLUDE_PREFIX  = -I
LIBRARY_PREFIX  = -L

OBJ_SUFFIX      = .o
LIB_PREFIX      = lib
LIB_SUFFIX      = .a
EXE_SUFFIX      = .exe

CFLAGS	+= $(DVD_CFLAGS)
CFLAGS	+= -Wall

# Build for debugging if required
ifdef DEBUG
  ifndef OPTLEVEL
    OPTLEVEL := 0
  endif
  CFLAGS += -g
else
  ifndef OPTLEVEL
    OPTLEVEL := 2
  endif
endif

ifndef OS21_RUNTIME_LIB
  ifdef USE_DEBUG_KERNEL
    OS21_RUNTIME_LIB := os21_d
  else
    OS21_RUNTIME_LIB := os21
  endif
endif

ifdef GENERATE_MAP
  EXTRA_LINK_FLAGS = -Wl,--M $(basename $@).map
endif

CFLAGS += -O$(OPTLEVEL) -fno-strict-aliasing -mruntime=$(OS21_RUNTIME_LIB) -DOS21_RUNTIME -DST200_OS21

CONFIG_FILE = $(if $(SPECIAL_CONFIG_FILE),$(SPECIAL_CONFIG_FILE),$(DVD_PLATFORM))

LDOPTS  = -mboard=$(CONFIG_FILE) -mruntime=$(OS21_RUNTIME_LIB) -lm

ifdef DEBUG
   LDOPTS += -g
endif

AROPTS	= -r

LINT_COMMAND = $(LINT) -zero '-passes(2)' -i$(DVD_MAKE) std.lnt '+os($(LINT_OUTPUT))'\
      $(subst -D$(SPACE),-D,$(filter-out -cpp -O2 -O0 -fno-strict-aliasing -mruntime=$(OS21_RUNTIME_LIB),$(filter-out $(PROC_CFLAGS),$(DEFAULT_CFLAGS))))\
      -D__IEEE_LITTLE_ENDIAN\
      $($(basename $@)_CFLAGS) $($(basename $@)_ST200_CFLAGS)\
      -i$(ST200ROOT)/include $(INCLUDES) $(BASE_INCLUDES) $<

define COMPILE_C
$(if $(LINT_OUTPUT),echo $(LINT_COMMAND) >> $(LINT_OUTPUT)
$(LINT_COMMAND))
$(CC) $(DEFAULT_CFLAGS) -mcore=st231 $($(basename $@)_CFLAGS) $($(basename $@)_ST200_CFLAGS) \
      $(INCLUDES) $(BASE_INCLUDES) $(OS_INCLUDES) -o $@ -c $<
endef

# The build library function is complex because the librarian cannot include
# a library in a library. We have to extract the libraries into a temporary
# directory and then insert those files into the library.

define FIRST_LIB_IN_PATH
$(if $(wildcard $(addsuffix /$(IMP),$(LIBRARY_PATH))),\
     $(firstword $(wildcard $(addsuffix /$(IMP),$(LIBRARY_PATH)))),\
     $(error Failed to find library $(IMP) on path ($(LIBRARY_PATH))))
endef

#			$(AR) -x $(FIRST_LIB_IN_PATH)
#			$(AR) -q $@ $(shell $(AR) -t $(FIRST_LIB_IN_PATH))
#			$(RM) $(shell $(AR) -t $(FIRST_LIB_IN_PATH))

#
# Add object files and sub-libraries to library
#
# The adding of library files as opposed to object files is not supported
# using command line parameters. Library files can be added to library files
# using the 'ar' scripting language. A simple line of script is piped to
# the 'ar' program for each library file that needs to be added to another
# library file.  First we remove the existing library, then create the
# new library with the scripting language.  Then, we add all objects to the
# library.  Finally, using the scripting language, we add all libraries to the
# library.
#
#
#	@$(AR) -d $@
define BUILD_LIBRARY
$(if $(wildcard $@),@$(RM) $@)
$(if $(wildcard ar.mac),@$(RM) ar.mac)
$(if $(filter lib%.a,$(notdir $^)),
@echo CREATE $@ > ar.mac
$(foreach IMP,$^,
  $(if $(filter lib%.a,$(notdir $(IMP))),
    $(if $(findstring ./,$(dir $(IMP))),
      @echo ADDLIB $(FIRST_LIB_IN_PATH) >> ar.mac
     ,@echo ADDLIB $(IMP) >> ar.mac
    )
  )
)
@echo SAVE >> ar.mac
@echo END >> ar.mac
@$(AR) -M <ar.mac
@$(RM) ar.mac
)
$(if $(filter %.o,$^),@$(AR) -q $@ $(filter %.o,$^))
$(AR) -s $@
endef

ifndef ST200_MSOC
  ifeq "$(findstring $(DVD_FRONTEND),8010)" "$(DVD_FRONTEND)"
    ST200_MSOC := stm$(DVD_FRONTEND)
  else
    ifeq "$(findstring $(DVD_FRONTEND),5525)" "$(DVD_FRONTEND)"
      ST200_MSOC := stx$(DVD_FRONTEND)
    else
      ifeq "$(findstring $(DVD_FRONTEND),5301)" "$(DVD_FRONTEND)"
        ST200_MSOC := sti5300
      else
        ST200_MSOC := sti$(DVD_FRONTEND)
      endif
    endif
  endif
endif

ifndef ST200_MCORE
  ST200_MCORE := st231
endif

DEFAULT_LKFLAGS += -O$(OPTLEVEL)

define LINK_EXECUTABLE
$(LINK) $(LIBRARIES) \
      $(DEFAULT_LKFLAGS) $(EXTRA_LINK_FLAGS) $($(basename $@)_LKFLAGS) $($(basename $@)_ST200_LKFLAGS) \
      $(filter %.o,$^) \
      -Wl,--start-group $(addprefix -l,$(patsubst lib%.a,%,$(filter lib%.a,$(notdir $^)))) -Wl,--end-group \
      $(LDOPTS) -msoc=$(ST200_MSOC) -mcore=$(ST200_MCORE) -o $(basename $@).exe
endef

define RUN_OBJECT
-@$(RM) runit.cmd 2>1 > $(NULL)
@$(ECHO) target gdi -t $(TARGET) > runit.cmd
@$(ECHO) load >> runit.cmd
@$(ECHO) run >> runit.cmd
$(RUN) --directory=. --command=runit.cmd --args $(subst _RUN,,$@) $(ST200_RUNARGS) $(DVD_RUNARGS)
-@$(RM) runit.cmd 2>1 > $(NULL)
endef


define DEBUG_RUN_OBJECT
$(DB) $(subst _DEBUG_RUN,,$@)
endef

# Default rule to build a .o object file
%.o: %.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)


# Default rule to build a library file from a single .o file
lib%.a: %.o
	@$(ECHO) Building library $@
	$(BUILD_LIBRARY)

endif   # ST200_OS21 toolset variables and rules


###############################################################
# LINUX (LINUX)

ifeq "$(DVD_TOOLSET)" "LINUX"

###################
# Cross Compilation
###################

ifeq "$(ARCHITECTURE)" "ST40"
  CROSS_COMPILE   := sh4-linux-
  ARCH            := sh
  EMULATION_MODE  := shlelf_linux
else
  ifeq "$(ARCHITECTURE)" "ST200"
    CROSS_COMPILE   := st200
    ARCH            := $(error not defined)
    EMULATION_MODE  := $(error not correctly defined for st200)
  endif
endif

####################
# original Linux kernel variables

#AS		= $(CROSS_COMPILE)as
#CPP		= $(CC) -E
#NM		= $(CROSS_COMPILE)nm
#STRIP		= $(CROSS_COMPILE)strip
#OBJCOPY		= $(CROSS_COMPILE)objcopy
#OBJDUMP		= $(CROSS_COMPILE)objdump
#MODFLAGS	= -DMODULE
#PERL		= perl
#AWK		= awk

CC              = $(CROSS_COMPILE)gcc
AR              = $(CROSS_COMPILE)ar
LD              = $(CROSS_COMPILE)ld
LINK            = $(CROSS_COMPILE)gcc
INCLUDE_PREFIX  = -I
LIBRARY_PREFIX  = -L

OBJ_SUFFIX      = .o
LIB_PREFIX      = lib
LIB_SUFFIX      = .a
MOD_PREFIX      =
MOD_SUFFIX      = .ko
EXE_SUFFIX      =

CFLAGS	+= $(DVD_CFLAGS)

# Build for debugging if required
ifdef DEBUG
  ifndef OPTLEVEL
    OPTLEVEL := 0
  endif
  CFLAGS += -g

  ifneq "$(KERNELRELEASE)" ""
    CFLAGS += -fno-omit-frame-pointer
  endif
else
  ifndef OPTLEVEL
    OPTLEVEL := 2
  endif
endif

# These three LINUX defines must be consolidated
CFLAGS += -O$(OPTLEVEL) -DST_OSLINUX
CFLAGS += -Wall ## for generating all warnings

ifdef MODULE
# This define is obsolete and should be deleted
# once no more used by any makefiles
# This is replaced with KERNEL_MODE
LDOPTS	= -r
else

ifdef KERNEL_MODE
LDOPTS	= -r
else
LDOPTS	= -m$(EMULATION_MODE) -lm -lpthread
endif

endif
ifdef DEBUG
  LDOPTS	+= -g
endif

AROPTS	= -cr

define COMPILE_C
$(CC) $(EXTRA_CFLAGS) $(DEFAULT_CFLAGS) $($(basename $@)_CFLAGS) $($(basename $@)_$(ARCHITECTURE)_CFLAGS) \
      $(INCLUDES) $(BASE_INCLUDES) $(OS_INCLUDES) -o $@ -c $<
endef

# The build library function is complex because the librarian cannot include
# a library in a library. We have to extract the libraries into a temporary
# directory and then insert those files into the library.

define FIRST_LIB_IN_PATH
$(if $(wildcard $(addsuffix /$(IMP),$(LIBRARY_PATH))),\
     $(firstword $(wildcard $(addsuffix /$(IMP),$(LIBRARY_PATH)))),\
     $(error Failed to find library $(IMP) on path ($(LIBRARY_PATH))))
endef

define BUILD_LIBRARY
$(if $(wildcard $@),@$(RM) $@)
$(if $(wildcard ar.mac),@$(RM) ar.mac)
$(if $(filter lib%.a,$(notdir $^)),
@echo CREATE $@ > ar.mac
$(foreach IMP,$^,
  $(if $(filter lib%.a,$(notdir $(IMP))),
   $(if $(LINUX_$(IMP)_NOLIB),,
    $(if $(findstring ./,$(dir $(IMP))),
      @echo ADDLIB $(FIRST_LIB_IN_PATH) >> ar.mac
     ,@echo ADDLIB $(IMP) >> ar.mac
     )
    )
   )
)
@echo SAVE >> ar.mac
@echo END >> ar.mac
@$(AR) -M <ar.mac
@$(RM) ar.mac
)
$(if $(filter %.o,$^),@$(AR) -q $@ $(filter %.o,$^))
$(AR) -s $@
endef

define LINK_EXECUTABLE
$(LINK) $(LIBRARIES) \
      $(DEFAULT_LKFLAGS) $($(basename $@)_LKFLAGS) $($(basename $@)_$(ARCHITECTURE)_LKFLAGS) \
      $(filter %.o,$^) \
      $(foreach LIB,$(filter lib%.a,$(notdir $^)),$(if $(LINUX_$(LIB)_NOLINK),,$(addprefix -l,$(patsubst lib%.a,%,$(LIB))))) \
          $(LDOPTS) -o $@
endef

# Don't include these rules when building for a KBUILD
ifeq "$(KERNELRELEASE)" ""

# Default rule to build a .o object file
%.o: %.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)

# Default rule to build a library file from a single .o file
%.a: %.o
	@$(ECHO) Building library $@
	$(BUILD_LIBRARY)

endif

endif   # LINUX toolset variables and rules


###############################################################
# A error catch - if we have not recognised the DVD_TOOLSET
ifeq "$(origin OBJ_SUFFIX)" "undefined"
  $(error invalid DVD_TOOLSET ($(DVD_TOOLSET)))
endif


# For all toolsets, we define two required macros
define LIBNAME_FROM_AT
$(strip $(patsubst $(LIB_PREFIX)%$(LIB_SUFFIX),%,$@))
endef

define LIBNAME_FROM_AT_F
$(strip $(patsubst $(LIB_PREFIX)%$(LIB_SUFFIX),%,$(@F)))
endef

# Define some extra macros to convert naming convention for ST20/ST40
# Care must be taken to keep these up to date with any changes in naming
# convention.
ST20OBJ_TO_ST40OBJ = $(patsubst %.tco,%.o,$(1))
ST20LIB_TO_ST40LIB = $(patsubst %.lib,lib%.a,$(1))
ST20EXE_TO_ST40EXE = $(patsubst %.lku,%.exe,$(1))

ST40OBJ_TO_ST20OBJ = $(patsubst %.o,%.tco,$(1))
ST40LIB_TO_ST20LIB = $(patsubst lib%.a,%.lib,$(1))
ST40EXE_TO_ST20EXE = $(patsubst %.exe,%.lku,$(1))

ST20OBJ_TO_LINUXOBJ = $(patsubst %.tco,%.o,$(1))
ST20LIB_TO_LINUXLIB = $(patsubst %.lib,lib%.a,$(1))
ST20EXE_TO_LINUXEXE = $(patsubst %.lku,%,$(1))

LINUXOBJ_TO_ST20OBJ = $(patsubst %.o,%.tco,$(1))
LINUXLIB_TO_ST20LIB = $(patsubst lib%.a,%.lib,$(1))
LINUXEXE_TO_ST20EXE = $(patsubst %,%.lku,$(1))

# Define some extra macros to convert naming convention for ST20/ST200
# Care must be taken to keep these up to date with any changes in naming
# convention.
ST20OBJ_TO_ST200OBJ = $(patsubst %.tco,%.o,$(1))
ST20LIB_TO_ST200LIB = $(patsubst %.lib,lib%.a,$(1))
ST20EXE_TO_ST200EXE = $(patsubst %.lku,%.exe,$(1))

ST200OBJ_TO_ST20OBJ = $(patsubst %.o,%.tco,$(1))
ST200LIB_TO_ST20LIB = $(patsubst lib%.a,%.lib,$(1))
ST200EXE_TO_ST20EXE = $(patsubst %.exe,%.lku,$(1))


# EOF

