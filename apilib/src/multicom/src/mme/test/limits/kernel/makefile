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

SRCS_ALLOC = allocate.c
SRCS_LOCAL = simple_test.c local.c

#
# explicit rules
#

ifeq ($(OS_0),linux_ko)
all: $(MAKE_DIRS) app_alloc app_local
else
all:
endif

# *** Changed macros to STLinux2.2 style deprecates STLinux2.0 ***
#app_alloc : CONF_CFLAGS = -DKBUILD_MODNAME=allocate -DKBUILD_BASENAME=$(subst -,_,$(*F))
app_alloc : CONF_CFLAGS = -D"KBUILD_MODNAME=KBUILD_STR(allocate)"
app_alloc : $(call APPLICATION_0,allocate)
# *** Changed macros to STLinux2.2 style deprecates STLinux2.0 ***
#app_local : CONF_CFLAGS = -DKBUILD_MODNAME=local -DKBUILD_BASENAME=$(subst -,_,$(*F))
app_local : CONF_CFLAGS = -D"KBUILD_MODNAME=KBUILD_STR(local)"
app_local : $(call APPLICATION_0,local)

run : all

#
# macroized rules (don't try to understand this unless you have to)
#

$(call APPLICATION_0,allocate) : $(call OBJS_0,$(SRCS_ALLOC))
	$(call BUILD_APP_0,$(call APPLICATION_0,allocate),$(call OBJS_0,$(SRCS_ALLOC)))

$(call APPLICATION_0,local) : $(call OBJS_0,$(SRCS_LOCAL))
	$(call BUILD_APP_0,$(call APPLICATION_0,local),$(call OBJS_0,$(SRCS_LOCAL)))

#
# manage distributions
#

DIST_SRC = makefile
include $(RPC_ROOT)/src/mkfiles/install.mak
