#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for shared subtree
#

RPC_ROOT = ..

include $(RPC_ROOT)/src/mkfiles/host.mak
include $(RPC_ROOT)/src/mkfiles/platform.mak

# Note the order of the subdir make is important,
# particularly platform support should be made
# before embx, to ensure platform header files
# are available to that build.
SUBDIRS = mkfiles tools platform embx rpc $(wildcard mme)

include $(RPC_ROOT)/src/mkfiles/subdir.mak

DIST_SRC       = makefile

include $(RPC_ROOT)/src/mkfiles/install.mak

