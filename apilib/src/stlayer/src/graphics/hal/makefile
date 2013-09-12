#########################
# MODULE : graphics.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Define the targets and exported files
#--------------------------------------
ST20_TARGETS := graphics.lib
ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
else
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
endif
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))


# Conditional rules to generate the graphic layer lib (graphics.lib)
# The sub drivers used depend on the chip
#-------------------------------------------------------------------
ifeq "$(DVD_BACKEND)" "5508"
SUBDIRS := osdtopbt sub_pict
endif
ifeq "$(DVD_BACKEND)" "5510"
SUBDIRS := osdtopbt stillpic sub_pict
endif
ifeq "$(DVD_BACKEND)" "5512"
SUBDIRS := osdtopbt stillpic sub_pict
endif
ifeq "$(DVD_BACKEND)" "5514"
SUBDIRS := osdframe stillpic sub_pict
endif
ifeq "$(DVD_BACKEND)" "5516"
SUBDIRS := osdframe stillpic sub_pict
endif
ifeq "$(DVD_BACKEND)" "5517"
SUBDIRS := osdframe stillpic sub_pict
endif
ifeq "$(DVD_BACKEND)" "5518"
SUBDIRS := osdtopbt sub_pict
endif
ifeq "$(DVD_BACKEND)" "7015"
SUBDIRS := gamma
endif
ifeq "$(DVD_BACKEND)" "7020"
SUBDIRS := gamma
endif
ifeq "$(DVD_BACKEND)" "GX1"
SUBDIRS := gamma
endif
ifeq "$(DVD_BACKEND)" "5528"
SUBDIRS := gamma
endif
ifeq "$(DVD_BACKEND)" "7710"
SUBDIRS := gamma
endif
ifeq "$(DVD_BACKEND)" "7100"
SUBDIRS := gamma
endif
ifeq "$(DVD_BACKEND)" "7109"
SUBDIRS := gamma
endif
ifeq "$(DVD_BACKEND)" "7200"
SUBDIRS := gamma
endif

ifeq "$(DVD_BACKEND)" "DELTAPHI_HE"
SUBDIRS := gamma
endif
ifeq "$(DVD_BACKEND)" "DELTAMU_HE"
SUBDIRS := gamma
endif

ifeq "$(USE_GDP_NODE_DUMP)" "TRUE"
DVD_CFLAGS:=  -DDUMP_GDPS_NODES $(DVD_CFLAGS)
endif

ifeq "$(DVD_BACKEND)" "7109"
        ifneq ($(DVD_SECURED_CHIP),)
                DVD_CFLAGS := $(DVD_CFLAGS) -DDVD_SECURED_CHIP
                CFLAGS := $(CFLAGS) -D DVD_SECURED_CHIP
        endif
	ifneq ($(STLAYER_NO_STMES),)
		DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_NO_STMES
		CFLAGS := $(CFLAGS) -D STLAYER_NO_STMES
	endif
endif


# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the sub-libs
#-------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

# Delete all objects and libs
#----------------------------
clean: $(SUBDIR_CLEAN)
	$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

