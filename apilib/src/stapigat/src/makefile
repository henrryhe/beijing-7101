#########################################
# MODULE : stapigat.lib / libstapigat.a #
#########################################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ifeq "$(DVD_OS)" "LINUX"
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/linux
endif

# Dependencies
HEADER_IMPORTS := $(STAPIGAT_INCLUDE)
ifeq "$(DVD_OS)" "LINUX"
HEADER_IMPORTS += stavmem/linux
endif

# Remove stapigat from imports
#-----------------------------
ST20_IMPORTS := $(filter-out stapigat,$(STAPIGAT_IMPORTS_ST20))
ST40_IMPORTS := $(filter-out stapigat,$(STAPIGAT_IMPORTS_ST40))
ST200_IMPORTS := $(filter-out stapigat,$(STAPIGAT_IMPORTS_ST200))


# Define the targets
#-------------------
ST20_TARGETS := stapigat.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

# Implicite objs for ST20
# -----------------------
ST20_OBJS := api_cmd.tco
ifneq "$(DVD_OS)" "LINUX"
ST20_OBJS += clboot.tco
endif
ST20_OBJS += disp_gam.tco disp_osd.tco disp_vid.tco
ST20_OBJS += stemrst.tco scartswt.tco simvout.tco simvtg.tco startup.tco
ST20_OBJS += trace.tco tt_task.tco

FRONTEND_USE_GDM = 5100 5105 5301 5188 5525 5107
ifeq "$(findstring $(DVD_FRONTEND), $(FRONTEND_USE_GDM))" "$(DVD_FRONTEND)"
        ST20_OBJS += disp_gdm.tco
else
        ST20_OBJS += disp_gam.tco
endif

BACKEND_HAS_LX_COMPANION = 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_HAS_LX_COMPANION))" "$(DVD_BACKEND)"
  ifneq "$(findstring stvid,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
        ST20_OBJS += lx_loader.tco
            CFLAGS := $(CFLAGS) -DMULTICOM
            INCLUDE_PATH += $(RPC_ROOT)/include
            INCLUDE_PATH += $(RPC_ROOT)/src/mme/include
            INCLUDE_PATH += $(RPC_ROOT)/src/embx/include
            FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/$(ARCHITECTURE)
  endif
endif



# Implicite objs for ST40
# -----------------------
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ, $(ST20_OBJS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ, $(ST20_OBJS))


# Implicite objs for ST200
# -----------------------
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ, $(ST20_OBJS))

# Retrieve ST20 objs for actual ARCHITECTURE
# ------------------------------------------
OTHER_OBJS :=
ifneq "$(DVD_OS)" "LINUX"
ifneq "$(findstring stboot,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS +=  clboot.tco
endif
ifneq "$(findstring stavmem,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clavmem.tco
endif
endif
ifneq "$(findstring stcfg,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clcfg.tco
endif
ifneq "$(findstring stclkrv,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clclkrv.tco
endif
ifneq "$(findstring stevt,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clevt.tco
endif
ifneq "$(findstring stextvin,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += evin_cmd.tco
endif
ifneq "$(findstring stgpdma,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clgpdma.tco
endif
ifneq "$(findstring stfdma,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clfdma.tco
endif
ifneq "$(findstring sti2c,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += cli2c.tco
endif
ifneq "$(findstring stintmr,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clintmr.tco
endif
ifneq "$(findstring stpio,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clpio.tco
endif
ifneq "$(findstring stpwm,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += clpwm.tco
endif
ifneq "$(findstring sttbx,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += cltbx.tco
endif
ifneq "$(findstring stuart,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += cluart.tco
endif
ifneq "$(findstring testtool,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += cltst.tco
endif
ifneq "$(findstring stdenc,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += denc_cmd.tco
endif
ifneq "$(findstring stlayer,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += lay_cmd.tco
endif
ifneq "$(findstring stttx,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += ttx_cmd.tco
endif
ifneq "$(findstring sttuner,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += tune_cmd.tco
endif
ifneq "$(findstring sttsmux,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += cltsmux.tco
endif
ifneq "$(findstring stvbi,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += vbi_cmd.tco
endif
ifneq "$(findstring stvout,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += vout_cmd.tco
endif
ifneq "$(findstring stvin,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += vin_cmd.tco
endif
ifneq "$(findstring stvmix,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += vmix_cmd.tco
endif
ifneq "$(findstring stvtg,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += vtg_cmd.tco
endif
ifneq "$(findstring stgfb,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
	INCLUDE_PATH += $(ROOTFS_DIR)/usr/include/directfb
	OTHER_OBJS += gfb_cmd.tco
endif
ifneq "$(findstring stvid,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += vid_cmd.tco vid_util.tco vid_inj.tco
    ifeq "$(STVID_USE_BLIT)" "TRUE"
            OTHER_OBJS += vid_blit.tco
    endif
endif
ifneq "$(findstring staud,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
      ifeq "$(DVD_BACKEND)" "7710"
         OTHER_OBJS += aud_cmd.tco
      endif
endif

ifneq "$(findstring staudlx,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
        OTHER_OBJS += audlx_cmd.tco
endif


ifneq "$(findstring stblit,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += blt_cmd.tco blt_debug.tco
endif


ifneq "$(findstring compo,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += compo_cmd.tco
endif

ifneq "$(findstring stdisp,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += disp_cmd.tco
endif

ifneq "$(findstring sthdmi,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    OTHER_OBJS += hdmi_cmd.tco
endif

ifneq ($(DVD_SECURED_CHIP),)
    OTHER_OBJS += clmes.tco
endif

FRONTEND_USE_TTXCMD = 7100 5528 5301 7109
ifeq "$(findstring $(DVD_FRONTEND), $(FRONTEND_USE_TTXCMD))" "$(DVD_FRONTEND)"
         OTHER_OBJS += ttx_cmd.tco
endif

ifeq ($(ARCHITECTURE),ST40)
    LINUX_OBJS += $(call ST20OBJ_TO_LINUXOBJ, $(OTHER_OBJS))
        ST40_OBJS += $(call ST20OBJ_TO_ST40OBJ, $(OTHER_OBJS))
else
    ST20_OBJS += $(OTHER_OBJS)
endif

ifeq ($(DVD_TRANSPORT),link)
    ST20_OBJS += link_cmd.tco
else
    ifeq ($(DVD_TRANSPORT), demux)
        ST20_OBJS += demux_cmd.tco
    else
        ST20_OBJS += pti_cmd.tco
    endif
endif

ST40_OBJS += $(call ST20OBJ_TO_ST40OBJ, $(ST20_OBJS))
ST200_OBJS += $(call ST20OBJ_TO_ST200OBJ, $(ST20_OBJS))
LINUX_OBJS += $(call ST20OBJ_TO_LINUXOBJ, $(ST20_OBJS))

# Set special interconnect configuration for STi5105 with SDRAM platforms
ifeq "$(DVD_BACKEND)" "5105"
	ifeq "$(SDRAM_DEVICE_TYPE)" "SDR_SDRAM"
	CFLAGS := $(CFLAGS) -DSELECT_DEVICE_STB5118
      endif
endif

# For compilation of pti+link or stpti
CFLAGS += $(OPTIONAL_CFLAGS) -DSTB_FRONTEND_$(DVD_FRONTEND) -DSTB_BACKEND_$(DVD_BACKEND)
FLAGS_FOR_SUBDIRS := -DSTB_FRONTEND_$(DVD_FRONTEND) -DSTB_BACKEND_$(DVD_BACKEND)

ifdef STUB_INJECT
CFLAGS += -DSTUB_INJECT
endif

ifdef USE_VTGSIM
    CFLAGS += -DUSE_VTGSIM
endif

# For compilation of the function STAVMEM_GetMemoryStatus()
# CFLAGS += $(OPTIONAL_CFLAGS) -D STAVMEM_DEBUG_MEMORY_STATUS

BACKEND_USE_CLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_USE_CLKRV))" "$(DVD_BACKEND)"
  CFLAGS += -DSTVTG_USE_CLKRV
endif

BACKEND_USE_DISP = 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_USE_DISP))" "$(DVD_BACKEND)"
  ifneq ($(USE_DISPVID),)
  CFLAGS += -DUSE_DISPVID
  endif
endif

ifeq ($(DVD_BACKEND),7710)
  ifeq "$(STI7710_CUT2x)" "TRUE"
     CFLAGS += -DSTI7710_CUT2x
  endif
endif

ifdef STVID_USE_FGT
  CFLAGS += -DSTVID_USE_FGT -DST_XVP_ENABLE_FLEXVP -DST_XVP_ENABLE_FGT 
endif

ifneq (,$(findstring $(DVD_BACKEND),7100 7109 ))
ifdef WA_GNBvd54182
    CFLAGS +=-DWA_GNBvd54182
endif
endif


# This Flag is used just for the outputsatge driver to avoid the dependency with stpti /STi7710

ifneq ($(STPTI_NUSED),)
  CFLAGS += -DSTPTI_NUSED
endif
# Generic address removed for multi-chip platforms
ifneq ($(DVD_FRONTEND),$(DVD_BACKEND))
    MULTI_CHIP_PLATFORMS := mb290 mb314 mb382 mb376
    ifneq "$(findstring $(DVD_PLATFORM),$(MULTI_CHIP_PLATFORMS))" ""
        CFLAGS := $(CFLAGS) -DREMOVE_GENERIC_ADDRESSES
        FLAGS_FOR_SUBDIRS := $(FLAGS_FOR_SUBDIRS) -DREMOVE_GENERIC_ADDRESSES
    endif
endif

# for compilation with new STVIN.
ifeq ($(DVD_STVIN),MASTER)
    CFLAGS += -D ST_MasterDigInput
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
endif

# Enable Security features if DVD_SECURED_CHIP is set.
ifneq ($(DVD_SECURED_CHIP),)
    CFLAGS += -D DVD_SECURED_CHIP
    FLAGS_FOR_SUBDIRS := $(FLAGS_FOR_SUBDIRS) -D DVD_SECURED_CHIP
endif

# Additional components in sub-directories
#-----------------------------------------
SUBDIRS :=
ifneq "$(findstring dumpreg,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    SUBDIRS += dumpreg
endif

ifneq "$(findstring stextvin,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    SUBDIRS += stextvin
endif

ifeq "$(DVD_OS)" "LINUX"
# Compile user wrapping file under linux directory
SUBDIRS += linux
endif

ifneq "$(findstring stblit,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
    SUBDIRS += snake
endif

# Patch sub-directory
#--------------------
# Add condition here for patch
SUBDIRS += patch
ifneq "$(findstring patch,$(SUBDIRS))" ""
    OBJS20_IN_PATCH :=
ifneq "$(DVD_OS)" "LINUX"
 ifeq "$(STVID_USE_BLIT)" "TRUE"
     OBJS20_IN_PATCH := $(OBJS20_IN_PATCH) blt_cmd.tco
 endif
    endif

    OBJS20_IN_PATCH := $(OBJS20_IN_PATCH) denc_cmd.tco clavmem.tco
    
    OBJSLINUX_IN_PATCH := $(call ST20OBJ_TO_LINUXOBJ, $(OBJS20_IN_PATCH))

    OBJS40_IN_PATCH := $(call ST20OBJ_TO_ST40OBJ, $(OBJS20_IN_PATCH))

    OBJS200_IN_PATCH := $(call ST20OBJ_TO_ST200OBJ, $(OBJS20_IN_PATCH))
    OBJS20_USED_IN_PATCH := $(filter $(OBJS20_IN_PATCH),$(ST20_OBJS))
    OBJS40_USED_IN_PATCH := $(filter $(OBJS40_IN_PATCH),$(ST40_OBJS))
    OBJS200_USED_IN_PATCH := $(filter $(OBJS200_IN_PATCH),$(ST200_OBJS))
    OBJSLINUX_USED_IN_PATCH := $(filter $(OBJSLINUX_IN_PATCH),$(LINUX_OBJS))

# Check that object in patch are present. If not remove patch subdir
      ifeq ($(ARCHITECTURE),ST40)
        ifeq ($(OBJS40_USED_IN_PATCH),)
            SUBDIRS := $(filter-out patch, $(SUBDIRS))
        endif
      else
        ifeq ($(ARCHITECTURE),ST200)
            ifeq ($(OBJS200_USED_IN_PATCH),)
                SUBDIRS := $(filter-out patch, $(SUBDIRS))
            endif
        else
	    ifeq ($(OBJS20_USED_IN_PATCH),)
                SUBDIRS := $(filter-out patch, $(SUBDIRS))
	    endif
        endif
      endif
    ST20_OBJS := $(filter-out $(OBJS20_USED_IN_PATCH),$(ST20_OBJS))
    LINUX_OBJS := $(filter-out $(OBJSLINUX_USED_IN_PATCH),$(LINUX_OBJS))
    ST40_OBJS := $(filter-out $(OBJS40_USED_IN_PATCH),$(ST40_OBJS))
    ST200_OBJS := $(filter-out $(OBJS200_USED_IN_PATCH),$(ST200_OBJS))

    export OBJS20_USED_IN_PATCH
    export OBJS40_USED_IN_PATCH
    export OBJS200_USED_IN_PATCH
    export OBJSLINUX_USED_IN_PATCH
endif

# Export flags for subdirs
#-----------------------------
export FLAGS_FOR_SUBDIRS

# Remove subdir from imports
#-----------------------------
ST20_IMPORTS := $(filter-out $(SUBDIRS),$(ST20_IMPORTS))
ST40_IMPORTS := $(filter-out $(SUBDIRS),$(ST40_IMPORTS))
ST200_IMPORTS := $(filter-out $(SUBDIRS),$(ST200_IMPORTS))

# Pick up standars targets
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif
ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif
ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif
ifeq "$(BUILDING_FOR)" "LINUX"

# LM: Under Linux the following lines force to compile subdirs.
# If not done here, compiled libraries would not be placed
# in the adequate directory thus making the concatenation
# between liblinux.a libdumpreg.a libpatch.a impossible
# (new makesystem constraints).

$(DVD_BUILD_DIR)/linux/liblinux.a: FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/linux
	$(CP) $(DVD_BUILD_DIR)/linux/stapigat.ko .

$(DVD_BUILD_DIR)/dumpreg/libdumpreg.a: FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/dumpreg
	$(CP) $(DVD_BUILD_DIR)/dumpreg/objs/LINUX/libdumpreg.a $(DVD_BUILD_DIR)/dumpreg

ifneq "$(findstring stblit,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
$(DVD_BUILD_DIR)/snake/libsnake.a: FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/snake
	$(CP) $(DVD_BUILD_DIR)/snake/objs/LINUX/libsnake.a $(DVD_BUILD_DIR)/snake
endif

$(DVD_BUILD_DIR)/stextvin/libstextvin.a: FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/stextvin
	$(CP) $(DVD_BUILD_DIR)/stextvin/objs/LINUX/libstextvin.a $(DVD_BUILD_DIR)/stextvin

ifneq ($(OBJSLINUX_USED_IN_PATCH),)
$(DVD_BUILD_DIR)/patch/libpatch.a: FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/patch
	$(CP) $(DVD_BUILD_DIR)/patch/objs/LINUX/libpatch.a $(DVD_BUILD_DIR)/patch

# LM: These rules ($(DVD_BUILD_DIR)/.../....a) override the default makesystem
# in order to take the above rules.
  ifneq "$(findstring stblit,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
  $(LINUX_TARGETS): $(LINUX_OBJS) $(DVD_BUILD_DIR)/linux/liblinux.a $(DVD_BUILD_DIR)/dumpreg/libdumpreg.a $(DVD_BUILD_DIR)/patch/libpatch.a $(DVD_BUILD_DIR)/snake/libsnake.a
	@echo "File(s) in patch directory"
	$(BUILD_LIBRARY)
  else
  $(LINUX_TARGETS): $(LINUX_OBJS) $(DVD_BUILD_DIR)/linux/liblinux.a $(DVD_BUILD_DIR)/dumpreg/libdumpreg.a $(DVD_BUILD_DIR)/stextvin/libstextvin.a $(DVD_BUILD_DIR)/patch/libpatch.a
	@echo "File(s) in patch directory"
	$(BUILD_LIBRARY)
  endif
else
  ifneq "$(findstring stblit,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
  $(LINUX_TARGETS): $(LINUX_OBJS) $(DVD_BUILD_DIR)/linux/liblinux.a $(DVD_BUILD_DIR)/dumpreg/libdumpreg.a $(DVD_BUILD_DIR)/snake/libsnake.a
	@echo "No file in patch directory"
	$(BUILD_LIBRARY)
  else
  $(LINUX_TARGETS): $(LINUX_OBJS) $(DVD_BUILD_DIR)/linux/liblinux.a $(DVD_BUILD_DIR)/dumpreg/libdumpreg.a $(DVD_BUILD_DIR)/stextvin/libstextvin.a
	@echo "No file in patch directory"
	$(BUILD_LIBRARY)
  endif
endif

FORCE:
endif


# Dependancies
#-------------
$(ST20_OBJS): testcfg.h
$(ST40_OBJS): testcfg.h
$(ST200_OBJS): testcfg.h

# Clean .lib and obj
#-------------------
ifneq "$(findstring stblit,$(STAPIGAT_IMPORTS_$(ARCHITECTURE)))" ""
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	$(MAKE) -C $(DVD_BUILD_DIR)/dumpreg clean
	$(MAKE) -C $(DVD_BUILD_DIR)/snake clean
else
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	$(MAKE) -C $(DVD_BUILD_DIR)/dumpreg clean
	$(MAKE) -C $(DVD_BUILD_DIR)/stextvin clean
endif


# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

#end
