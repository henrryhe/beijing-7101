#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the mme companion library
#

COMMON_INCLUDES = ../include
EMBX_INCLUDES = $(RPC_ROOT)/src/embx/include

RPC_ROOT = ../../..
include $(RPC_ROOT)/src/mkfiles/host.mak

#
# configurable macros
#

LIBRARY = mme_companion

SRCS =		MMEManager.c \
		MMEMessageQueue.c \
		MMEReceiver.c \
		mme_tune.c \
                mme_companion_release.c

COMMON_HDRS = \
		$(COMMON_INCLUDES)/mmeP.h \
		$(COMMON_INCLUDES)/mme_debugP.h \
		$(COMMON_INCLUDES)/mme_messagesP.h \
		$(COMMON_INCLUDES)/mme_queueP.h

LOCAL_HDRS    = debug_ctrl.h \
		mme_companionP.h

HDRS = $(LOCAL_HDRS) $(COMMON_HDRS)

LOCAL_CFLAGS = -I$(COMMON_INCLUDES) -I$(EMBX_INCLUDES)

#
# derived macros
#

OBJS_ST20_OS20  = $(foreach f,$(SRCS:.c=.$(OBJ_ST20_OS20)), $(OBJDIR_ST20_OS20)/$f)
OBJS_ST40_OS21  = $(foreach f,$(SRCS:.c=.$(OBJ_ST40_OS21)), $(OBJDIR_ST40_OS21)/$f)
OBJS_ST220_OS21 = $(foreach f,$(SRCS:.c=.$(OBJ_ST220_OS21)), $(OBJDIR_ST220_OS21)/$f)
OBJS_ST231_OS21 = $(foreach f,$(SRCS:.c=.$(OBJ_ST231_OS21)), $(OBJDIR_ST231_OS21)/$f)

$(OBJS_ST20_OS20) $(OBJS_ST40_OS21) $(OBJS_ST220_OS21) $(OBJS_ST231_OS21) : $(HDRS) makefile

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
DIST_SRC      = $(SRCS) $(LOCAL_HDRS) makefile

include $(RPC_ROOT)/src/mkfiles/install.mak



















