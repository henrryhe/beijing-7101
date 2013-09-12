# UK 271006 Changed for ALSA

# Header inports go here
HEADER_IMPORTS += stos stcommon staudlx/linux staudlx staudlx/src stpti4

include $(DVD_MAKE)/kbuild.mak 

obj-m           := staudlx_alsa.o
staudlx_alsa-objs := st_pcm_core.o 

ifdef STAUDLX_ALSA_TEST 
staudlx_alsa-objs +=startup.o
endif 

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)
EXTRA_CFLAGS += $(DVD_LINUX_CFLAGS)
EXTRA_CFLAGS += -DST_OSLINUX -DST_$(DVD_FRONTEND) -D$(DVD_PLATFORM) -DST_LINUX

ifdef STAUD_REMOVE_PTI_SUPPORT
	CFLAGS := $(CFLAGS) -DSTAUD_REMOVE_PTI_SUPPORT
endif 

ifdef STAUDLX_ALSA_TEST
	CFLAGS := $(CFLAGS) -DSTAUDLX_ALSA_TEST
endif 

ifdef STAUDLX_ALSA_LDDE2.2
	CFLAGS := $(CFLAGS) -DSTAUDLX_ALSA_LDDE2.2
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

	$(RM) $(staudlx_alsa-objs)
	$(RM) $(foreach FILE,$(staudlx_alsa-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif 
