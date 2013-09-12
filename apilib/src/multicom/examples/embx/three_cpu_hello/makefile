#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
#
# Makefile for EMBX three cpu example
#

ifdef PLATFORM
ifneq ($(PLATFORM),mb379)
ifneq ($(PLATFORM),mb392)
$(error This example can only be run on STm8000 platforms)
endif
endif
endif

APP_NAME   = three_cpu_hello

APP_MASTER = $(APP_NAME)
APP_SLAVE  = $(APP_NAME)_aenc
APP_ADEC   = $(APP_NAME)_adec

SRCS_MASTER = $(APP_NAME)_master.c
SRCS_SLAVE  = $(APP_NAME)_slave.c

include ../../harness/harness.mak

# handle CPU 2 as a variant of CPU 1

example : app_adec

app_adec : BOARD=$(subst audioenc,audiodec,$(BOARD_1))
app_adec : $(call APPLICATION_1,$(APP_ADEC))

$(call APPLICATION_1,$(APP_ADEC)) : $(call OBJS_1,$(SRCS_SLAVE))
	@$(ECHO) +++ Link $(MACHINE_1) application for CPU 2 [$(call APPLICATION_1,$(APP_ADEC))] +++
	$(call BUILD_APP_1,$(call APPLICATION_1,$(APP_ADEC)),$(call OBJS_1,$(SRCS_SLAVE)))
