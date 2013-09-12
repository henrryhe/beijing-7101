#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the RPC benchmark test
#

TEST              = string
ENABLE_RPC        = 1

RPC_LAYOUT        = str.h

SRCS_MASTER       = master.c utils.c
SRCS_SLAVE        = slave.c  utils.c

RPC_TEST = ../..
include $(RPC_TEST)/harness/harness.mak
