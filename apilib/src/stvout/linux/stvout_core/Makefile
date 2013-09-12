# KBUILD Makefile for stvout_core

HEADER_IMPORTS += stvout/linux stvout/linux/stvout_core stvout stvout/src
HEADER_IMPORTS += stvout/src/hal stvout/src/hal/svm stvout/src/hal/sddig stvout/src/hal/hdout stvout/src/hal/denc
HEADER_IMPORTS += stvout/src/hdmi stvout/src/hdmi/hal stvout/src/hdmi/hal/hdmi_ip stvout/src/hdmi/hal/hdcp_ip
HEADER_IMPORTS += stos stcommon

include $(DVD_MAKE)/kbuild.mak
obj-m            := stvout_core.o

ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
stvout_core-objs := stvout_core_main.o \
                    stvout_core_exports.o \
                    ../../src/stvout.o \
                    ../../src/vout_drv.o \
                    ../../src/hal/denc/dac_out.o \
                    ../../src/hal/denc/denc_out.o \
                    ../../src/hal/hdout/analog.o \
                    ../../src/hal/hdout/digital.o \
                    ../../src/hdmi/hdmi_api.o \
                    ../../src/hdmi/hdcp.o \
                    ../../src/hdmi/hdmi_pro.o \
                    ../../src/hdmi/hdmi_int.o \
                    ../../src/hdmi/hal/hal_hdmi.o \
                    ../../src/hdmi/hal/hdmi_ip/hdmi_ip.o \
                    ../../src/hdmi/hal/hdcp_ip/hdcp_ip.o
else
stvout_core-objs := stvout_core_main.o \
                    stvout_core_exports.o \
                    ../../src/stvout.o \
                    ../../src/vout_drv.o \
                    ../../src/hal/denc/dac_out.o \
                    ../../src/hal/denc/denc_out.o \
                    ../../src/hal/hdout/analog.o \
                    ../../src/hal/hdout/digital.o \
                    ../../src/hdmi/hdmi_api.o \
                    ../../src/hdmi/hdmi.o \
                    ../../src/hdmi/hdmi_int.o \
                    ../../src/hdmi/hal/hal_hdmi.o \
                    ../../src/hdmi/hal/hdmi_ip/hdmi_ip.o
endif



ifeq "$(STVOUT_CEC_CAPTURE)" "TRUE"
stvout_core-objs +=  ../../src/hdmi/cec.o
else
ifeq "$(STVOUT_CEC_PIO_COMPARE)" "TRUE"
stvout_core-objs += ../../src/hdmi/cec.o
endif
endif


EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)

ifeq "$(STVOUT_CEC_CAPTURE)" "TRUE"
        DVD_CFLAGS :=$(DVD_CFLAGS)  -DSTVOUT_CEC_PIO_COMPARE -DSTVOUT_CEC_CAPTURE
else
ifeq "$(STVOUT_CEC_PIO_COMPARE)" "TRUE"
        DVD_CFLAGS :=$(DVD_CFLAGS)  -DSTVOUT_CEC_PIO_COMPARE
endif
endif


# From src/makefile
VFE_FAMILY = 5528 5301
ifeq "$(findstring $(DVD_FRONTEND), $(VFE_FAMILY))" "$(DVD_FRONTEND)"
VOUT_DEFS := -D STVOUT_DVO
endif

ST7X_FAMILY = 7100 7109 7200
ifeq "$(findstring $(DVD_FRONTEND), $(ST7X_FAMILY))" "$(DVD_FRONTEND)"
VOUT_DEFS := -D STVOUT_HDOUT -D STVOUT_HDDACS -D STVOUT_HDMI
endif
EXTRA_CFLAGS += $(VOUT_DEFS)

ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
        EXTRA_CFLAGS := $(EXTRA_CFLAGS) -DSTVOUT_HDCP_PROTECTION
endif

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

ifeq "$(KDIR)" ""
$(error The environment variable KDIR must be set)
endif

# External makefile
PWD     := $(shell pwd)

all: default

default:
	$(MAKE) V=1 -C $(KDIR) M=$(PWD)  modules

# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest

clean:
	$(RM) $(stvout_core-objs)
	$(RM) $(foreach FILE,$(stvout_core-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif
