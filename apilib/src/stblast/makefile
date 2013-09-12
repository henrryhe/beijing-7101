DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
IMPORTS := stos stpio
HEADER_IMPORTS := include

ifeq "$(ARCHITECTURE)" "LINUX"
HEADER_IMPORTS += stblast/linux
endif

EXPORTS := stblast.h $(LIB_PREFIX)stblast$(LIB_SUFFIX)
TARGETS := $(LIB_PREFIX)stblast$(LIB_SUFFIX)

ifndef ST40_TARGETS
  ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
  ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
  ST40_IMPORTS = $(IMPORTS)
endif

ifndef ST200_TARGETS
  ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
  ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
  ST200_IMPORTS = $(IMPORTS)
endif

ifndef LINUX_TARGETS
  LINUX_TARGETS = $(TARGETS)
endif

ifndef LINUX_EXPORTS
  LINUX_EXPORTS = $(EXPORTS)
endif

ifndef LINUX_IMPORTS
  LINUX_IMPORTS = $(IMPORTS)
endif

# Local objects which comprise this component
OBJS:= stblast$(OBJ_SUFFIX) blastint$(OBJ_SUFFIX) timer$(OBJ_SUFFIX) \
       space$(OBJ_SUFFIX) rc5$(OBJ_SUFFIX) rc6a$(OBJ_SUFFIX) shift$(OBJ_SUFFIX) \
       rc6aM0$(OBJ_SUFFIX) lowlatency$(OBJ_SUFFIX) rcmm$(OBJ_SUFFIX) ruwido$(OBJ_SUFFIX)\
       rcrf8$(OBJ_SUFFIX) rmap$(OBJ_SUFFIX) rmapdoublebit$(OBJ_SUFFIX) lowlatencyPro$(OBJ_SUFFIX)


ifdef USER_WRAPPER
# In user mode, only compile user wrapping file under linux directory
LINUX_OBJS :=
else
# In kernel mode, compile all files to create final .ko

LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
endif

ifeq "$(ARCHITECTURE)" "LINUX"
ifdef USER_WRAPPER
# In kernel mode, compile all files to create final .ko
STBLAST_SUBDIRS :=
else
# In user mode, only compile user wrapping file under linux directory
STBLAST_SUBDIRS := linux
endif

else
STBLAST_SUBDIRS :=
endif

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(STBLAST_SUBDIRS))

ifeq "$(ARCHITECTURE)" "LINUX"
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,linux)
endif

#ifeq "$(ARCHITECTURE)" "LINUX"
ifdef USER_WRAPPER
# In user mode, only compile user wrapping file under linux directory
SUBDIRS     := linux
else
# In kernel mode, compile all files to create final .ko
SUBDIRS     :=
endif

#endif

ifdef IR_INVERT
    CFLAGS := $(CFLAGS) -DIR_INVERT
    OBJS:= $(OBJS) invinput$(OBJ_SUFFIX)
endif

ifdef STBLAST_TIMER_TASK_PRIORITY
    CFLAGS += -DSTBLAST_TIMER_TASK_PRIORITY=$(STBLAST_TIMER_TASK_PRIORITY)
endif

ifdef STBLAST_TASK_PRIORITY
    CFLAGS += -DSTBLAST_TASK_PRIORITY=$(STBLAST_TASK_PRIORITY)
endif

ifdef STBLAST_ENABLE_NEC_REPEAT
    CFLAGS += -DSTBLAST_NEC_REPEAT_MODE_FEATURE
endif
ifdef STBLAST_NOT_USE_IRB_JACK
    CFLAGS := $(CFLAGS) -DSTBLAST_NOT_USE_IRB_JACK
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS := $(CFLAGS) -DSTAPI_INTERRUPT_BY_NUMBER
endif

ifdef BLAST_API_DEBUG
    CFLAGS := $(CFLAGS) -DBLAST_API_DEBUG
endif

include $(DVD_MAKE)/defrules.mak

$(LIB_PREFIX)stblast$(LIB_SUFFIX): $(OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(OBJS)
	-$(RM) $(TARGETS)
else

include $(DVD_MAKE)/builddir.mak

endif
