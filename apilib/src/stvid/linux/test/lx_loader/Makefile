# PJS 270705 Changed to just use KDIR

# Header inports go here
HEADER_IMPORTS += stcommon stvid/tests/src/lx_loader stvid/tests/src stos

#IMPORTS += stcommon

include $(DVD_MAKE)/kbuild.mak 



obj-m           := mlxload.o
mlxload-objs := klxload.o \
		   ../lx_loader.o


LINUX_OBJS := $(obj-m)
LINUX_OBJS += $(mlxload-objs)

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)

# All MULTICOM concern
EXTRA_CFLAGS += -I$(DVD_ROOT)/dvdgr-prj-stapigat/src -I$(RPC_ROOT)/include -I$(RPC_ROOT)/src/mme/include -I$(RPC_ROOT)/src/embx/include
EXTRA_CFLAGS += -D__SH4__ -D__LINUX__ -D__KERNEL__

#TEMPORARY FLAG
EXTRA_CFLAGS += -DMODULE -DLINUX_OS -DST_OSLINUX -DLINUX_FULL_KERNEL_VERSION -DAVMEM_KERNEL_MODE
EXTRA_CFLAGS += -DSTTBX_PRINT -DUSE_LMI_SYS_FOR_LINUX -DSTVID_DEBUG_GET_STATISTICS -DGLOBAL_PASS_PARAMS_THROUGH_IOCTLS

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
	$(RM) $(LINUX_OBJS)
	$(RM) $(foreach FILE,$(LINUX_OBJS),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif
