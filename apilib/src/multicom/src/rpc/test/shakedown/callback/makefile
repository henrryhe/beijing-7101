#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the RPC benchmark test
#

TEST              = callback
ENABLE_RPC        = 1

# the master side of the test requires the OS20 API
NO_ST40_LINUX_MASTER = 1

RPC_LAYOUT        = callback.h

SRCS_MASTER       = master.c
SRCS_SLAVE        = slave.c

RPC_TEST = ../..
include $(RPC_TEST)/harness/harness.mak
