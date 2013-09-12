# KBUILD Makefile for stvid_core

ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))

ifeq ($(DVD_STVIN),MASTER)
  VID_OPTIONAL_FEATURES = MasterDigInput diginput
endif

endif

HEADER_IMPORTS += stvid stvid/src
ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
HEADER_IMPORTS += stvid/src/api stvid/src/avsync stvid/src/display stvid/src/display/hal stvid/src/display/hal/sddispo2 stvid/src/buffers stvid/src/buffers/hal stvid/src/buffers/hal/genbuff stvid/src/producer stvid/src/ordqueue
endif
ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
HEADER_IMPORTS += stvid/src/decode stvid/src/decode/hal stvid/src/trickmod
endif
HEADER_IMPORTS += stpti4 stclkrv
HEADER_IMPORTS += stcommon stos

ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
HEADER_IMPORTS += stfdma
ifneq (,$(findstring $(DVD_FRONTEND),7100 7109))
HEADER_IMPORTS += stvid/src/decode/hal/lcmpegx1
endif
HEADER_IMPORTS += stvid/src/inject
HEADER_IMPORTS += stvid/src/producer stvid/src/producer/vidcodec stvid/src/producer/vidcodec/vcodh264 stvid/src/producer/vidcodec/vcodh264/parser stvid/src/producer/vidcodec/vcodh264/decoder stvid/src/producer/vidcodec/vcodh264/decoder/preprocessor
HEADER_IMPORTS += stvid/src/speed
ifeq "$(DVD_FRONTEND)" "7200"
HEADER_IMPORTS +=  stvid/src/producer/vidcodec/vcodmpg2 stvid/src/producer/vidcodec/vcodmpg2/parser stvid/src/producer/vidcodec/vcodmpg2/decoder
HEADER_IMPORTS +=  stvid/src/producer/vidcodec/vcodvc1 stvid/src/producer/vidcodec/vcodvc1/parser stvid/src/producer/vidcodec/vcodvc1/decoder
endif
ifeq "$(DVD_FRONTEND)" "7109"
ifeq "$(STVID_USE_DMPEG_AS_CODEC)" "YES"
HEADER_IMPORTS +=  stvid/src/producer/vidcodec/vcodmpg2 stvid/src/producer/vidcodec/vcodmpg2/parser stvid/src/producer/vidcodec/vcodmpg2/decoder
endif
HEADER_IMPORTS +=  stvid/src/producer/vidcodec/vcodvc1 stvid/src/producer/vidcodec/vcodvc1/parser stvid/src/producer/vidcodec/vcodvc1/decoder
HEADER_IMPORTS +=  stvid/src/producer/vidcodec/vcodavs stvid/src/producer/vidcodec/vcodavs/parser stvid/src/producer/vidcodec/vcodavs/decoder
endif
HEADER_IMPORTS +=  stvid/src/producer/vidcodec/vcodmpg4p2 stvid/src/producer/vidcodec/vcodmpg4p2/parser stvid/src/producer/vidcodec/vcodmpg4p2/decoder
ifeq "$(DVD_FRONTEND)" "7100"
ifdef STVID_USE_DEBLOCK
HEADER_IMPORTS +=  stvid/src/deblock
endif
endif
endif

ifneq ($(STVID_USE_CRC),)
  HEADER_IMPORTS += stvid/src/crc
endif

ifneq ($(STVID_USE_FGT),)
    HEADER_IMPORTS  += stvid/src/fgt
endif

ifeq "$(DVD_FRONTEND)" "5528"
HEADER_IMPORTS += stvid/src/decode/hal/sdmpgo2
HEADER_IMPORTS += stvid/src/trickmod
endif

ifneq (,$(findstring $(DVD_FRONTEND),7109 7200))
HEADER_IMPORTS += stvid/src/display/hal/displaypipe
endif

ifneq (,$(findstring $(DVD_FRONTEND),7109))
HEADER_IMPORTS += stvid/src/display/hal/graphics
endif

include $(DVD_MAKE)/kbuild.mak

obj-m           := stvid_core.o
stvid_core-objs :=  stvid_core_main.o \
                    stvid_core_exports.o \
		    stvid_core_proc.o

stvid_core-objs += ../../src/vid_ctcm.o
stvid_core-objs += ../../src/api/vid_buf.o  \
                   ../../src/api/vid_com.o  \
                   ../../src/api/vid_comp.o \
                   ../../src/api/vid_deco.o \
                   ../../src/api/vid_err.o  \
                   ../../src/api/vid_pict.o

ifneq ($(findstring diginput,$(VID_OPTIONAL_FEATURES)),)
  stvid_core-objs += ../../src/api/vid_dvpw.o
endif

ifneq ($(STVID_USE_CRC),)
stvid_core-objs += ../../src/crc/vid_crc.o \
                   ../../src/api/vid_crcapi.o
endif

ifndef STVID_NO_AVSYNC
stvid_core-objs += ../../src/avsync/vid_sync.o
endif
stvid_core-objs += ../../src/display/vid_cmd.o  \
                   ../../src/display/vid_disp.o \
                   ../../src/display/vid_flds.o \
                   ../../src/display/vid_queu.o \
                   ../../src/display/vid_sets.o \
                   ../../src/display/vid_dini.o \
                   ../../src/display/hal/halv_dis.o \
                   ../../src/display/hal/sddispo2/hv_dis8.o

stvid_core-objs += ../../src/buffers/vid_buff.o \
                   ../../src/buffers/hal/halv_buf.o \
                   ../../src/buffers/hal/genbuff/genbuff.o

#Do not use the decode module for the 7200...
ifeq (,$(findstring $(DVD_FRONTEND),7200))
stvid_core-objs += ../../src/decode/vid_bits.o \
                   ../../src/decode/vid_dec.o \
                   ../../src/decode/hal/halv_dec.o \
                   ../../src/decode/vid_head.o \
                   ../../src/decode/vid_intr.o \
                   ../../src/decode/vid_sc.o \
                   ../../src/trickmod/vid_tric.o
endif
stvid_core-objs += ../../src/ordqueue/vid_ordq.o
ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
stvid_core-objs += ../../src/speed/vid_speed.o
endif

ifeq (,$(findstring $(DVD_FRONTEND),7200))
ifeq ($(DVD_SERVICE),DIRECTV)
stvid_core-objs += ../../src/decode/vid_dtv.o
endif
endif

ifeq "$(DVD_FRONTEND)" "5528"
stvid_core-objs += ../../src/decode/hal/sdmpgo2/hv_dec8.o
endif


#deblocking for 7100
ifeq "$(DVD_FRONTEND)" "7100"
ifdef STVID_USE_DEBLOCK
stvid_core-objs += ../../src/deblock/deblock.o
endif
endif

ifneq (,$(findstring $(DVD_FRONTEND),7109 7200))
stvid_core-objs += ../../src/display/hal/displaypipe/hv_vdp.o
endif

ifneq (,$(findstring $(DVD_FRONTEND),7109))
stvid_core-objs += ../../src/display/hal/graphics/hv_gdp.o
endif

ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
ifneq (,$(findstring $(DVD_FRONTEND),7100 7109))
stvid_core-objs += ../../src/decode/hal/lcmpegx1/lcmpegx1.o
endif
ifdef STUB_INJECT
stvid_core-objs += ../../src/inject/inject_stub.o
else
stvid_core-objs += ../../src/inject/inject.o
endif
stvid_core-objs += ../../src/producer/vid_prod.o \
                   ../../src/producer/producer.o \
                   ../../src/producer/vidcodec/vidcodec.o \
                   ../../src/producer/vidcodec/vcodh264/parser/h264parser.o \
                   ../../src/producer/vidcodec/vcodh264/parser/marking.o \
                   ../../src/producer/vidcodec/vcodh264/parser/nal.o \
                   ../../src/producer/vidcodec/vcodh264/parser/parse_pps.o \
                   ../../src/producer/vidcodec/vcodh264/parser/parse_sei.o \
                   ../../src/producer/vidcodec/vcodh264/parser/parse_slice.o \
                   ../../src/producer/vidcodec/vcodh264/parser/parse_sps.o \
                   ../../src/producer/vidcodec/vcodh264/parser/poc.o \
                   ../../src/producer/vidcodec/vcodh264/parser/ref_pic_list.o \
                   ../../src/producer/vidcodec/vcodh264/decoder/h264decode.o \
                   ../../src/producer/vidcodec/vcodh264/decoder/h264task.o \
                   ../../src/producer/vidcodec/vcodh264/decoder/preprocessor/h264preproc.o \
                   ../../src/producer/vidcodec/vcodh264/decoder/preprocessor/h264preproc_transformer.o

### MPEG2
ifeq "$(DVD_FRONTEND)" "7200"
stvid_core-objs += ../../src/producer/vidcodec/vcodmpg2/parser/mpg2parser.o ../../src/producer/vidcodec/vcodmpg2/parser/mpg2scparse.o ../../src/producer/vidcodec/vcodmpg2/parser/mpg2parsertsk.o ../../src/producer/vidcodec/vcodmpg2/parser/mpg2getbits.o ../../src/producer/vidcodec/vcodmpg2/parser/mpg2dtvud.o
stvid_core-objs += ../../src/producer/vidcodec/vcodmpg2/decoder/mpg2dec.o
ifdef STVID_VALID
stvid_core-objs += ../../src/producer/vidcodec/vcodmpg2/decoder/mpg2dumpmme.o ../../src/producer/vidcodec/vcodmpg2/decoder/mpg2stub.o
endif
endif
ifeq "$(DVD_FRONTEND)" "7109"
ifeq "$(STVID_USE_DMPEG_AS_CODEC)" "YES"
stvid_core-objs += ../../src/producer/vidcodec/vcodmpg2/parser/mpg2parser.o ../../src/producer/vidcodec/vcodmpg2/parser/mpg2scparse.o ../../src/producer/vidcodec/vcodmpg2/parser/mpg2parsertsk.o ../../src/producer/vidcodec/vcodmpg2/parser/mpg2getbits.o ../../src/producer/vidcodec/vcodmpg2/parser/mpg2dtvud.o
stvid_core-objs += ../../src/producer/vidcodec/vcodmpg2/decoder/mpg2dec.o
ifdef STVID_VALID
stvid_core-objs += ../../src/producer/vidcodec/vcodmpg2/decoder/mpg2dumpmme.o ../../src/producer/vidcodec/vcodmpg2/decoder/mpg2stub.o
endif
endif
endif

### VC1
ifneq (,$(findstring $(DVD_FRONTEND),7109 7200))
stvid_core-objs += ../../src/producer/vidcodec/vcodvc1/parser/vc1parser.o ../../src/producer/vidcodec/vcodvc1/parser/bdu.o ../../src/producer/vidcodec/vcodvc1/parser/parsebdu.o
stvid_core-objs += ../../src/producer/vidcodec/vcodvc1/decoder/vc1decode.o
ifdef STVID_VALID
stvid_core-objs += ../../src/producer/vidcodec/vcodvc1/decoder/vc1dumpmme.o ../../src/producer/vidcodec/vcodvc1/decoder/vc1stub.o
endif
endif

### MPG4P2
stvid_core-objs += ../../src/producer/vidcodec/vcodmpg4p2/parser/mpg4p2parser.o ../../src/producer/vidcodec/vcodmpg4p2/parser/parser.o ../../src/producer/vidcodec/vcodmpg4p2/parser/vbs.o
stvid_core-objs += ../../src/producer/vidcodec/vcodmpg4p2/decoder/mpg4p2decode.o

### AVS
ifneq (,$(findstring $(DVD_FRONTEND),7109))
stvid_core-objs += ../../src/producer/vidcodec/vcodavs/parser/avs_vbs.o
stvid_core-objs += ../../src/producer/vidcodec/vcodavs/parser/avsparser.o
stvid_core-objs += ../../src/producer/vidcodec/vcodavs/parser/parser_avs.o
stvid_core-objs += ../../src/producer/vidcodec/vcodavs/decoder/avsdecode.o
endif

endif

ifneq ($(STVID_USE_FGT),)
    EXTRA_FLAGS     += -I../../src/fgt
    stvid_core-objs += ../../src/fgt/vid_fgt.o
endif

ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
# All MULTICOM concern
EXTRA_CFLAGS += -I$(RPC_ROOT)/include -I$(RPC_ROOT)/src/mme/include -I$(RPC_ROOT)/src/embx/include
endif

EXTRA_CFLAGS += -DAVMEM_KERNEL_MODE -DUSE_LMI_SYS_FOR_LINUX -DSTVID_DEBUG_GET_STATISTICS -DGLOBAL_PASS_PARAMS_THROUGH_IOCTLS
EXTRA_CFLAGS += -DST_producer -DST_display -DST_sddispo2 -DST_genbuff -DST_ordqueue
EXTRA_CFLAGS += -DST_speed

ifeq ($(STVID_TRICKMODE_NO_BACKWARD),)
EXTRA_CFLAGS += -D STVID_TRICKMODE_BACKWARD
endif
ifneq "$(DVD_FRONTEND)" "7200"
EXTRA_CFLAGS += -DST_trickmod -DST_decode -DST_smoothbackward -DST_oldmpeg2codec
endif

ifneq ($(STVID_DEBUG_GET_STATUS),)
EXTRA_CFLAGS +=  -DSTVID_DEBUG_GET_STATUS
endif

ifeq "$(DVD_FRONTEND)" "5528"
EXTRA_CFLAGS += -DST_sdmpgo2
endif

ifneq ($(STVID_USE_FGT),)
    EXTRA_CFLAGS +=  -DST_XVP_ENABLE_FLEXVP -DST_XVP_ENABLE_FGT -DSTVID_USE_FGT
endif

ifneq ($(VIDEO_DEBUG_DEINTERLACING_MODE),)
    EXTRA_CFLAGS +=  -DVIDEO_DEBUG_DEINTERLACING_MODE
endif

ifneq ($(STVID_DEBUG_GET_DISPLAY_PARAMS),)
    EXTRA_CFLAGS += -DSTVID_DEBUG_GET_DISPLAY_PARAMS
endif

ifneq ($(STVID_USE_CRC),)
EXTRA_CFLAGS += -DSTVID_USE_CRC -DST_crc
endif

ifneq ($(STVID_WA_GNBvd62704),)
EXTRA_CFLAGS += -DWA_GNBvd62704
endif

ifndef STUB_INJECT
    ifndef STVID_NOT_USE_VC1_MP_SC_SIGNATURE
        EXTRA_CFLAGS += -DSTVID_USE_VC1_MP_SC_SIGNATURE
    endif
endif

ifneq (,$(findstring $(DVD_FRONTEND),7109 7200))
ifeq "$(DVD_FRONTEND)" "7109"
EXTRA_CFLAGS += -DST_displaypipe -DST_graphics -DST_vcodvc1
else
EXTRA_CFLAGS += -DST_displaypipe -DST_vcodvc1
endif
ifneq ($(VIDEO_MINIMIZE_MEMORY_USAGE_FOR_DEI),)
  EXTRA_CFLAGS += -DSTVID_MINIMIZE_MEMORY_USAGE_FOR_DEI
endif
ifeq "$(STVID_USE_DMPEG_AS_CODEC)" "YES"
  EXTRA_CFLAGS += -DSTVID_USE_DMPEG_AS_CODEC
endif
endif

ifeq "$(DVD_FRONTEND)" "7100"
ifdef STVID_USE_DEBLOCK
	EXTRA_CFLAGS += -DST_deblock -DSTVID_USE_DEBLOCK
endif
endif


ifeq "$(DVD_FRONTEND)" "7100"
EXTRA_CFLAGS +=  -D FORCE_SECONDARY_ON_AUX_DISPLAY
endif

# Enable Security features if DVD_SECURED_CHIP is set.
ifneq ($(DVD_SECURED_CHIP),)
EXTRA_CFLAGS +=  -D DVD_SECURED_CHIP
endif

ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
EXTRA_CFLAGS += -DST_multicodec -DST_inject -DST_lcmpegx1 -DST_vcodh264 -DST_vcodmpg4p2
ifndef STVID_NO_AVSYNC
	EXTRA_CFLAGS += -DST_avsync
endif
ifneq (,$(findstring $(DVD_FRONTEND),7200))
EXTRA_CFLAGS += -DST_vcodmpg2
endif
ifneq (,$(findstring $(DVD_FRONTEND),7109))
EXTRA_CFLAGS += -DST_vcodavs
endif
ifeq "$(DVD_FRONTEND)" "7109"
ifeq "$(STVID_USE_DMPEG_AS_CODEC)" "YES"
EXTRA_CFLAGS += -DST_vcodmpg2
endif
endif
ifneq ($(STVID_SUPPORT_ONLY_SD_PROFILES),)
EXTRA_CFLAGS += -DSUPPORT_ONLY_SD_PROFILES
endif

EXTRA_CFLAGS += -D LCMPEGH1 -DLCMPEGX1_WA_GNBvd45748
EXTRA_CFLAGS += -D__SH4__ -DHAL_ST40

ifdef STUB_INJECT
EXTRA_CFLAGS += -DSTUB_INJECT
endif

# Add optional features compilation
EXTRA_CFLAGS := $(EXTRA_CFLAGS) $(addprefix -D ST_,$(VID_OPTIONAL_FEATURES))

endif

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

# External makefile

ifeq "$(KDIR)" ""
$(error The environment variable KDIR must be set)
endif

PWD     := $(shell pwd)

all: default

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stvid_core-objs)
	$(RM) $(foreach FILE,$(stvid_core-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif
