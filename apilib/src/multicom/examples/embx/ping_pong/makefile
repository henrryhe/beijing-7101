#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2005. All rights reserved.
#
# Makefile for mailbox 'ping pong' test
#

ifdef PLATFORM
ifneq ($(PLATFORM),mb379)
ifneq ($(PLATFORM),mb392)
$(error This example can only be run on STm8000 platforms)
endif
endif
endif

APP_NAME   = ping_pong

APP_MASTER = $(APP_NAME)
APP_SLAVE  = $(APP_NAME)_aenc

SRCS_MASTER = $(APP_NAME)_master.c
SRCS_SLAVE  = $(APP_NAME)_slave.c

include ../../harness/harness.mak
