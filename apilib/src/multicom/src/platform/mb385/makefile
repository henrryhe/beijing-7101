#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
#
# Replacement OS21 board support for MB385.
#

RPC_ROOT = ../../..
include $(RPC_ROOT)/src/mkfiles/host.mak

#
# configurable macros
#

LIBRARY       = rpc_mb385

SRCS          = rpc_mb385.c
HDRS          = rpc_mb385.h

#
# derived macros
#

OBJS_ST220_OS21 = $(foreach f,$(SRCS:.c=.$(OBJ_ST220_OS21)),$(OBJDIR_ST220_OS21)/$f)

$(OBJS_ST220_OS21) : $(HDRS) makefile

#
# include the build rules
#

include $(RPC_ROOT)/src/mkfiles/st220_os21.mak

#
# explicit rules
#

all: $(MAKE_DIRS) $(MAKE_TARGETS)

clean:
	-$(RM) $(call DOSCMD,$(CLEAN_DIRS))

#
# manage distributions
#

DIST_SRC            = $(SRCS) $(HDRS) makefile
DIST_INCLUDE        = $(HDRS)

include $(RPC_ROOT)/src/mkfiles/install.mak
