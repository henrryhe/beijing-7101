#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the MME host side
#

# directories in the RPC tree where we need to take code or headers from
COMMON_INCLUDES=../include
EMBX_INCLUDES=$(RPC_ROOT)/src/embx/include
vpath %.c ../companion

RPC_ROOT = ../../..

include $(RPC_ROOT)/src/mkfiles/host.mak

#
# configurable macros
#

LIBRARY     = mme_host

LOCAL_TRANSFORMER_SRCS = ExecutionLoop.c \
                         LocalTransformer.c \
                         LookupTable.c

LOCAL_SRCS =    $(LOCAL_TRANSFORMER_SRCS) \
		Manager.c \
                RemoteTransformer.c \
		mme_debug.c \
                mme_host_release.c

COMMON_SRCS =	MMEMessageQueue.c \
                mme_tune.c

SRCS        =   $(LOCAL_SRCS) $(COMMON_SRCS)

SRCS_LINUX_KERNEL = mme_linux_module.c $(SRCS)

SRCS_LINUX_USER = mme_linux_user.c $(LOCAL_TRANSFORMER_SRCS) $(COMMON_SRCS)

SRCS_LINUX  =   mme_linux_module.c mme_linux_user.c

LOCAL_HDRS =	mme_hostP.h \
		LookupTable.h \
		mme_linux.h

COMMON_HDRS = 	$(COMMON_INCLUDES)/mme_queueP.h \
		$(COMMON_INCLUDES)/mme_messagesP.h \
		$(COMMON_INCLUDES)/mme_debugP.h \
		$(COMMON_INCLUDES)/mmeP.h

HDRS = $(LOCAL_HDRS) $(COMMON_HDRS)

LOCAL_CFLAGS = -I$(COMMON_INCLUDES) -I$(EMBX_INCLUDES)

#
# derived macros
#

OBJS_IA32_LINUX    = $(foreach f,$(SRCS_LINUX_USER:.c=.$(OBJ_IA32_LINUX)), \
                                 $(OBJDIR_IA32_LINUX)/$f)
OBJS_ST20_OS20     = $(foreach f,$(SRCS:.c=.$(OBJ_ST20_OS20)), $(OBJDIR_ST20_OS20)/$f)
OBJS_ST40_OS21     = $(foreach f,$(SRCS:.c=.$(OBJ_ST40_OS21)), $(OBJDIR_ST40_OS21)/$f)
OBJS_ST40_LINUX    = $(foreach f,$(SRCS_LINUX_USER:.c=.$(OBJ_ST40_LINUX)), \
                                 $(OBJDIR_ST40_LINUX)/$f)
OBJS_ST40_LINUX_KO = $(foreach f,$(SRCS_LINUX_KERNEL:.c=.$(OBJ_ST40_LINUX_KO)), \
                                 $(OBJDIR_ST40_LINUX_KO)/$f)
OBJS_ST40_WINCE    = $(foreach f,$(SRCS:.c=.$(OBJ_ST40_WINCE)), $(OBJDIR_ST40_WINCE)/$f)
OBJS_ST220_OS21    = $(foreach f,$(SRCS:.c=.$(OBJ_ST220_OS21)), $(OBJDIR_ST220_OS21)/$f)
OBJS_ST231_OS21    = $(foreach f,$(SRCS:.c=.$(OBJ_ST231_OS21)), $(OBJDIR_ST231_OS21)/$f)

$(OBJS_ST20_OS20) $(OBJS_ST40_OS21) $(OBJS_ST40_LINUX_KO) $(OBJS_ST40_LINUX) \
$(OBJS_ST40_WINCE) $(OBJS_ST220_OS21) $(OBJS_ST231_OS21) : $(HDRS) makefile

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
DIST_SRC      = $(LOCAL_SRCS) $(SRCS_LINUX) $(LOCAL_HDRS) makefile

include $(RPC_ROOT)/src/mkfiles/install.mak

