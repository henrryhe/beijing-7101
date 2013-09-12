#
# makefile
#
# Copyright (C) STMicroelectronics R&D Ltd. 2002
#
# Makefile for shared source subtree
#

RPC_ROOT = ../..

include $(RPC_ROOT)/src/mkfiles/host.mak
include $(RPC_ROOT)/src/mkfiles/platform.mak

SUBDIRS = $(wildcard cscope) $(wildcard rpc_server) rpc_userver $(wildcard rpc_tools) test

include $(RPC_ROOT)/src/mkfiles/subdir.mak

DIST_SRC       = makefile

include $(RPC_ROOT)/src/mkfiles/install.mak
