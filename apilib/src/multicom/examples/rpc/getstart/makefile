#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
#
# Makefile for RPC getstart example
#

# Ensure the RPC stripper is used to build this example.
ENABLE_RPCCC=1	

APP_MASTER  = application
APP_SLAVE   = cdplayer

STUB_MASTER = app
STUB_SLAVE  = cd

SRCS_MASTER = $(APP_MASTER).c $(STUB_MASTER).stubs.c
SRCS_SLAVE  = $(APP_SLAVE).c  $(STUB_SLAVE).stubs.c
SRC_IDL     = cdplayer.h

include ../../harness/harness.mak
