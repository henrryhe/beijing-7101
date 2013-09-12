#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the RPC benchmark test
#

TEST              = shared

ENABLE_RPC        = 1
NO_ST40_LINUX     = 1
ifdef ENABLE_EMBXSHMC
DISABLE_TEST      = 1
endif

RPC_TEST = ../..
include $(RPC_TEST)/harness/harness.mak
