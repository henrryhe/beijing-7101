#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the RPC benchmark test
#

RPC_TEST = ..
RPC_ROOT = $(RPC_TEST)/../../..

include target.mak

#
# configurable macros
#

LIBNAME        = harness

SRCS           = harness.c \
                 harness_os20.c st_c1_timer_cc.c \
		 harness_os21.c \
		 harness_linux.c \
		 harness_wince.c \
		 stb5525_shm.c
HDRS           = harness.h

LOCAL_CFLAGS  += -I$(RPC_ROOT)/src/embx/include
 
OBJDIR_ST40_LINUX_KO := $(OBJDIR_ST40_LINUX_KO)/kernel
$(OBJDIR_ST40_LINUX_KO) :
	-$(MKDIR) $(call DOSCMD,$@)

$(OBJDIR_ST40_LINUX_KO)/%.$(OBJ_ST40_LINUX_KO) : %.c
	$(CC_ST40_LINUX_KO) -c $(CFLAGS_ST40_LINUX_KO) $< -o $@

#
# explicit rules
#

all: $(MAKE_DIRS) lib_master lib_slave

lib_master : CONF_CFLAGS = -DCONF_MASTER
lib_master : $(call LIBRARY_0,$(LIBNAME))

lib_slave : CONF_CFLAGS = -DCONF_SLAVE
lib_slave : $(call LIBRARY_1,$(LIBNAME))

run : all

#
# macroized rules (don't try to understand this unless you have to)
#

$(call LIBRARY_0,$(LIBNAME)) : $(call OBJS_0,$(SRCS))
	$(call BUILD_LIB_0,$(call LIBRARY_0,$(LIBNAME)),$(call OBJS_0,$(SRCS)))

$(call LIBRARY_1,$(LIBNAME)) : $(call OBJS_1,$(SRCS))
	$(call BUILD_LIB_1,$(call LIBRARY_1,$(LIBNAME)),$(call OBJS_1,$(SRCS)))

#
# dependancies 
#

$(call OBJS_0,$(SRCS)) $(call OBJS_1,$(SRCS)) : $(HDRS) makefile


#
# manage distributions
#

# Distribute the st40load_gdb script
DIST_IA32_LINUX_BIN = st40load_gdb

DIST_SRC = $(wildcard makefile *.c *.h *.mak rpctest.sh st40load.cfg ramfs.lst ramfs20.lst ramfs/etc/* ramfs/sbin/*)
include $(RPC_ROOT)/src/mkfiles/install.mak

# automatically generate the ramfs/etc directory (we cannot dive into
# this directory since the directory structure is copied directly into
# the generated ram disc.
$(DIR_SRC) : $(DIR_SRC)/ramfs/etc $(DIR_SRC)/ramfs/sbin
$(DIR_SRC)/ramfs/etc :
	-$(MKDIR) $(call DOSCMD,$(DIR_SRC)/ramfs/etc)
$(DIR_SRC)/ramfs/sbin :
	-$(MKDIR) $(call DOSCMD,$(DIR_SRC)/ramfs/sbin)

