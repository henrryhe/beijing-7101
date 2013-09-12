#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2005. All rights reserved.
#
# Build a Linux kernel module to help with the local test
#

vpath %.c ../
vpath %.h ../
vpath %.mak ../

ifeq ($(OS_0),linux)
override OS_0 = linux_ko
endif

RPC_TEST = ../..
include $(RPC_TEST)/harness/target.mak

#
# configurable macros
# 

SRCS = simple_test.c
HDRS = params.h

#
# explicit rules
#

ifeq ($(OS_0),linux_ko)
all: $(MAKE_DIRS) app_master
else
all:
endif

# *** Changed macros to STLinux2.2 style deprecates STLinux2.0 ***
#app_master : CONF_CFLAGS = -DKBUILD_MODNAME=transformer -DKBUILD_BASENAME=$(subst -,_,$(*F))
app_master : CONF_CFLAGS = -D"KBUILD_MODNAME=KBUILD_STR(transformer)"
app_master : $(call APPLICATION_0,transformer)

run : all

#
# macroized rules (don't try to understand this unless you have to)
#

$(call APPLICATION_0,transformer) : $(call OBJS_0,$(SRCS))
	$(call BUILD_APP_0,$(call APPLICATION_0,transformer),$(call OBJS_0,$(SRCS)))

#
# manage distributions
#

DIST_SRC = makefile
include $(RPC_ROOT)/src/mkfiles/install.mak
