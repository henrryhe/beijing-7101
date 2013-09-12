######################
# MODULE : mp3pb.lib #
######################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets and exported files
#--------------------------------------
ST20_TARGETS := mp3pb.lib
ST20_EXPORTS :=

ST20_IMPORTS :=

ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST40_IMPORTS := $(ST20_IMPORTS)

HEADER_IMPORTS :=

ifneq (,$(findstring $(DVD_BACKEND),5100 5301 7100 7109 7710))
 HEADER_IMPORTS += stfdma
endif


# Define tco files
#------------------
ST20_OBJS   := mp3pb.tco
ST40_OBJS   := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


# SUBDIRS
#------------------
SUBDIRS :=

# CFLAGS
#------------------
ifeq ($(ARCHITECTURE),ST20)
 CFLAGS += -DDVD_EXCLUDE_OS20TO21_MACROS -Dtask_flags_no_min_stack_size=0 -D__OS_20TO21_MAP_H
endif

#This flag is needed by STAUD for STDVM support of multiple clkrv handles
ifneq (,$(findstring $(DVD_BACKEND),5528 5100 5301 7710))
 CFLAGS += -DSTAUD_STDVM_SUPPORT
endif

ifdef MP3PB_DEBUG
 CFLAGS += -DSTTBX_REPORT -DSTTBX_PRINT
endif

ifdef MP3PB_USE_CPU_INJECTION
 CFLAGS += -DMP3PB_USE_CPU_INJECTION
endif

# OPSPLUS include path
DVD_CFLAGS += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(OSPLUS_ROOT)/include)


#Pick up standard targets
#------------------
include $(DVD_MAKE)/defrules.mak


# Build lib and obj
#------------------
$($(ARCHITECTURE)_TARGETS): $($(ARCHITECTURE)_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)


# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning MP3PB
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif



