# PJS 270705 Changed to just use KDIR

# Header inports go here
HEADER_IMPORTS += stcommon stos stpti4 stapigat/src stvid stvid/tests stvid/tests/src

include $(DVD_MAKE)/kbuild.mak 

obj-m               := stvidtest_core.o
stvidtest_core-objs := stvidtest_core_main.o \
		               stvidtest_core_exports.o \
		               kvid_inj.o

EXTRA_CFLAGS += $(DVD_INCLUDE_PATH)
EXTRA_CFLAGS += $(KBUILD_CFLAGS)

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
	$(MAKE) -C $(KDIR) M=$(PWD) modules



# Remove the object files, the .<object>.cmd file and use KBUILD to remove the rest
clean:
	$(RM) $(stvidtest_core-objs)
	$(RM) $(foreach FILE,$(stvidtest_core-objs),$(dir $(FILE)).$(notdir $(FILE)).cmd)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif
