DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

IMPORTS := stos sttbx
EXPORTS := stmerge.h $(LIB_PREFIX)stmerge$(LIB_SUFFIX)
TARGETS = $(LIB_PREFIX)stmerge$(LIB_SUFFIX)

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

ifdef STMERGE_DEFAULT_PARAMETERS
TSMERGE_CFLAGS += -DSTMERGE_DEFAULT_PARAMETERS 
endif

ifdef STMERGE_DTV_PACKET
TSMERGE_CFLAGS += -DSTMERGE_DTV_PACKET
endif

ifdef STMERGE_NO_TAG
TSMERGE_CFLAGS += -DSTMERGE_NO_TAG
endif

ifdef STMERGE_INTERRUPT_SUPPORT
TSMERGE_CFLAGS += -DSTMERGE_INTERRUPT_SUPPORT
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
TSMERGE_CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
endif

ifneq (,$(findstring $(DVD_FRONTEND),5100 7710 7100 5301 7109))
TSMERGE_CFLAGS += -DSTMERGE_NO_1394_IN
endif

CFLAGS += $(TSMERGE_CFLAGS) -DSTTBX_PRINT -DSTTBX_REPORT
OBJS := stmerge$(OBJ_SUFFIX)

include $(DVD_MAKE)/defrules.mak

# This builds this component
$(LIB_PREFIX)stmerge$(LIB_SUFFIX): $(SUBDIR_LIBS) $(OBJS)
	              @$(ECHO) Building $@
	              $(BUILD_LIBRARY)

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(OBJS)
	-$(RM) $(TARGETS)


#local dependancies
stmerge$(OBJ_SUFFIX)	:    stmerge.h mergereg.h

else

include $(DVD_MAKE)/builddir.mak

endif

