#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the RPC benchmark test
#


RPC_TEST = ..
include target.mak

#
# configurable macros
#

LIBNAME       = harness

SRCS          = harness.c \
                harness_os20.c \
		harness_os21.c \
		harness_linux.c \
		harness_wince.c \
		st_c1_timer_cc.c \
		stb5525_shm.c

HDRS          = harness.h

#
# explicit rules
#

all: $(MAKE_DIRS) lib_master lib_slave

lib_master : CONF_CFLAGS = -DCONF_MASTER
lib_master : $(call LIBRARY_0,$(LIBNAME))

lib_slave : CONF_CFLAGS = -DCONF_SLAVE
lib_slave : $(call LIBRARY_1,$(LIBNAME))

run : all

clean:
	-$(RM) $(call DOSCMD,$(CLEAN_DIRS) *.stubs.c *.stubs.cpped)

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
# macros to make distributions
#

DIST_SRC = $(wildcard makefile *.c *.h *.mak st40load.cfg ramfs.lst ramfs/etc/* ramfs/sbin/*)
include $(RPC_ROOT)/src/mkfiles/install.mak

# automatically generate the ramfs/etc directory (we cannot dive into
# this directory since the directory structure is copied directly into
# the generated ram disc.
$(DIR_SRC) : $(DIR_SRC)/ramfs/etc
$(DIR_SRC) : $(DIR_SRC)/ramfs/sbin
$(DIR_SRC)/ramfs/etc :
	-$(MKDIR) $(call DOSCMD,$(DIR_SRC)/ramfs/etc)
$(DIR_SRC)/ramfs/sbin :
	-$(MKDIR) $(call DOSCMD,$(DIR_SRC)/ramfs/sbin)

