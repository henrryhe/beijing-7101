# UK 211206 Changed for Linux DVB  API

# Header inports go here
HEADER_IMPORTS += stos stcommon staudlx/linux staudlx staudlx/src stpti4

include $(DVD_MAKE)/kbuild.mak 

obj-m           := staudlx_dvb.o
# staudlx_dvb-objs := coreaud.o audiomod.o
staudlx_dvb-objs := audio_dvb.o


EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
EXTRA_CFLAGS += $(DVD_LINUX_CFLAGS)
EXTRA_CFLAGS += -DST_OSLINUX -DST_$(DVD_FRONTEND) -D$(DVD_PLATFORM) -DST_LINUX

ifdef STAUD_REMOVE_PTI_SUPPORT
	CFLAGS := $(CFLAGS) -DSTAUD_REMOVE_PTI_SUPPORT
endif 

# The following checks to see if we have been invoked in the kbuild
# (KERNELRELEASE will be defined). If not we have the means of launching
# the KBUILD (all and default targets).

ifneq ($(KERNELRELEASE),)

# Kernel makefile

else

# External makefile
PWD     := $(shell pwd)


default:
	$(MAKE) -C $(KDIR) M=$(PWD)  modules


# Some version of # Some version of tidying up...
clean:  

	$(RM) $(staudlx_dvb-objs)
	$(RM) $(foreach FILE,$(staudlx_dvb-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif 
