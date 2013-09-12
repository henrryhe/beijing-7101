DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

ifeq "$(DVD_OS)" "LINUX"

export DISABLE_TBX=1

endif


# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
IMPORTS = stpio stuart stboot stos stcommon stevt stsmart
TARGETS = core$(EXE_SUFFIX) insrtrm$(EXE_SUFFIX) t1scope$(EXE_SUFFIX)


# Only build this if (presumably) the driver was also built with the t1sim
# flag
ifdef SMART_T1SIMTEST
  TARGETS += t1sim$(EXE_SUFFIX)
endif

ifeq "$(ARCHITECTURE)" "ST20"
TARGETS += dscard$(EXE_SUFFIX)
endif

ifndef DISABLE_TBX
IMPORTS += sttbx
endif

#ST40
ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifeq "$(DVD_OS)" "LINUX"
ST40_IMPORTS = stos stcommon stpio stevt stsmart
else
ST40_IMPORTS = $(IMPORTS)
endif

#ST200
ifndef ST200_TARGETS
ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
ST200_IMPORTS = $(IMPORTS)
endif

# Test definitions:
# STSMART_TEST_CLIENTS - sets the number of clients to test simultaneously
# Select default options
ifndef STSMART_TEST_CLIENTS
  STSMART_TEST_CLIENTS=4
endif

# Append options to CFLAGS
TESTDEFS := -DSMART_NUMBER_CLIENTS=$(STSMART_TEST_CLIENTS) -DSTTBX_PRINT \
            -DSTSMART_USE_STEVT

ifdef SMART_MULTIPLE_RESET
TESTDEFS += -DSMART_MULTIPLE_RESET
endif

ifdef STSMART_DETECT_INVERTED
TESTDEFS += -DSTSMART_DETECT_INVERTED
endif

ifdef STSMART_WARM_RESET
    CFLAGS += -DSTSMART_WARM_RESET
endif

ifdef DISABLE_TBX
TESTDEFS := $(TESTDEFS) -DDISABLE_TBX
endif

CFLAGS += $(TESTDEFS)

include $(DVD_MAKE)/defrules.mak

# Libs
$(LIB_PREFIX)dslib$(LIB_SUFFIX): dslib$(OBJ_SUFFIX)
	$(BUILD_LIBRARY)

# Objects
ifeq "$(ARCHITECTURE)" "ST40"
core$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Coompiling to core$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

insrtrm$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to insrtrm$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -DTEST_INSERT_REMOVE $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

dscard$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to dscard$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -DTEST_DS_CARD $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

t1scope$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to t1scope$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -DTEST_T1_CARD_SCOPE $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

t1sim$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to t1sim$(OBJ_SUFFIX)
	$(CC)  $(CFLAGS) -DTEST_T1_CARD_SIM -DSMART_T1TEST $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

endif



ifeq "$(ARCHITECTURE)" "ST200"
core$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to core$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

insrtrm$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to insrtrm$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -mcore=st231 -DTEST_INSERT_REMOVE $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

dscard$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to dscard$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -mcore=st231 -DTEST_DS_CARD $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

t1scope$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to t1scope$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -mcore=st231 -DTEST_T1_CARD_SCOPE $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

t1sim$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to t1sim$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -mcore=st231 -DTEST_T1_CARD_SIM -DSMART_T1TEST $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $^

endif

ifeq "$(ARCHITECTURE)" "ST20"
core$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to core$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

insrtrm$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to insrtrm$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -DTEST_INSERT_REMOVE $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

dscard$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to dscard$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -DTEST_DS_CARD $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

t1scope$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to t1scope$(OBJ_SUFFIX)
	$(CC) $(CFLAGS) -DTEST_T1_CARD_SCOPE $(INCLUDES) $(BASE_INCLUDES) $^ -o $@


t1sim$(OBJ_SUFFIX): smarttst.c
	@$(ECHO) Compiling to t1sim$(OBJ_SUFFIX)
	$(CC) -DTEST_T1_CARD_SIM -DSMART_T1TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

endif


# Applications
core$(EXE_SUFFIX): core$(OBJ_SUFFIX)  $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

insrtrm$(EXE_SUFFIX): insrtrm$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

ifeq "$(ARCHITECTURE)" "ST20"
dscard$(EXE_SUFFIX): dscard$(OBJ_SUFFIX) $(LIB_PREFIX)dslib$(LIB_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

t1scope$(EXE_SUFFIX): t1scope$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

t1sim$(EXE_SUFFIX): t1sim$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

clean:
	-$(RM) *$(OBJ_SUFFIX)
	-$(RM) *$(LIB_SUFFIX) ../objs/LINUX/*.a
	-$(RM) *$(EXE_SUFFIX)
	-$(RM) *.dbg

else

include $(DVD_MAKE)/builddir.mak

endif