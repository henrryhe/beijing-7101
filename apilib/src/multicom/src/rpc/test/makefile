#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2001
#
# Master makefile for the RPC test suite
#

SUBDIRS    = harness shakedown $(wildcard regression) benchmark

LOCAL_DIST_SRC = $(wildcard ramfs.lst ramfs/etc/* st40load.cfg)

RPC_TEST   = .
include $(RPC_TEST)/harness/subdir.mak

