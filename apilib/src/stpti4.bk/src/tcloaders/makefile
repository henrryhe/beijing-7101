#########################################################################
# COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
#
# Source file name : makefile
#
#########################################################################
DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Find out what we are running on.
ifeq ($(strip $(DVD_HOST)),)
  ifneq (,$(findstring sh.exe,$(SHELL)))
    DVD_HOST := PC
  else
    DVD_HOST := UNIX
  endif
endif


ST20_IMPORTS = stos

ifeq ($(findstring STPTI_GPDMA_SUPPORT,$(STPTI_CONFIG)), STPTI_GPDMA_SUPPORT)
  ST20_IMPORTS += stgpdma
endif

ifeq ($(findstring STPTI_FDMA_SUPPORT,$(STPTI_CONFIG)), STPTI_FDMA_SUPPORT)
  ST20_IMPORTS += stfdma
endif

ifeq ($(findstring STPTI_PCP_SUPPORT,$(STPTI_CONFIG)), STPTI_PCP_SUPPORT)
 ST20_IMPORTS += stpcp
endif

ST40_IMPORTS := $(ST20_IMPORTS)
ST200_IMPORTS := $(ST20_IMPORTS)


# Actively gather the loaders present in this directory
# This mirrors a similar section in linux//stpti4_ioctl/makefile.  You should make sure these remain in sync!
ifeq ($(findstring STPTI_DVB_SUPPORT,$(STPTI_CONFIG)), STPTI_DVB_SUPPORT)
    # DVB loaders are all the DVB_Ldr*.c without the NDS ones DVB_LdrNDS*.c
    TC_LOADERS_CFILES += $(filter-out $(wildcard $(DVD_BUILD_DIR)/DVB_LdrNDS*.c),$(wildcard $(DVD_BUILD_DIR)/DVB_Ldr*.c))
    ifeq ($(findstring STPTI_NDS_SUPPORT,$(STPTI_CONFIG)), STPTI_NDS_SUPPORT)
        # DVB NDS loaders are all the DVB_LdrNDS*.c
        TC_LOADERS_CFILES += $(wildcard $(DVD_BUILD_DIR)/DVB_LdrNDS*.c)
    endif
endif

ifeq ($(findstring STPTI_DTV_SUPPORT,$(STPTI_CONFIG)), STPTI_DTV_SUPPORT)
    # DTV loaders are all DTV_Ldr*.c and COMBI_Ldr*.c
    TC_LOADERS_CFILES += $(wildcard $(DVD_BUILD_DIR)/DTV_Ldr*.c)
endif

ifeq ($(findstring STPTI_BSL_SUPPORT,$(STPTI_CONFIG)), STPTI_BSL_SUPPORT)
    ifeq ($(findstring STPTI_DVB_SUPPORT,$(STPTI_CONFIG)), STPTI_DVB_SUPPORT)
        ifeq ($(findstring 7109,$(DVD_FRONTEND)), 7109)
            # DTV loaders are all DTV_Ldr*.c and COMBI_Ldr*.c
            TC_LOADERS_CFILES = $(wildcard $(DVD_BUILD_DIR)/DVBBSL_LdrSL.c)
        endif
        ifeq ($(findstring 5301,$(DVD_FRONTEND)), 5301)
            # DTV loaders are all DTV_Ldr*.c and COMBI_Ldr*.c
            TC_LOADERS_CFILES = $(wildcard $(DVD_BUILD_DIR)/DVBBSL_LdrL.c)
        endif
    endif
endif

ifeq ($(findstring STPTI_BSL_SUPPORT,$(STPTI_CONFIG)), STPTI_BSL_SUPPORT)
    ifeq ($(findstring STPTI_DTV_SUPPORT,$(STPTI_CONFIG)), STPTI_DTV_SUPPORT)
        ifeq ($(findstring 7109,$(DVD_FRONTEND)), 7109)
            TC_LOADERS_CFILES = $(wildcard $(DVD_BUILD_DIR)/DTVBSL_LdrSL.c)
        endif
        ifeq ($(findstring 5301,$(DVD_FRONTEND)), 5301)
            # DTV loaders are all DTV_Ldr*.c and COMBI_Ldr*.c
            TC_LOADERS_CFILES = $(wildcard $(DVD_BUILD_DIR)/DTVBSL_LdrL.c)
        endif
    endif
endif

# Convert c filenames into object filenames
ST20_OBJS :=  $(notdir $(patsubst %.c,%.tco,$(TC_LOADERS_CFILES)))
ST40_OBJS :=  $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS :=  $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

CFLAGS += $(STPTI_CONFIG)

$(ARCHITECTURE)_TARGETS := $(LIB_PREFIX)tcloaders$(LIB_SUFFIX)

# Default rules
include $(DVD_MAKE)/defrules.mak

$($(ARCHITECTURE)_TARGETS): $($(ARCHITECTURE)_OBJS)
	$(BUILD_LIBRARY)

clean: 
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
    # We clean using wildcards to make sure we are comprehensive as the objects list
    # is also drawn from wildcards (*.c) and if the c files have been deleted the
    # objects list will be empty.
	$(RM) *.o *.tco
	$(RM) $($(ARCHITECTURE)_TARGETS)
    
    
    
else

include $(DVD_MAKE)/builddir.mak

endif

