DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon

STTTX_USE_DENC :=1

CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_STPTI

ifeq "$(findstring $(DVD_FRONTEND),5105 7710 5107)" "$(DVD_FRONTEND)"
ST20_IMPORTS := stpti4lt stos
else 
ifeq "$(DVD_FRONTEND)" "5188"
ST20_IMPORTS := stdemux stfdma stos
else
ST20_IMPORTS := $(DVD_TRANSPORT) stos
endif
endif

ST20_EXPORTS := stttx.h stttx.lib
ST20_TARGETS := stttx.lib

INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/main)


# Needed by STPTI ...
ifeq "$(DVD_FRONTEND)" "5514"
    HEADER_IMPORTS := $(HEADER_IMPORTS) stgpdma
endif

ifndef STTTX_USE_DENC
    ST20_IMPORTS := $(ST20_IMPORTS) stvbi 
endif

# local objects which comprise this component
ST20_OBJS := stttx.tco stttxtsk.tco 

# Then set up the ST40/ST200 versions of all those. Flags will probably be the
# same, so we don't touch those.
ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ST200_IMPORTS := $(ST20_IMPORTS)
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

# on 5100 TTX dma don't complete in time (SW workaround)
# also problem rarely seen on 5516 too
ifndef TTXDMA_NOT_COMPLETE_NO_WORKAROUND
ifdef TTXDMA_NOT_COMPLETE_WORKAROUND
ifneq "$(DVD_BACKEND)" "7020"
ifneq "$(findstring $(DVD_FRONTEND), 5105 5107 5188)" "$(DVD_FRONTEND)"
    ##CFLAGS := $(CFLAGS) -DTTXDMA_NOT_COMPLETE_WORKAROUND
endif
endif
else
ifneq "$(DVD_BACKEND)" "7020"
ifneq "$(findstring $(DVD_FRONTEND), 5105 5107 5188)" "$(DVD_FRONTEND)"
    ##CFLAGS := $(CFLAGS) -DTTXDMA_NOT_COMPLETE_WORKAROUND
endif
endif
endif
endif

ifndef STTTX_SYNC_OFFSET
  STTTX_SYNC_OFFSET=9000
endif

ifeq "$(DVD_FRONTEND)" "5528"
    CFLAGS := $(CFLAGS) 
endif

ifeq "$(DVD_BACKEND)" "7020"
  CFLAGS := $(CFLAGS) -DSTTTX_USE_DENC
endif

# Check environment variable build options
ifdef USE_PROFILE
    CFLAGS := $(CFLAGS) -D USE_PROFILE
endif

ifdef STTTX_PES_FILE
    CFLAGS := $(CFLAGS) -DSTTTX_PES_FILE=\"$(STTTX_PES_FILE)\"
endif

# Temporary hack to avoid problems with fast channels such as HSE
ifdef POOLING_TASK_DIVIDER
    CFLAGS := $(CFLAGS) -DPOOLING_TASK_DIVIDER=$(POOLING_TASK_DIVIDER)
endif

ifdef STTTX_HANDLER_TASK_PRIORITY
    CFLAGS := $(CFLAGS) -DSTTTX_HANDLER_TASK_PRIORITY=$(STTTX_HANDLER_TASK_PRIORITY)
endif
 
ifdef STTTX_POOLING_TASK_PRIORITY
    CFLAGS := $(CFLAGS) -DSTTTX_POOLING_TASK_PRIORITY=$(STTTX_POOLING_TASK_PRIORITY)
endif

ifdef STTTX_TIMEOUT_TASK_PRIORITY
    CFLAGS := $(CFLAGS) -DSTTTX_TIMEOUT_TASK_PRIORITY=$(STTTX_TIMEOUT_TASK_PRIORITY)
endif
   
ifdef STTTX_DEBUG
  CFLAGS := $(CFLAGS) -DSTTBX_REPORT -DSTTTX_DEBUG -DSTTBX_PRINT
endif

ifdef STTTX_SUBT_SYNC_ENABLE
  CFLAGS := $(CFLAGS) -DSTTTX_SUBT_SYNC_ENABLE -DSTTTX_SYNC_OFFSET=$(STTTX_SYNC_OFFSET)
endif

ifdef STTTX_DEBUG1
  CFLAGS := $(CFLAGS) -DSTTTX_DEBUG1
endif

# Default to testing from a live stream
ifndef STTTX_PES_MODE
  STTTX_PES_MODE := 0
endif

# Append options to cflags
CFLAGS := $(CFLAGS) -DSTTTX_PES_MODE=$(STTTX_PES_MODE)

# extra options

# if set, pass it in to the driver
ifeq (,$(findstring, STPTI, $(DVD_TRANSPORT_UPPERCASE)))
  CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_$(DVD_TRANSPORT_UPPERCASE)
else
ifeq (,$(findstring, DEMUX, $(DVD_TRANSPORT_UPPERCASE)))
  CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_$(DVD_TRANSPORT_UPPERCASE)
else
  # else default to PTI
  DVD_TRANSPORT_UPPERCASE := PTI
  CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_$(DVD_TRANSPORT_UPPERCASE)
endif
endif

# The DVD_TRANSPORT_STPTI saves 'if (stpti) || (stpti4)' all the time;
# the STPTI_DVB_SUPPORT is required, otherwise the stpti.h file doesn't
# declare a couple of functions (STPTI_BufferExtractPesPacketData,
# STPTI_BufferReadPes) that we use.
ifeq "$(DVD_TRANSPORT_UPPERCASE)" "STPTI4"
  CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_STPTI -DSTPTI_DVB_SUPPORT
endif

# poke direct to denc, rather than using stvbi
ifdef STTTX_USE_DENC
  CFLAGS := $(CFLAGS) -DSTTTX_USE_DENC
endif

# link debug?
ifdef STTTX_LINK_DEBUG
   CFLAGS := $(CFLAGS) -DSTTTX_LINK_DEBUG
endif

# Default rules
include $(DVD_MAKE)/defrules.mak

# This builds this component
$($(ARCHITECTURE)_TARGETS): $($(ARCHITECTURE)_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif
