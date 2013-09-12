#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for test directory
#

RPC_ROOT = ../../..

NO_SHLINUX_MASTER = 1

include $(RPC_ROOT)/src/mkfiles/host.mak

SUBDIRS = embxshell_tests              \
	  $(wildcard embxserial_tests) \
	  transport

include $(RPC_ROOT)/src/mkfiles/subdir.mak

DIST_SRC = makefile

include $(RPC_ROOT)/src/mkfiles/install.mak


