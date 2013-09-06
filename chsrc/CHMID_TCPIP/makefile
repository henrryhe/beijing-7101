#----------------------------------------------------------------------
#
#           Makefile for SINGAPPL TUNER
#
# Build options:
#
# Make options:
#   'make'              builds lib file.
#
#   'make clean'        remove lib build files.
#
#----------------------------------------------------------------------


MODULE_NAME =CHMID_TCPIP
$(MODULE_NAME)_OBJ_DIR = $(MODULE_NAME).dir
$(MODULE_NAME)_PLATFORM_DIR = $(DVD_PLATFORM)_$(DVD_BACKEND)
include $(DVD_MAKE)/generic.mak
BDIR = $($(MODULE_NAME)_OBJ_DIR)$(SLASH)$($(MODULE_NAME)_PLATFORM_DIR)$(SLASH)

BUILD_$(MODULE_NAME)_OBJECTS = $(addprefix $(BDIR), $($(MODULE_NAME)_OBJECTS))
SRC     = $(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURDIR))$(SLASH)

INCDIR  = -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURDIR))
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(PRJROOT)/chsrc/include)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),$(PRJROOT)/chsrc/main)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),$(PRJROOT)/chsrc/dbase)

INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),changapp)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),interfaces)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),protocols)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),utilities)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),Projects)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),include)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),applications)
INCDIR += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../CHDRV_NET/include)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(OSPLUSROOT)/include) 

#----------------------------------------------------------------------
# set virtual paths to objects (as path from test application ./objs/ST20 subdirectory)

vpath %.c 	$(CURDIR)/interfaces
vpath %.c    	$(CURDIR)/protocols
vpath %.c     	$(CURDIR)/utilities
vpath %.c     	$(CURDIR)/changapp
vpath %.c 	$(CURDIR)/Projects

vpath %.h 	$(CURDIR)/include
vpath %.h 	$(CURDIR)/Projects
vpath %.h 	$(CURDIR)/applications
vpath %.h    	$(CURDIR)/protocols
vpath %.h     	$(CURDIR)/utilities
vpath %.h 	$(CURDIR)/interfaces
vpath %.h     	$(CURDIR)/changapp

$(MODULE_NAME)_OBJECTS = loop_back.tco ethernet.tco icmpv4.tco udp.tco ipv4.tco \
tcp.tco dhcp.tco  network_stack.tco sockets.tco  memory_pool.tco  smsc_threading.tco \
packet_manager.tco task_manager.tco CHMID_TCPIP_api.tco CHMID_TCPIP_IO.tco tcpip.tco

C_CC_GLOBAL  = -c -DST_$(DVD_BACKEND) -DST_$(DVD_OS) -D$(DVD_PLATFORM) -D$(DVD_TRANSPORT) -D$(DVD_SERVICE) -DPROCESSOR_$(PROC_DEFINE) $(INCDIR) $(OPTIONAL_CFLAGS) $(DVD_CFLAGS)


ifeq "$(ARCHITECTURE)" "ST20"
C_CC_default = $(PROCESSOR) -g -O0 $(C_CC_GLOBAL) 
endif
 
ifeq "$(ARCHITECTURE)" "ST40"
C_CC_default = -mruntime=os21 -g -O0 $(C_CC_GLOBAL)
endif


all : create_directories $(BDIR)$(MODULE_NAME).lib copy_chlib

purge :
	@$(ECHO) Cleaning $(MODULE_NAME)...
	-@$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(BDIR))$(SLASH)*.tco
	-@$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(BDIR))$(SLASH)*.lib
	-@$(RM) $(CURR_DIR)\chlib\$(MODULE_NAME).lib
	-@$(RM) $(CURR_DIR)\include\CHMID_TCPIP_api.h
	-@$(RM) *.bak
    ifneq "$(wildcard $(BDIR))" ""
		-@$(RMDIR) $(BDIR)
    endif
    ifneq "$(wildcard $($(MODULE_NAME)_OBJ_DIR))" ""
		-@$(RMDIR) $($(MODULE_NAME)_OBJ_DIR)
    endif

$($(MODULE_NAME)_OBJ_DIR) :
	-@$(MKDIR) $($(MODULE_NAME)_OBJ_DIR)
	
$($(MODULE_NAME)_PLATFORM_DIR) :
	-@$(MKDIR) $($(MODULE_NAME)_OBJ_DIR)$(SLASH)$($(MODULE_NAME)_PLATFORM_DIR)

create_directories : 
	-@$(MAKE) $($(MODULE_NAME)_OBJ_DIR) --quiet --ignore-errors
	-@$(MAKE) $($(MODULE_NAME)_PLATFORM_DIR) --quiet --ignore-errors
	-@$(CP) $(CURR_DIR)\CHMID_TCPIP\include\CHMID_TCPIP_api.h $(CURR_DIR)\include\CHMID_TCPIP_api.h

$(BDIR)$(MODULE_NAME).lib :					   \
       $(BUILD_$(MODULE_NAME)_OBJECTS)
	@$(ECHO) Building $@...
ifeq "$(ARCHITECTURE)" "ST20"
	@$(AR) $^ -o $@
endif
ifeq "$(ARCHITECTURE)" "ST40"
	@$(AR) -rs $@ $^
endif


$(BUILD_$(MODULE_NAME)_OBJECTS): $(BDIR)\%.tco : $(vpath)%.c##$(SRC)\%.c
	$(CC) $< -o $@ $(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	


copy_chlib:
	-@$(CP) $(BDIR)$(MODULE_NAME).lib $(CURR_DIR)\chlib\$(MODULE_NAME).lib


###################################
# Dependency Generator Target
# by Louie
# run "gmake dep"
# to auto create a dependency check
# file for this module
###################################
-include ../../depends.mak
OBJS:=$(ST20_OBJS)
dep: 
	@$(ECHO) Generating dependencies file depends.mak
	-$(RM) $(OBJS)	
	$(MAKE) -C ../.. $(OBJS) CFLAGS="$(CFLAGS) -v -C" > compile.log
	depends $(@D)/compile.log $(@D)/../../depends.mak
	-$(RM) $(OBJS) compile.log
####### DEPENDS.MAK ###############



# end
