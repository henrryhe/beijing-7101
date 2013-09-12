# vi:noexpandtab
# ST20/ST40/ST200 makefile

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ifeq ($(DVD_HOST),unix)
LN	:= ln -sf
else
LN	:= $(CP)
endif

ifeq ($(DVD_OS),LINUX)
IMPORTS	:= stos stcommon
else
IMPORTS := stos stcommon stboot
endif

EXPORTS := 
TARGETS := net_test$(EXE_SUFFIX) IPtestx
HOST_TARGETS := IPtest

ifeq "$(DVD_OS)" "OS40" 
IMPORTS += os20emu
endif

# ST40
ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

# ST200
ifndef ST200_TARGETS
ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
ST200_IMPORTS = $(IMPORTS)
endif

CFLAGS := $(CFLAGS) -DSTTBX_REPORT -DSTTBX_PRINT -DDEBUG -g -Wall

# The test components to be built...

ifeq ($(DVD_OS),LINUX)
TEST_COMPS = sb_if
else
TEST_COMPS = 
endif

NET_TEST_OBJS := $(foreach i, $(TEST_COMPS), $(addprefix $i, $(OBJ_SUFFIX)))
NET_TEST_OBJS += $(DVD_BUILD_DIR)$(GOOD_SLASH)../src/objs/LINUX/stnet.o $(DVD_BUILD_DIR)$(GOOD_SLASH)../src/objs/LINUX/net_ioctl.o

IPTESTX_OBJS := $(addprefix IPtestx, $(OBJ_SUFFIX))


include $(DVD_MAKE)/defrules.mak

$(DVD_BUILD_DIR)$(GOOD_SLASH)../src/objs/LINUX/stnet.o $(DVD_BUILD_DIR)$(GOOD_SLASH)../src/objs/LINUX/net_ioctl.o:
	$(MAKE) -C $(DVD_BUILD_DIR)$(GOOD_SLASH)/../src

net_test$(EXE_SUFFIX): $(NET_TEST_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(EXTRA_LINKING_$(ARCHITECTURE)) 

IPtestx.c: IPtest.c
	$(LN) $< $@

$(IPTESTX_OBJS): IPtestx.c

IPtestx: $(IPTESTX_OBJS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

install: $(TARGETS)
	$(CP) $(TARGETS) $(ROOTFS_DIR)/root/bin
	sync

clean:
	@$(ECHO) Cleaning
	-$(RM) $(DVD_BUILD_DIR)$(GOOD_SLASH)$(HOST_TARGETS)
	-$(RM) $(DVD_BUILD_DIR)$(GOOD_SLASH)objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)$(GOOD_SLASH)*

# Local header dependencies

else

IPtest: IPtest.c
	@$(ECHO) Building $@
	$(CC) -Wall -g -o $@ $<

include $(DVD_MAKE)/builddir.mak

endif
