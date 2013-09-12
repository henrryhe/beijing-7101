#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the RPC benchmark test
#


TEST              = benchmark
ENABLE_RPC        = 1
NO_ST40_LINUX_MASTER = 1		# Not on Linux at the moment INSbl22887

RPC_LAYOUT        = benchmark.h

SRCS_MASTER       = master.c
SRCS_SLAVE        = slave.c

HDRS              = $(RPC_LAYOUT)

RPC_TEST = ..
include $(RPC_TEST)/harness/harness.mak

