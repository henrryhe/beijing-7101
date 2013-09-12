#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
#
# Makefile for the MME simple test.
#

# SHLINUX_KERNEL=1        # build as a kernel module for SHLinux

TEST          = api

SRCS_MASTER   = host.c transformer.c streaming.c
SRCS_SLAVE    = companion.c transformer.c streaming.c

HDRS          = transformer.h streaming.h

RPC_TEST = ..
include $(RPC_TEST)/harness/harness.mak
