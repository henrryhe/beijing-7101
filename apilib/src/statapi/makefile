DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak


# Other components this component is dependant upon
ST20_IMPORTS := stcommon stevt sttbx stos
ST40_IMPORTS := stcommon stevt sttbx stos
ST200_IMPORTS := stcommon stevt sttbx stos

ST20_TARGETS := statapi.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_EXPORTS := statapi.lib
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))


# local objects which comprise this component
ST20_OBJS = hal_atapi.tco ctrl_atapi.tco cmmd_atapi.tco cmmd_ata.tco statapi.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

# Build options

# *May* need changing on a per device basis. Relevant to PIO transfer only.
ifdef ATAPI_WAIT
    CFLAGS := $(CFLAGS) -DWAIT=$(ATAPI_WAIT)
else
    CFLAGS := $(CFLAGS) -DWAIT=3
endif

ifdef ATAPI_BMDMA
    CFLAGS := $(CFLAGS)  -DBMDMA_ENABLE
endif

ifdef ATAPI_DEBUG
    CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DATAPI_DEBUG -DSTTBX_REPORT

endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS := $(CFLAGS)  -DSTAPI_INTERRUPT_BY_NUMBER
endif

ifdef ATAPI_FDMA
    CFLAGS := $(CFLAGS) -DATAPI_FDMA
    ST20_IMPORTS += stfdma
    ST40_IMPORTS += stfdma
    ST200_IMPORTS += stfdma
endif

ifdef ATAPI_CRYPT
    CFLAGS := $(CFLAGS) -DSTATAPI_CRYPT
    ST20_IMPORTS  += stcrypt
endif

ifdef ATAPI_POLLING
    CFLAGS := $(CFLAGS) -DATAPI_POLLING
endif

ifdef HDDI_5514_CUT_3_0
    CFLAGS := $(CFLAGS) -DHDDI_5514_CUT_3_0
endif
ifdef HDDI_5514_CUT_3_1
    CFLAGS := $(CFLAGS) -DHDDI_5514_CUT_3_1
endif
ifdef HDDI_5514_CUT_3_2
    CFLAGS := $(CFLAGS) -DHDDI_5514_CUT_3_2
endif

ifdef STATAPI_BAT_PRIORITY
    CFLAGS := $(CFLAGS) -DBAT_PRIORITY=$(STATAPI_BAT_PRIORITY)
endif

ifdef STATAPI_NO_PARAMETER_CHECK
    CFLAGS := $(CFLAGS) -DSTATAPI_NO_PARAMETER_CHECK
endif

ifdef BAD_DISK_NO_SIG_CHECK_ATA
    CFLAGS := $(CFLAGS) -DBAD_DISK_NO_SIG_CHECK_ATA
endif

ifdef STATAPI_SET_EMIREGISTER_MAP
    CFLAGS := $(CFLAGS)  -DSTATAPI_SET_EMIREGISTER_MAP
endif  

ifdef STATAPI_HOSTCERROR_INTERRUPT_ENABLE
    CFLAGS := $(CFLAGS)  -DSTATAPI_HOSTCERROR_INTERRUPT_ENABLE
endif

include $(DVD_MAKE)/defrules.mak

$($(ARCHITECTURE)_TARGETS): $($(ARCHITECTURE)_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

hal_atapi.tco: hal_atapi.c hal_atapi.h

ctrl_atapi.tco: ata.h hal_atapi.h

cmmd_atapi.tco: ata.h hal_atapi.h

cmmd_ata.tco: ata.h hal_atapi.h

statapi.tco: ata.h hal_atapi.h

else
include $(DVD_MAKE)/builddir.mak
endif
