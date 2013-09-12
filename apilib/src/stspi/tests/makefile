DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak


# Other components this component is dependant upon
ifdef DISABLE_TOOLBOX
IMPORTS := stboot stpio stos stevt stspi stcommon
else
IMPORTS := stboot stpio stos stevt stspi sttbx stuart stcommon
endif
ifdef STSPI_MASTER_SLAVE
TARGETS := stspitest$(EXE_SUFFIX) stspimstest$(EXE_SUFFIX)
else
TARGETS := stspitest$(EXE_SUFFIX)
endif

ifdef FULL_MEMORY_ACCESS
TEST_FLAGS += -DFULL_MEMORY_ACCESS
endif

ifdef STSPI_MASTER_SLAVE
TEST_FLAGS +=  -DSTSPI_MASTER_SLAVE
endif

ifdef DISABLE_ICACHE
TEST_FLAGS += -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
TEST_FLAGS +=  -DDISABLE_DCACHE
endif

ifdef DISABLE_TOOLBOX
TEST_FLAGS +=  -DSTSPI_NO_TBX
endif

# For I2C and SPI using same SSC
ifdef STI2C_STSPI_SAME_SSC_SUPPORT
	CFLAGS := $(CFLAGS) -DSTI2C_STSPI_SAME_SSC_SUPPORT
endif

# Local object which comprises this component
ifdef STSPI_MASTER_SLAVE
OBJS  := stspitest$(OBJ_SUFFIX) stspimstest$(OBJ_SUFFIX)
else
OBJS  := stspitest$(OBJ_SUFFIX)
endif

ifndef ST40_TARGETS
  ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
  ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
  ST40_IMPORTS = $(IMPORTS)
endif

ifndef ST200_TARGETS
  ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
  ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
  ST200_IMPORTS = $(IMPORTS)
endif

# Set compile flags
CFLAGS := $(CFLAGS) $(TEST_FLAGS) -DSTTBX_PRINT -DSTTBX_REPORT

include $(DVD_MAKE)/defrules.mak

stspitest$(EXE_SUFFIX): $(OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

ifdef STSPI_MASTER_SLAVE
stspimstest$(EXE_SUFFIX): $(OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(all_OBJS)
	-$(RM) $(TARGETS)
	-$(RM) *.dbg

else

include $(DVD_MAKE)/builddir.mak

endif

