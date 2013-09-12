#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
#
# Makefile for the MME simple test.
#

RPC_TEST = ..
RPC_ROOT = ../../../..
SUBDIRS       = kernel

TEST          = limits

SRCS_MASTER   = host.c simple_test.c
SRCS_SLAVE    = companion.c simple_test.c

HDRS          = simple_test.h

ifneq ($(OS_0),linux_ko)
SRCS_MASTER  += allocate.c
LOCAL_ST40_MODULES = -m kernel/$(OBJDIR_ST40_LINUX)/allocate.ko \
                     -m kernel/$(OBJDIR_ST40_LINUX)/local.ko
endif

include $(RPC_ROOT)/src/mkfiles/subdir.mak
include $(RPC_TEST)/harness/harness.mak
