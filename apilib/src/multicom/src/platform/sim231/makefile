#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2000, 01. All rights reserved.
#
# Makefile for the mailbox API shell 
#

RPC_ROOT = ../../..
include $(RPC_ROOT)/src/mkfiles/host.mak
include $(RPC_ROOT)/src/mkfiles/build.mak

#
# configurable macros
#

LIBRARY       = mailbox_sim231

SRCS          = mailbox.c

LOCAL_CFLAGS_IA32_LINUX     = -fpic
LDFLAGS_IA32_LINUX          = $(LOCAL_CFLAGS_IA32_LINUX) -shared

LOCAL_CFLAGS_SPARC_SOLARIS  = -fPIC
LDFLAGS_SPARC_SOLARIS       = $(LOCAL_CFLAGS_IA32_LINUX) -Xlinker -G -z allextract

#
# derived macros
#

OBJS_IA32_LINUX    = $(foreach f, $(SRCS:.c=.$(OBJ_IA32_LINUX)), $(OBJDIR_IA32_LINUX)/$f)
#OBJS_IA32_WINNT    = $(foreach f, $(SRCS:.c=.$(OBJ_IA32_WINNT)), $(OBJDIR_IA32_WINNT)/$f)
OBJS_SPARC_SOLARIS = $(foreach f, $(SRCS:.c=.$(OBJ_SPARC_SOLARIS)), $(OBJDIR_SPARC_SOLARIS)/$f)

#
# explicit rules
#

ifdef ENABLE_IA32_LINUX
TARGET_FOR_IA32_LINUX = $(OBJDIR_IA32_LINUX)/lib$(LIBRARY).so
DIST_IA32_LINUX_LIB  += $(TARGET_FOR_IA32_LINUX)

$(TARGET_FOR_IA32_LINUX) : $(OBJS_IA32_LINUX)
	$(LD_IA32_LINUX) -o $@ $(OBJS_IA32_LINUX) $(LDFLAGS_IA32_LINUX)
endif

ifdef ENABLE_SPARC_SOLARIS
TARGET_FOR_SPARC_SOLARIS = $(OBJDIR_SPARC_SOLARIS)/lib$(LIBRARY).so
DIST_SPARC_SOLARIS_LIB  += $(TARGET_FOR_SPARC_SOLARIS)

$(TARGET_FOR_SPARC_SOLARIS) : $(OBJS_SPARC_SOLARIS)
	$(LD_SPARC_SOLARIS) -o $@ $(OBJS_SPARC_SOLARIS) $(LDFLAGS_SPARC_SOLARIS)
endif

all: $(MAKE_DIRS) $(MAKE_TARGETS)

clean:
	-$(RM) $(call DOSCMD,$(CLEAN_DIRS))

#
# manage distributions
#
DIST_SRC = $(SRCS) mb411_video.txt mb411_audio.txt README makefile

include $(RPC_ROOT)/src/mkfiles/install.mak

