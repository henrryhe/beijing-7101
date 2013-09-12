#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for embx subtree
#

RPC_ROOT = ../..

include $(RPC_ROOT)/src/mkfiles/host.mak

SUBDIRS = include host companion test

include $(RPC_ROOT)/src/mkfiles/subdir.mak

# TODO: this line prevents any of MME from being copied into
#       a customer distribution, this is to prevent contamination
#       of the RPC/EMBX R2.2 release.
DISABLE_INSTALL = 1

DIST_SRC = makefile

include $(RPC_ROOT)/src/mkfiles/install.mak
