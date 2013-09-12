#
# makefile
#
# Copyright (C) STMicroelectronics R&D Ltd. 2002
#
# Makefile for rpc_userver
#

RPC_ROOT = ../../..
include $(RPC_ROOT)/src/mkfiles/host.mak

#
# configurable macros
#

LIBRARY = rpc_userver

SRCS = rpc_userver.c

HDRS = rpc_userver.h

#
# derived macros
#

OBJS_ST40_LINUX_KO = $(foreach f,$(SRCS:.c=.$(OBJ_ST40_LINUX_KO)),$(OBJDIR_ST40_LINUX_KO)/$f)

$(OBJS_ST40_LINUX_KO) : $(HDRS)

#
# explicit rules
#

include $(RPC_ROOT)/src/mkfiles/build.mak

all: $(MAKE_DIRS) $(MAKE_TARGETS)

clean:
	-$(RM) $(call DOSCMD,$(CLEAN_DIRS))

#
# manage distributions
#

DIST_SRC      = $(SRCS) $(HDRS) makefile
DIST_INCLUDE  = $(HDRS)

include $(RPC_ROOT)/src/mkfiles/install.mak
