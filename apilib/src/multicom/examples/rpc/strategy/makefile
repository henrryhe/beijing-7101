#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
#
# Makefile for RPC getstart example
#

# Ensure the RPC stripper is used to build this example.
ENABLE_RPCCC=1	

APP_MASTER  = caller
APP_SLAVE   = callee

STUB_MASTER = cllr
STUB_SLAVE  = clle

SRCS_MASTER = $(APP_MASTER).c $(STUB_MASTER).stubs.c
SRCS_SLAVE  = $(APP_SLAVE).c  $(STUB_SLAVE).stubs.c
SRC_IDL     = strategy.h

include ../../harness/harness.mak
