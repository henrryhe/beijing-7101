#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for product tree
#

# Set RPC_ROOT for correct operation of host.mak
RPC_ROOT=.
include ./src/mkfiles/host.mak

# We cannot continue to use "RPC_ROOT = . " as this causes the install
# pattern matching targets to be ignored, causing incorrect rules to be
# generated. So instead set RPC_ROOT to be the full pathname to the tree.
RPC_ROOT = $(subst $(subst _, ,_),\ ,$(shell $(PWD)))

SUBDIRS = configs docs src examples

DIST_SRC_LOCATION = .
DIST_SRC       = makefile

include $(RPC_ROOT)/src/mkfiles/subdir.mak

# Clearly announce a successful build (providing we have not been 
# directed to ignore errors).
ifeq (,$(findstring i,$(firstword $(MAKEFLAGS))))
all install : jabber
jabber :
	@$(ECHO) Multicom build completed successfully
endif

include $(RPC_ROOT)/src/mkfiles/install.mak

# provide a kbuild interface to the multicom build system to allow
# just the kernel modules to be rebuilt.

ifeq ($(KERNELDIR),)
KERNELDIR = $(KERNELDIR_ST40_LINUX_KO)
else
ifeq ($(KERNELDIR_ST40_LINUX_KO),)
KERNELDIR_ST40_LINUX_KO = $(KERNELDIR)
endif
endif

LIBDIR_MODULES = $(RPC_DIST)/lib/linux/st40

modules modules_install : $(LIBDIR_MODULES)/Makefile
	$(MAKE) -C $(KERNELDIR) KBUILD_EXTMOD=$(LIBDIR_MODULES) $@

modules : kbuild_wrapper

kbuild_wrapper :
	$(MAKE) KBUILD_WRAPPER=1 \
		KERNELDIR_ST40_LINUX_KO=$(KERNELDIR) \
		DISABLE_IA32_LINUX=1 \
		DISABLE_ST20_OS20=1 \
		DISABLE_ST40_OS21=1 \
		DISABLE_ST40_LINUX=1 \
		DISABLE_ST40_WINCE=1 \
		DISABLE_ST220_OS21=1 \
		DISABLE_ST231_OS21=1 \
		DISABLE_PLATFORM_WARNINGS=1 \
		install

$(LIBDIR_MODULES)/Makefile : $(RPC_ROOT)/src/mkfiles/kbuild.mak
	$(MKDIR) $(LIBDIR_MODULES)
	$(CP) $< $@
