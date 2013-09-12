# 7109 reference tree STAPI src directory makefile

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak
include $(DVD_ROOT)/syspriority.mak

ST40_TARGETS := $(LIB_PREFIX)stapi_$(DVD_TRANSPORT)$(LIB_SUFFIX) ##$(LIB_PREFIX)testtool$(LIB_SUFFIX)
ST40_EXPORTS := $(LIB_PREFIX)stapi_$(DVD_TRANSPORT)$(LIB_SUFFIX)
ST40_IMPORTS := statapi staudlx stavmem stblast stblit stboot stcc stclkrv stcommon\
                stdenc stevt stfdma stflash stgxobj sthdmi sti2c stlayer stmerge\
                stos stpccrd stpio stpti4  stspi sttbx stttx \
                stvbi stvid stvmix stvout stvtg stpwm
##				stsmart sttuner  testtool stuart 

##ifneq ($(DVR),)
##	ST40_IMPORTS += stdvm stprm stnet
##endif



ifneq "$(DTV_TESTING)" "YES"
	STPTI_OPTIONS   += -DSTPTI_DVB_SUPPORT -DSTPTI_FDMA_SUPPORT
	STCLKRV_OPTIONS += -DSTPTI_DVB_SUPPORT
else
	STPTI_OPTIONS   += -DSTPTI_DTV_SUPPORT -DSTPTI_DVB_SUPPORT
	STCLKRV_OPTIONS += -DSTPTI_DTV_SUPPORT
endif


.PHONY: all

# Driver depended options:

stvid.lib libstvid.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STVID_OPTIONS) $(STVID_TASK_PRIORITIES)

$(DVD_TRANSPORT).lib lib$(DVD_TRANSPORT).a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STPTI_OPTIONS)  $(STPTI_TASK_PRIORITIES)

stclkrv.lib libstclkrv.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STCLKRV_OPTIONS) $(STCLKRV_TASK_PRIORITIES)

sttbx.lib libsttbx.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STTBX_FLAGS) $(STTBX_TASK_PRIORITIES)

stuart.lib libstuart.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STUART_TASK_PRIORITIES)

stlayer.lib libstlayer.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STLAYER_TASK_PRIORITIES)

stvtg.lib libstvtg.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STVTG_TASK_PRIORITIES)

stfdma.lib libstfdma.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STFDMA_TASK_PRIORITIES)

statapi.lib libstatapi.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STATAPI_OPTIONS) $(STATAPI_TASK_PRIORITIES)

staudlx.lib libstaudlx.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) -DWMA_IN_LOOP $(STAUDLX_TASK_PRIORITIES)

sttuner.lib libsttuner.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STTUNER_TASK_PRIORITIES)

stblast.lib libstblast.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STBLAST_TASK_PRIORITIES)

stblit.lib libstblit.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STBLIT_TASK_PRIORITIES)

stsmart.lib libstsmart.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STSMART_TASK_PRIORITIES)

stttx.lib libstttx.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STTTX_TASK_PRIORITIES)

stvout.lib libstvout.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STVOUT_TASK_PRIORITIES)

export DVD_CFLAGS

include $(DVD_MAKE)/defrules.mak

FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/st40
EMBX_IMPORTS = embxmailbox embxshm embxshell mme_host
IMPORT_EMBX_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(EMBX_IMPORTS)))
$(IMPORT_EMBX_LIBS):
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH), $(FULLDIR_EMBX)/$(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(LIBNAME_FROM_AT_F)))) \
    $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(ARCHITECTURE)_EXPORTS)) > $(NULL)

$(EXTERNAL_LIBS):
	@$(ECHO) Exporting $@

$(LIB_PREFIX)stapi_$(DVD_TRANSPORT)$(LIB_SUFFIX): $(IMPORT_LIBS) $(IMPORT_EMBX_LIBS) $(EXTERNAL_LIBS)
	@$(ECHO) Creating $@
	$(BUILD_LIBRARY)

.PHONY: clean

clean: clean_imports clean_libs clean_idb
	@$(ECHO) ---- Cleaning Tree ----
	-$(RM) $(LIB_PREFIX)*$(LIB_SUFFIX)
ifeq ($(DVD_HOST),pc)
	-$(RM) /q *
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of STAPI src directory makefile

