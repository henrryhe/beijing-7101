#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for embxshell_tests directory
#

RPC_ROOT = ../../../..

include $(RPC_ROOT)/src/mkfiles/host.mak
include $(RPC_ROOT)/src/mkfiles/platform.mak

SUBDIRS = framework_buffer         \
	  framework_connect        \
	  framework_connectblock   \
	  framework_crosstransport \
	  framework_enumerate      \
	  framework_initialise     \
	  framework_interruptopen  \
	  framework_lateregister   \
	  framework_object         \
	  framework_port           \
	  framework_recblock       \
	  framework_recmessage     \
	  framework_recobject      \
	  framework_sendmessage    \
	  framework_sendobject     \
	  framework_transport 


include $(RPC_ROOT)/src/mkfiles/subdir.mak

TEST_SRC     = test_main.c
TEST_SCRIPTS = run_st20_tests run_st40_tests run_st200_tests

DIST_SRC     = $(TEST_SRC) generic_test.mak makefile $(TEST_SCRIPTS)

include $(RPC_ROOT)/src/mkfiles/install.mak
