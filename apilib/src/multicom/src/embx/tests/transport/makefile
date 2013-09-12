#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2001
#
# Master makefile for the RPC test suite
#

SUBDIRS  = harness memory message object port benchmark

RPC_TEST = .
include $(RPC_TEST)/harness/subdir.mak

DIST_SRC = makefile README
include $(RPC_ROOT)/src/mkfiles/install.mak
