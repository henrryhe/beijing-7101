.PHONY: check_object_dir

include $(DVD_MAKE)/environment.mak

OBJECT_DIR := $(DVD_BUILD_DIR)/objs/$(OBJECT_DIRECTORY)

ifeq "$(wildcard $(DVD_OS_LC).mak)" ""
  # No OS specific makefile, so use makefile
  USE_MAKEFILE := makefile
else
  USE_MAKEFILE := $(DVD_OS_LC).mak
endif

define MAKE_AT_TARGET
  $(if $(filter $(MAKELEVEL),$(DVD_MAKELIMIT)), \
	@$(ECHO) MAKELEVEL limit reached - cannot sub-make $@, \
	- $(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(OBJECT_DIR))
	@$(MAKE) -C $(OBJECT_DIR) -f $(DVD_BUILD_DIR)/$(USE_MAKEFILE) \
		IN_OBJECT_DIR=1 DVD_PARENT_DIR=$(DVD_BUILD_DIR) $@)
endef

define MAKE_DEFAULT_TARGET
  $(if $(filter $(MAKELEVEL),$(DVD_MAKELIMIT)), \
	@$(ECHO) MAKELEVEL limit reached - cannot sub-make $@, \
	@$(MAKE) -C $(OBJECT_DIR) -f $(DVD_BUILD_DIR)/$(USE_MAKEFILE) \
		IN_OBJECT_DIR=1 DVD_PARENT_DIR=$(DVD_BUILD_DIR))
endef

default: check_object_dir
	$(MAKE_DEFAULT_TARGET)

# For some reason, .DEFAULT fails on the check_object_dir, so
# we catch all the targets that we are likely to use.

%.lib: check_object_dir
	$(MAKE_AT_TARGET)

lib%.a: check_object_dir
	$(MAKE_AT_TARGET)

%.lku: check_object_dir
	$(MAKE_AT_TARGET)

%.exe: check_object_dir
	$(MAKE_AT_TARGET)

%.hex: check_object_dir
	$(MAKE_AT_TARGET)

ifeq "$(ARCHITECTURE)" "ST20"
  OTHER_ARCHS := ST40 ST200
else
  ifeq "$(ARCHITECTURE)" "ST40"
    OTHER_ARCHS := ST20 ST200
  else
    OTHER_ARCHS := ST20 ST40
  endif
endif

clean: check_object_dir
	$(MAKE_AT_TARGET)
    ifndef PRESERVE_FILES
	$(FULLRM) objs$(SLASH)$(OBJECT_DIRECTORY)
    else
	$(RMDIR) objs$(SLASH)$(OBJECT_DIRECTORY)
    endif
    ifeq "$(DVD_FULL_CLEAN)" "yes"
      ifneq "$(wildcard $(addprefix objs/,$(OTHER_ARCHS)))" ""
        ifneq "$(wildcard $(addsuffix $(addprefix objs/,$(OTHER_ARCHS)),/*))" ""
	$(RM) $(wildcard $(addsuffix $(addprefix objs$(SLASH),$(OTHER_ARCHS)),$(SLASH)*))
        endif
	$(RMDIR) $(addprefix objs$(SLASH),$(OTHER_ARCHS))
      endif
    endif
	-$(RMDIR) objs

ifndef SUPPRESS_CLEAN_ALL
clean_all: check_object_dir
	$(MAKE_AT_TARGET)
    ifndef PRESERVE_FILES
	$(FULLRM) objs$(SLASH)$(OBJECT_DIRECTORY)
    else
	$(RMDIR) objs$(SLASH)$(OBJECT_DIRECTORY)
    endif
    ifeq "$(DVD_FULL_CLEAN)" "yes"
      ifneq "$(wildcard $(addprefix objs/,$(OTHER_ARCHS)))" ""
        ifneq "$(wildcard $(addsuffix $(addprefix objs/,$(OTHER_ARCHS)),/*))" ""
	$(RM) $(wildcard $(addsuffix $(addprefix objs$(SLASH),$(OTHER_ARCHS)),$(SLASH)*))
        endif
	$(RMDIR) $(addprefix objs$(SLASH),$(OTHER_ARCHS))
      endif
    endif
	-$(RMDIR) objs
endif

# For targets without a rule
.DEFAULT: check_object_dir
	$(MAKE_AT_TARGET)

check_object_dir: $(DVD_BUILD_DIR)/objs $(OBJECT_DIR)

$(DVD_BUILD_DIR)/objs:
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_BUILD_DIR)/objs)
  ifeq "$(DVD_HOST)" "unix"
	chmod 770 $(DVD_BUILD_DIR)/objs
  endif


$(OBJECT_DIR):
	$(MKDIR) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(OBJECT_DIR))
  ifeq "$(DVD_HOST)" "unix"
	chmod 770 $(OBJECT_DIR)
  endif
