########################################
#   File name: makefile
#   Description: makefile for dbase.lib on CHANGHONG 7100 platform
#   Author: Yixiaoli
#   Date:2006-08-07
#   COPYRIGHT (C) 2006 CHANGHONG
########################################

MODULE_NAME =ch_pic
$(MODULE_NAME)_OBJ_DIR = $(MODULE_NAME).dir
$(MODULE_NAME)_PLATFORM_DIR = $(DVD_PLATFORM)_$(DVD_BACKEND)
include $(DVD_MAKE)/generic.mak
BDIR = $($(MODULE_NAME)_OBJ_DIR)$(SLASH)$($(MODULE_NAME)_PLATFORM_DIR)$(SLASH)
$(MODULE_NAME)_OBJECTS = ckconfig.tco draw_gif.tco drawjpeg.tco jcapimin.tco jcapistd.tco jccoefct.tco jccolor.tco jcdctmgr.tco jchuff.tco jcinit.tco jcmainct.tco jcmarker.tco\
			jcmaster.tco jcomapi.tco jcparam.tco jcphuff.tco jcprepct.tco jcsample.tco jctrans.tco\
			jdapimin.tco jdapistd.tco jdatadst.tco jdatasrc.tco jdcoefct.tco jdcolor.tco jddctmgr.tco\
			jdhuff.tco jdinput.tco jdmainct.tco jdmarker.tco jdmaster.tco jdmerge.tco jdphuff.tco\
			jdpostct.tco jdsample.tco jdtrans.tco jerror.tco jfdctflt.tco jfdctfst.tco jfdctint.tco\
			jidctflt.tco jidctfst.tco jidctint.tco jidctred.tco jmemmgr.tco jmemnobs.tco jquant1.tco\
			jquant2.tco jutils.tco rdbmp.tco rdcolmap.tco rdgif.tco rdppm.tco rdrle.tco rdswitch.tco\
			rdtarga.tco transupp.tco wrbmp.tco graphcom.tco drawbmp.tco br_file.tco j_osd.tco


BUILD_$(MODULE_NAME)_OBJECTS = $(addprefix $(BDIR), $($(MODULE_NAME)_OBJECTS))


SRC     = $(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURDIR))$(SLASH)
INCDIR  = -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURDIR))

INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_INCLUDE))
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_CONFIG)/chip)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(CURR_DIR)/include)
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_CONFIG)/platform/$(DVD_PLATFORM))
INCDIR += -I$(subst $(BAD_SLASH),$(GOOD_SLASH),$(OSPLUSROOT)/include) 

C_CC_GLOBAL  = -c -DST_$(DVD_BACKEND) -DST_$(DVD_OS) -D$(DVD_PLATFORM) -D$(DVD_TRANSPORT) -D$(DVD_SERVICE) -DPROCESSOR_$(PROC_DEFINE) $(INCDIR) $(OPTIONAL_CFLAGS) $(DVD_CFLAGS)


ifeq "$(ARCHITECTURE)" "ST20"
C_CC_default  = $(PROCESSOR) -g -O0 $(C_CC_GLOBAL) 
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

