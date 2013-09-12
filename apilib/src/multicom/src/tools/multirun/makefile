#
# Makefile
#
# Copyright (C) STMicroelectronics Limited 2001. All rights reserved.
#
# Makefile for multirun - a tool to start multi process tests.
#

DISABLE_EMBXSHELL=1

RPC_ROOT = ../../..
include $(RPC_ROOT)/src/mkfiles/host.mak

#
# configurable macros
#

APPLICATION=multirun

SRCS     = $(wildcard *.c)
HDRS     = $(wildcard *.h)

#
# derived macros
#

OBJS_IA32_LINUX    = $(foreach f,$(SRCS:.c=.$(OBJ_IA32_LINUX)),$(OBJDIR_IA32_LINUX)/$f)
OBJS_IA32_WINNT    = $(foreach f,$(SRCS:.c=.$(OBJ_IA32_WINNT)),$(OBJDIR_IA32_WINNT)/$f)
OBJS_SPARC_SOLARIS = $(foreach f,$(SRCS:.c=.$(OBJ_SPARC_SOLARIS)),$(OBJDIR_SPARC_SOLARIS)/$f)

$(OBJS_IA32_LINUX) $(OBJS_IA32_WINNT) $(OBJS_SPARC_SOLARIS) : $(HDRS) makefile

#
# include the build rules
#

include $(RPC_ROOT)/src/mkfiles/ia32_linux.mak
include $(RPC_ROOT)/src/mkfiles/ia32_winnt.mak
include $(RPC_ROOT)/src/mkfiles/sparc_solaris.mak

#
# explicit rules
#

all : $(MAKE_DIRS) $(MAKE_TARGETS)

clean:
	-$(RM) $(call DOSCMD,$(CLEAN_DIRS))

#
# manage distributions
#

DIST_SRC = makefile $(SRCS) $(HDRS)
include $(RPC_ROOT)/src/mkfiles/install.mak
