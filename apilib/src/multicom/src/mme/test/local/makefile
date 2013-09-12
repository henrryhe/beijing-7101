#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
#
# Makefile for the MME simple test.
#

RPC_TEST      = ..
RPC_ROOT      = ../../../..
SUBDIRS       = kernel

# Only for SHLINUX user

ifeq ($(OS_0),linux)

TEST        = local
SRCS_MASTER = simple.c
SRCS_SLAVE  = 
HDRS        = params.h

LOCAL_ST40_MODULES = -m $(SUBDIRS)/$(OBJDIR_ST40_LINUX)/transformer.ko

# No companion for these tests
DISABLE_SLAVE = 1

else # MASTER is in Linux User mode
# since TEST is undefined nothing much will happen!
endif # MASTER is in Linux User mode

include $(RPC_ROOT)/src/mkfiles/subdir.mak
include $(RPC_TEST)/harness/harness.mak
