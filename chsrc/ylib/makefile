##############################################
#   File name: makefile
#   Description: makefile for ylib.lib on CHANGHONG 7100 platform
#   Author: Yixiaoli
#   Date:2006-08-14
#   COPYRIGHT (C) 2006 CHANGHONG
##############################################

MODULE_NAME = ylib
$(MODULE_NAME)_OBJ_DIR = $(MODULE_NAME).dir
$(MODULE_NAME)_PLATFORM_DIR = $(DVD_PLATFORM)_$(DVD_BACKEND)
include $(DVD_MAKE)/generic.mak
BDIR = $($(MODULE_NAME)_OBJ_DIR)$(SLASH)$($(MODULE_NAME)_PLATFORM_DIR)$(SLASH)
 
#BUILD_$(MODULE_NAME)_OBJECTS = $(addprefix $(BDIR), $($(MODULE_NAME)_OBJECTS))
BUILD_$(MODULE_NAME)_OBJECTS =$(BDIR)chdemux.lib $(BDIR)graphicbase.lib $(BDIR)sectionbase.lib $(BDIR)channelbase.lib
##BUILD_$(MODULE_NAME)_OBJECTS =$(BDIR)chdemux.lib $(BDIR)sectionbase.lib
##BUILD_$(MODULE_NAME)_OBJECTS =$(BDIR)chdemux.lib

SRC     = $(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURDIR))$(SLASH)
INCDIR  = -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURDIR))
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_INCLUDE))
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_CONFIG)/chip)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/include)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_CONFIG)/platform/$(DVD_PLATFORM))

C_CC_GLOBAL  = -c -DST_$(DVD_BACKEND) -DST_$(DVD_OS) -D$(DVD_PLATFORM) -D$(DVD_TRANSPORT) -D$(DVD_SERVICE) -DPROCESSOR_$(PROC_DEFINE) $(INCDIR) $(OPTIONAL_CFLAGS) $(DVD_CFLAGS)

ifeq "$(ARCHITECTURE)" "ST20"
C_CC_default  = $(PROCESSOR) -g -O0 $(C_CC_GLOBAL) 
endif
 
ifeq "$(ARCHITECTURE)" "ST40"
C_CC_default     = -mruntime=os21 -g -O0 $(C_CC_GLOBAL)
endif


all : create_directories  $(BDIR)chdemux.lib  $(BDIR)sectionbase.lib $(BDIR)$(MODULE_NAME).lib copy_chlib
##      $(BDIR)$(MODULE_NAME).lib $(BDIR)graphicbase.lib $(BDIR)channelbase.lib 
##

create_directories : 
	-@$(MAKE) $($(MODULE_NAME)_OBJ_DIR) --quiet --ignore-errors
	-@$(MAKE) $($(MODULE_NAME)_PLATFORM_DIR) --quiet --ignore-errors

$(BDIR)graphicbase.tco : $(SRC)graphicbase.c 
	$(CC) $<        -o $@		$(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	

##$(BDIR)vidinj.tco : $(SRC)vidinj.c 
##	$(CC) $<        -o $@		$(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	

$(BDIR)CH_IFrameDisplay.tco : $(SRC)CH_IFrameDisplay.c 
	$(CC) $<        -o $@		$(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	



$(BDIR)graphicbase.lib :					   \
       $(BDIR)graphicbase.tco 
 ##      $(BDIR)graphicbase.tco $(BDIR)CH_IFrameDisplay.tco $(BDIR)CH_AudeoPlay.tco
	@$(ECHO) Building $@...
ifeq "$(ARCHITECTURE)" "ST20"
	@$(AR) $^ -o $@
endif
ifeq "$(ARCHITECTURE)" "ST40"
	@$(AR) -rs $@ $^
endif




$(BDIR)chdemux.tco : $(SRC)chdemux.c 
	$(CC) $<        -o $@		$(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	

$(BDIR)chdemux.lib :					   \
       $(BDIR)chdemux.tco
	@$(ECHO) Building $@...
ifeq "$(ARCHITECTURE)" "ST20"
	@$(AR) $^ -o $@
endif
ifeq "$(ARCHITECTURE)" "ST40"
	@$(AR) -rs $@ $^
endif


$(BDIR)sectionbase.tco : $(SRC)sectionbase.c 
	$(CC) $<        -o $@		$(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	

$(BDIR)sectionbase.lib :					   \
       $(BDIR)sectionbase.tco
	@$(ECHO) Building $@...
ifeq "$(ARCHITECTURE)" "ST20"
	@$(AR) $^ -o $@
endif
ifeq "$(ARCHITECTURE)" "ST40"
	@$(AR) -rs $@ $^
endif



$(BDIR)channelbase.tco : $(SRC)channelbase.c 
	$(CC) $<        -o $@		$(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	

$(BDIR)channelbase.lib :\
	$(BDIR)channelbase.tco	$(BDIR)CH_IFrameDisplay.tco			   
##       $(BDIR)channelbase.tco
	@$(ECHO) Building $@...
ifeq "$(ARCHITECTURE)" "ST20"
	@$(AR) $^ -o $@
endif
ifeq "$(ARCHITECTURE)" "ST40"
	@$(AR) -rs $@ $^
endif



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



$(BDIR)$(MODULE_NAME).lib :					   \
       $(BUILD_$(MODULE_NAME)_OBJECTS) 
	@$(ECHO) Building $@...
ifeq "$(ARCHITECTURE)" "ST20"
	@$(AR) $^ -o $@
endif
ifeq "$(ARCHITECTURE)" "ST40"
##	@$(AR) -rs $@ $^
	@$(AR) -rs $@ $(BDIR)/*.tco
endif



##$(BUILD_$(MODULE_NAME)_OBJECTS): $(BDIR)\%.tco : $(SRC)\%.c
##	$(CC) $<        -o $@		$(if $(C_CC_$(basename $(notdir $@))),$(C_CC_$(basename $(notdir $@))), $(C_CC_default))	

copy_chlib:
	-@$(CP) $(BDIR)$(MODULE_NAME).lib $(CURR_DIR)\chlib\$(MODULE_NAME).lib
	-@$(CP) $(BDIR)chdemux.lib  $(CURR_DIR)\chlib\chdemux.lib
	-@$(CP) $(BDIR)sectionbase.lib $(CURR_DIR)\chlib\sectionbase.lib
	-@$(CP) $(BDIR)channelbase.lib $(CURR_DIR)\chlib\channelbase.lib
	-@$(CP) $(BDIR)graphicbase.lib $(CURR_DIR)\chlib\graphicbase.lib
##	-@$(CP) $(SRC)\*.h $(CURR_DIR)\include\*.h
##	-@$(CP) *.h ..\include\*.h


### End of file ###