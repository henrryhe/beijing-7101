
########################################
#   File name: makefile
#   Description: makefile for eis_sh.lib on CHANGHONG 7100 platform
#   Author: zxg
#   Date:2007-04-24
#   COPYRIGHT (C) 2007 CHANGHONG
########################################

MODULE_NAME = eis
$(MODULE_NAME)_OBJ_DIR = $(MODULE_NAME).dir
$(MODULE_NAME)_PLATFORM_DIR = $(DVD_PLATFORM)_$(DVD_BACKEND)
include $(DVD_MAKE)/generic.mak
BDIR = $($(MODULE_NAME)_OBJ_DIR)$(SLASH)$($(MODULE_NAME)_PLATFORM_DIR)$(SLASH)
$(MODULE_NAME)_OBJECTS = eis_api_ca.tco eis_api_debug.tco eis_api_demux.tco eis_api_device.tco eis_api_file.tco eis_api_globe.tco eis_api_info.tco ipanel_socket.tco eis_api_media.tco eis_api_nvm.tco eis_api_os.tco eis_api_osd.tco eis_main.tco eis_api_msg.tco eis_api_upgrade.tco
$(MODULE_NAME)_OBJECTS += ipanel_graphics2.tco
BUILD_$(MODULE_NAME)_OBJECTS = $(addprefix $(BDIR), $($(MODULE_NAME)_OBJECTS))

SRC     = $(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURDIR))$(SLASH)
INCDIR  = -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURDIR))

INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_INCLUDE))
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_CONFIG)/chip)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/include)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_CONFIG)/platform/$(DVD_PLATFORM))
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/CHMID_TCPIP/changapp)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/CHMID_TCPIP/interfaces)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/CHMID_TCPIP/protocols)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/CHMID_TCPIP/Projects)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/CHMID_TCPIP/include)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/CHMID_TCPIP/applications)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/CHMID_TCPIP/utilities)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/CHDRV_NET/include)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/../apilib/include)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),eis_include)

C_CC_GLOBAL  = -c -DST_$(DVD_BACKEND) -DST_$(DVD_OS) -D$(DVD_PLATFORM) -D$(DVD_TRANSPORT) -D$(DVD_SERVICE) -DPROCESSOR_$(PROC_DEFINE) $(INCDIR) $(OPTIONAL_CFLAGS) $(DVD_CFLAGS)


ifeq "$(ARCHITECTURE)" "ST20"
C_CC_default  = $(PROCESSOR) -g -O $(C_CC_GLOBAL) 
endif
 
ifeq "$(ARCHITECTURE)" "ST40"
C_CC_default     = -mruntime=os21 -g -O0 $(C_CC_GLOBAL)
endif


all : create_directories $(BDIR)$(MODULE_NAME).lib copy_chlib

purge :
	@$(ECHO) Cleaning $(MODULE_NAME)...
	-@$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(BDIR))$(SLASH)*.tco
	-@$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(BDIR))$(SLASH)*.lib
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

$(BDIR)$(MODULE_NAME).lib :					   \
       $(BUILD_$(MODULE_NAME)_OBJECTS)
	@$(ECHO) Building $@...
ifeq "$(ARCHITECTURE)" "ST20"
	@$(AR) $^ -o $@
endif
ifeq "$(ARCHITECTURE)" "ST40"
	@$(AR) -rs $@ $^
endif


$(BUILD_$(MODULE_NAME)_OBJECTS): $(BDIR)\%.tco : $(SRC)\%.c
	$(CC) $<        -o $@		$(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	


copy_chlib:
	-@$(CP) $(BDIR)$(MODULE_NAME).lib $(CURR_DIR)\chlib\$(MODULE_NAME).lib

### END OF FILE ###


