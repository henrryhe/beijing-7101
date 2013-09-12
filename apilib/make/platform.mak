# platform.mak file
#
# Copyright (C) 1999 STMicroelectronics
#
# This file forms part of the multi-chip support in
# the build system. This file is included by generic.mak
# and this in turn includes the appropriate platform, chip
# and block files.
#

ifndef DVD_CONFIG_PLATFORM
  ifndef DVD_PLATFORM
    # Define the platform here, because we need the definition
    # to default the DVD_CONFIG_PLATFORM.
    DVD_PLATFORM := mb231
  endif
  DVD_CONFIG_PLATFORM := $(DVD_PLATFORM)

  # Just for sanity, check the platform is correct later.
  CHECK_DVD_PLATFORM := yes
endif

ifndef DVD_USER_CONFIG

# User config directory not set, so just take defaults

# include the platform config file if it exists.
-include $(PLATFORM_DIR)/$(DVD_CONFIG_PLATFORM).mak

ifndef CHIP_LIST
  CHIP_LIST := $(DVD_FRONTEND) $(filter-out $(DVD_FRONTEND),$(DVD_BACKEND))
endif

# include all the chip config files
ifdef CHIP_LIST
-include $(addprefix $(CHIP_DIR),$(addsuffix .mak,$(CHIP_LIST)))
endif

# include all the block config files
ifdef BLOCK_LIST
-include $(addprefix $(DVD_BUILD_DIR),$(addsuffix .mak,$(BLOCK_LIST)))
endif

else  # DVD_USER_CONFIG

# user config directory set, so things are more complex
# The user is allowed to override certain files in the
# DVD_USER_CONFIG directory.

# include the platform config file if it exists.
-include $(if $(wildcard $(DVD_USER_CONFIG)/platform/$(DVD_CONFIG_PLATFORM).mak),\
                         $(DVD_USER_CONFIG)/platform/$(DVD_CONFIG_PLATFORM).mak,\
                         $(PLATFORM_DIR)/$(DVD_CONFIG_PLATFORM).mak)

# include all the chip config files
ifdef CHIP_LIST
-include $(foreach CHIP,$(CHIP_LIST),\
              $(if $(wildcard $(DVD_USER_CONFIG)/chip/$(CHIP).mak),\
                              $(DVD_USER_CONFIG)/chip/$(CHIP).mak,\
                              $(CHIP_DIR)/$(CHIP).mak))
endif

# include all the block config files
ifdef BLOCK_LIST
-include $(foreach BLOCK,$(BLOCK_LIST),\
              $(if $(wildcard $(DVD_USER_CONFIG)/block/$(BLOCK).mak),\
                              $(DVD_USER_CONFIG)/block/$(BLOCK).mak,\
                              $(DVD_BUILD_DIR)/$(BLOCK).mak))
endif


endif # DVD_USER_CONFIG

ifeq "$(CHECK_DVD_PLATFORM)" "yes"
  # The DVD_CONFIG_PLATFORM was defaulted from DVD_PLATFORM - check
  # that the .mak file has not redefined this. If it has, it will just
  # cause confusion.
  ifneq "$(DVD_CONFIG_PLATFORM)" "$(DVD_PLATFORM)"
    $(error $(DVD_CONFIG_PLATFORM).mak redefines DVD_PLATFORM. Set config platform with DVD_CONFIG_PLATFORM variable, then DVD_PLATFORM can be defined in .mak file.)
  endif
endif

# Extend the optional CFLAGS with a define for each chip in the chip list
OPTIONAL_CFLAGS += $(addprefix -DST_,$(filter-out $(DVD_FRONTEND) $(DVD_BACKEND),$(CHIP_LIST)))

# End of file
