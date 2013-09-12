#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2001
#
# Master makefile for the RPC test suite
#

SUBDIRS  = harness simple params api benchmark buffercontent local notify register limits

RPC_TEST = .
include $(RPC_TEST)/harness/subdir.mak

DIST_SRC = makefile 
include $(RPC_ROOT)/src/mkfiles/install.mak
