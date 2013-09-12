#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for embx examples directory
#

RPC_ROOT = ../..

include $(RPC_ROOT)/src/mkfiles/host.mak

SUBDIRS = $(wildcard mediaref_db547b_dma) \
          $(wildcard three_cpu_hello) \
	  $(wildcard buffer_pool) \
	  $(wildcard ping_pong)

include $(RPC_ROOT)/src/mkfiles/subdir.mak


DIST_SRC = makefile

include $(RPC_ROOT)/src/mkfiles/install.mak
