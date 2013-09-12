#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for embx subtree
#

RPC_ROOT = ../..

include $(RPC_ROOT)/src/mkfiles/host.mak

SUBDIRS = embxshell                \
	  embxmailbox              \
	  embxloopback             \
	  embxshm                  \
	  embxshmc                 \
	  $(wildcard embxdma)      \
	  $(wildcard embxserial)   \
	  include                  \
	  tests

include $(RPC_ROOT)/src/mkfiles/subdir.mak

DIST_SRC = makefile

include $(RPC_ROOT)/src/mkfiles/install.mak
