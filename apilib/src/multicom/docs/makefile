#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for docs directory
#

RPC_ROOT = ..

include $(RPC_ROOT)/src/mkfiles/host.mak
include $(RPC_ROOT)/src/mkfiles/subdir.mak

DIST_SRC = license.pdf \
           makefile \
	   release.html \
	   userman.pdf \
	   $(DIST_SRC_COLLECT)

ifeq (collect.mak,$(wildcard collect.mak))
include collect.mak
endif

include $(RPC_ROOT)/src/mkfiles/install.mak
