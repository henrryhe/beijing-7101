# PJS 270705 Changed to just use KDIR

STAUD_CFLAGS += -DPCMP0_SUPPORTED -DSPDIFP0_SUPPORTED

ifeq "$(DVD_FRONTEND)" "7100"
chipdir := 7100
STAUD_CFLAGS += -DBYTES_PER_CHANNEL_IN_PLAYER=4 -DTENCH_TRANSFER_COUNT=10 -DPCMP1_SUPPORTED -DTW0CH_TRANSFER_COUNT=4
endif

ifeq "$(DVD_FRONTEND)" "7109"
chipdir := 7100
STAUD_CFLAGS += -DBYTES_PER_CHANNEL_IN_PLAYER=4 -DTENCH_TRANSFER_COUNT=20 -DPCMP1_SUPPORTED -DTW0CH_TRANSFER_COUNT=4
endif

ifeq "$(DVD_FRONTEND)" "7200"
chipdir := 7200
STAUD_CFLAGS += -DBYTES_PER_CHANNEL_IN_PLAYER=4 -DTW0CH_TRANSFER_COUNT=4 -DTENCH_TRANSFER_COUNT=20 -DPCMP1_SUPPORTED -DPCMP2_SUPPORTED -DPCMP3_SUPPORTED -DHDMIPCMP0_SUPPORTED -DHDMISPDIFP0_SUPPORTED
endif


# Header inports go here
HEADER_IMPORTS += stfdma stcommon stos stpti4 stclkrv stavmem staudlx staudlx/linux
HEADER_IMPORTS += staudlx/src staudlx/src/common staudlx/src/decoder
HEADER_IMPORTS += staudlx/src/input staudlx/src/mixer staudlx/src/output
HEADER_IMPORTS += staudlx/src/parser staudlx/src/audiocodec_interface
HEADER_IMPORTS += staudlx/src/encoder staudlx/src/post_processor
HEADER_IMPORTS += staudlx/src/output/$(chipdir)

include $(DVD_MAKE)/kbuild.mak

obj-m           := staudlx_core.o

ifndef STAUDLX_UNINSTALL_WMA
	ifndef STAUDLX_UNINSTALL_WMAPRO
		ifndef WMA_PARSER_SOURCE_BUILD
		EXTRA_LDFLAGS  += ${PWD}/../../src/parser/libwmaasfparser_linux.lib
		endif
	endif
endif

staudlx_core-objs := staudlx_core_main.o \
    staudlx_core_open.o \
    staudlx_core_read.o \
    staudlx_core_ioctl.o \
    staudlx_core_ops.o \
    staudlx_core_evts.o \
    staudlx_core_proc.o \
    staudlx_core_exports.o \
    ../../src/staud.o \
    ../../src/aud_wrap.o \
    ../../src/common/staud_core.o\
    ../../src/common/acc_multicom_toolbox.o \
    ../../src/common/aud_evt.o \
    ../../src/common/aud_utils.o \
    ../../src/common/acc_mme_wrapper.o \
    ../../src/common/blockmanager.o \
    ../../src/decoder/aud_mmeaudiostreamdecoder.o \
    ../../src/encoder/aud_mmeaudiostreamencoder.o \
    ../../src/decoder/aud_mmeaudiodecoderpp.o \
    ../../src/input/pcminput.o \
    ../../src/input/aud_pcmreader.o \
    ../../src/mixer/aud_mmeaudiomixer.o \
    ../../src/post_processor/aud_mmepp.o \
    ../../src/parser/aud_pes_es_parser.o \
    ../../src/parser/com_parser.o \
    ../../src/parser/es_ac3_parser.o \
    ../../src/parser/es_cdda_parser.o \
    ../../src/parser/es_dts_parser.o \
    ../../src/parser/es_lpcma_parser.o \
    ../../src/parser/es_lpcmv_parser.o \
    ../../src/parser/es_mlp_parser.o \
    ../../src/parser/es_mpeg_parser.o \
    ../../src/parser/pes_dvda_parser.o \
    ../../src/parser/pes_dvdv_parser.o \
    ../../src/parser/pes_mp2_parser.o \
    ../../src/parser/pes_packmp1_parser.o \
    ../../src/parser/parse_heaac.o \
    ../../src/parser/es_wmapes_parser.o \
    ../../src/parser/es_aac_parser.o \
    ../../src/parser/es_wave_parser.o \
    ../../src/parser/es_aiff_parser.o \
    ../../src/output/aud_pcmplayer.o \
    ../../src/output/aud_spdifplayer.o \
    ../../src/output/$(chipdir)/aud_pcmplayer_$(chipdir).o \
    ../../src/output/$(chipdir)/aud_spdifplayer_$(chipdir).o

ifndef STAUDLX_UNINSTALL_WMA
	ifndef STAUDLX_UNINSTALL_WMAPRO
		ifdef WMA_PARSER_SOURCE_BUILD
		staudlx_core-objs += ../../src/parser/wmaasfparser/es_wma_parser.o
		endif
	endif
endif

ifdef STAUD_VERBOSE_PRINT
staudlx_core-objs +=    ../../src/common/staud_prints.o
endif

# Cannot leave optimization to level 0 because of unknown symbol
# so set level to 2
OPTLEVEL := 2

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
EXTRA_CFLAGS += $(DVD_LINUX_CFLAGS)
EXTRA_CFLAGS += -DSTB \
                -DSTCLKRV_EXT_CLK_MHZ=${STCLKRV_EXT_CLK_MHZ} \
                -DLINUX_FULL_KERNEL_VERSION \
                -I$(RPC_ROOT)/include

ifndef DVD_PLATFORM
  DVD_PLATFORM := mb411
endif

ifeq "$(DVD_PLATFORM)" "mb411"
EXTRA_CFLAGS += -D$(DVD_PLATFORM) \
-DST_$(DVD_FRONTEND) \
-DARCHITECTURE_ST40 \
-D__SH4__ \
-DSTAUD_REMOVE_EVT_SUPPORT
endif

ifeq "$(DVD_PLATFORM)" "mb519"
EXTRA_CFLAGS += -D$(DVD_PLATFORM) \
-DST_$(DVD_FRONTEND) \
-DARCHITECTURE_ST40 \
-D__SH4__
endif

ifdef STAUD_REMOVE_CLKRV_SUPPORT
EXTRA_CFLAGS += -DSTAUD_REMOVE_CLKRV_SUPPORT
endif

ifdef STAUD_REMOVE_PTI_SUPPORT
EXTRA_CFLAGS += -DSTAUD_REMOVE_PTI_SUPPORT
endif

ifndef STCLKRV_EXT_CLK_MHZ
STCLKRV_EXT_CLK_MHZ := 27
endif

ifdef STTBX_PRINT
#EXTRA_CFLAGS += -DSTTBX_PRINT
endif

ifdef STTBX_REPORT
EXTRA_CFLAGS += -DSTTBX_REPORT
endif

ifdef STAUDLX_ALSA_SUPPORT
EXTRA_CFLAGS += -DSTAUDLX_ALSA_SUPPORT
endif

ifdef STAUD_VERBOSE_PRINT
EXTRA_CFLAGS += -DSTAUD_VERBOSE_PRINT
endif

ifeq "$(DVR)" "1"
	EXTRA_CFLAGS += -DDVR
endif

ifndef STAUDLX_UNINSTALL_AC3_DDP
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_AC3
	endif
	
ifndef STAUDLX_UNINSTALL_MPEG
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_MPEG
endif
	
ifndef STAUDLX_UNINSTALL_HEAAC
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_HEAAC
endif
	
ifndef STAUDLX_UNINSTALL_DTS
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_DTS
endif
	
ifndef STAUDLX_UNINSTALL_CDDA_PCM
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_CDDA
endif
ifndef STAUDLX_UNINSTALL_MLP
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_MLP
endif
ifndef STAUDLX_UNINSTALL_LPCMV
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_LPCMV
endif
ifndef STAUDLX_UNINSTALL_LPCMA
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_LPCMA
endif
ifndef STAUDLX_UNINSTALL_WMA
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_WMA
endif
ifndef STAUDLX_UNINSTALL_WMAPRO
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_WMAPROLSL
endif
ifndef STAUD_UNINSTALL_WAVE
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_WAVE
endif
ifndef STAUD_UNINSTALL_AIFF
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_AIFF
endif
	
ifndef STAUDLX_UNINSTALL_AAC
		EXTRA_CFLAGS +=  -DSTAUD_INSTALL_AAC
endif



ifeq "$(ENABLE_SPDIF_INTERRUPT)" "1"
	EXTRA_CFLAGS += -DENABLE_SPDIF_INTERRUPT
endif

ifeq "$(STAUDLX_UNINSTALL_AC3_DDP)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_AC3_DDP
endif

ifeq "$(STAUDLX_UNINSTALL_MPEG)" "1"
	STAUD_CFLAGS += -DSTAUDLX_UNINSTALL_MPEG
endif

ifeq "$(STAUDLX_UNINSTALL_HEAAC)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_HEAAC
endif

ifeq "$(STAUDLX_UNINSTALL_DTS)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_DTS
endif

ifeq "$(STAUDLX_UNINSTALL_CDDA_PCM)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_CDDA_PCM
endif

ifeq "$(STAUDLX_UNINSTALL_LPCMV)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_LPCMV
endif

ifeq "$(STAUDLX_UNINSTALL_LPCMA)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_LPCMA
endif

ifeq "$(STAUDLX_UNINSTALL_WMA)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_WMA
endif

ifeq "$(STAUDLX_UNINSTALL_WMAPRO)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_WMAPRO
endif

EXTRA_CFLAGS +=$(STAUD_CFLAGS)

# The following checks to see if we have been invoked in the kbuild
# (KERNELRELEASE will be defined). If not we have the means of launching
# the KBUILD (all and default targets).

ifneq ($(KERNELRELEASE),)
# Kernel makefile
else
# External makefile
PWD     := $(shell pwd)

all: default

default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules

# Some version of tidying up...
clean:
	$(RM) $(staudlx_core-objs)
	$(RM) Modules.symvers
	$(RM) $(foreach FILE,$(staudlx_core-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif

