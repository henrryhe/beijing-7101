#####################################
# MODULE : stSTRCOM.lib / libstSTRCOM.a #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := 
ST20_IMPORTS := 
ST40_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS := wince_os.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST20_EXPORTS := 
ST40_EXPORTS := 

ST20_OBJS :=  WCE_Collection.tco OS21WrapperClock.tco  \
			  OS21WrapperMessageQueue.tco OS21WrapperSemaphore.tco OS21WrapperTask.tco \
			  OS21WrapperMisc.tco OS21WrapperMemory.tco OS21WrapperFile.tco OS21WrapperInterrupt.tco \
			  OS21WrapperConsole.tco OS21WrapperMutex.tco OS21WraperPriorityMap.tco
ST20_OBJS +=  winceinterrupt.tco wincememory.tco wincesemaphore.tco wincetasks.tco 
			      
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Define HW cells present
#--------------------------
ifeq "$(WINCE_MASTER)" "0"
	WINCE_OS_SUBDIRS := SerialIO KitlIO UDPIO
endif


# Pass definitions to sub make calls
export STRCOM_DEFS
export STRCOM_OPTIONAL_MODULES

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(WINCE_OS_SUBDIRS))  

# Sub-libs for HAL :
#--------------------------

SUBDIRS     := $(WINCE_OS_SUBDIRS)

# Compile options
#----------------
CFLAGS := $(COMMON_DEFS) $(CFLAGS)
ifeq "$(WINCE_PERF_OPTIMIZE)" "1"
CFLAGS += -D WINCE_PERF_OPTIMIZE
endif


# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$()
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)



subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
