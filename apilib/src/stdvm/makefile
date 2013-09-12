######################
# MODULE : stdvm.lib #
######################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets and exported files
#--------------------------------------
ST20_TARGETS := stdvm.lib
ST20_EXPORTS := stdvm.h stdvm.lib
ST20_IMPORTS :=
ifneq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
else
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS))
endif
ST40_IMPORTS := $(ST20_IMPORTS)

HEADER_IMPORTS := stprm stos stcommon

ifneq (,$(findstring $(DVD_BACKEND),7710))
 HEADER_IMPORTS += $(DVD_TRANSPORT)lt
else
 HEADER_IMPORTS += $(DVD_TRANSPORT)
endif

ifneq (,$(findstring $(DVD_BACKEND),7100 7109))
 HEADER_IMPORTS += staudlx
endif

ifdef STDVM_ENABLE_NET
 HEADER_IMPORTS += stnet
endif

# Define tco files
#-----------------
ST20_OBJS   := handle.tco event.tco dvmfs.tco dvmindex.tco dvmtask.tco stdvm.tco
ifdef STDVM_ENABLE_CRYPT
 ST20_OBJS += dvmcrypt.tco
endif
ifeq "$(DVD_OS)" "LINUX"
 ST20_OBJS += linuxwrap.tco
endif
ifneq "$(DVD_OS)" "LINUX"
ST40_OBJS   := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
else
ST40_OBJS  	:= $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
endif

# INCLUDES	TODO: Remove
#-----------------------
ifeq "$(DVD_OS)" "LINUX"
 INCLUDES += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_ROOT)/dvdno-prj-7109linuxref/src/modules/utils)
endif

# SUBDIRS
#--------
SUBDIRS :=

# MP3 playback support has to be provided only to customers have license
#ifneq (,$(findstring $(DVD_BACKEND),5100))
# SUBDIRS += mp3pb
# CFLAGS += -DENABLE_MP3_PLAYBACK
#endif

# CFLAGS
#-------
ifeq "$(ARCHITECTURE)" "ST20"
 CFLAGS += -DDVD_EXCLUDE_OS20TO21_MACROS -D__OS_20TO21_MAP_H -Dtask_flags_no_min_stack_size=0
endif

#This flag is needed by STAUD for STDVM support of multiple clkrv handles
ifneq (,$(findstring $(DVD_BACKEND),5528 5100 5301 7710))
 CFLAGS += -DSTAUD_STDVM_SUPPORT
endif

ifdef STDVM_DEBUG
 CFLAGS += -DSTTBX_REPORT -DSTTBX_PRINT -DSTDVM_DEBUG
endif

ifdef STDVM_TRACE
 CFLAGS += -DENABLE_TRACE
endif

# Robust modes
ifdef STDVM_ENABLE_ROBUST_FILE_STORAGE
 CFLAGS += -DOPEN_CLOSE_FILE_ON_WRITE -DUSE_BACKUP_FILES -DSMALLER_STREAM_FILES

 ifdef STDVM_SUB_FILE_SIZE
  CFLAGS += -DSTDVM_SUB_FILE_SIZE=$(STDVM_SUB_FILE_SIZE)
 endif
endif

# CRYPT Support
ifdef STDVM_ENABLE_CRYPT
 CFLAGS += -DSTDVM_ENABLE_CRYPT
 ifneq (,$(findstring $(DVD_BACKEND),5100 7710 7100))
  DVD_CFLAGS += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CRYPT_PATH))
 endif
 ifneq (,$(findstring $(DVD_BACKEND),7109))
  DVD_CFLAGS += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(TKDMA_PATH))
 endif
endif

# NET Support,to be remove from stdvm
ifdef STDVM_ENABLE_NET
 CFLAGS += -DSTDVM_ENABLE_NET
 ifneq "$(DVD_OS)" "LINUX"
  CFLAGS += -DSTNET_CONTROLLED_BY_STDVM
 endif
endif

# OPSPLUS include path
ifdef OSPLUS_ROOT
 DVD_CFLAGS += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(OSPLUS_ROOT)/include)
endif

#Pick up standard targets
#------------------------
include $(DVD_MAKE)/defrules.mak


# Build lib and obj
#------------------
$($(ARCHITECTURE)_TARGETS): $($(ARCHITECTURE)_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)


# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning STDVM
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif



