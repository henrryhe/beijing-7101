DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stsys

## For OS40 we use os20emu -- this must be added as a header import to
## avoid problems when performing a tree build
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

IMPORTS := stos stpio stboot stcommon stuart
TARGETS := uart_test$(EXE_SUFFIX) mtsafe$(EXE_SUFFIX) stuffing$(EXE_SUFFIX) terminal$(EXE_SUFFIX) \
           soak$(EXE_SUFFIX) rtscts$(EXE_SUFFIX) xonxoff$(EXE_SUFFIX) producer$(EXE_SUFFIX) \
           consumer$(EXE_SUFFIX) 


ifeq "$(DVD_OS)" "OS40"
IMPORTS := stos sttbx stboot stpio stuart stcommon os20emu
else
ifndef DISABLE_TOOLBOX
IMPORTS += sttbx
endif
endif

ifdef STUART_DMA_SUPPORT
IMPORTS += stfdma
TARGETS += uart_dma$(EXE_SUFFIX)
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

# -DSTUART_DEBUG for test/debug code in the driver
# -DSTUART_STATISTICS to enable the statistics API
# -DSTUART_TEST_VERBOSE to output all test data - may result in large files
# -DSTUART_EXTERNAL_LOOPBACK loopback test require external loopback connector

# Setup default values

ifndef STUART_STATISTICS
export STUART_STATISTICS = 1
endif

ifndef STUART_TEST_VERBOSE
STUART_TEST_VERBOSE = 0
endif

ifndef STUART_EXTERNAL_LOOPBACK
STUART_EXTERNAL_LOOPBACK = 0
endif

TESTDEFS := -DSTUART_STATISTICS=1 \
            -DTEST_VERBOSE=$(STUART_TEST_VERBOSE) \
            -DEXTERNAL_LOOPBACK=$(STUART_EXTERNAL_LOOPBACK)

ifndef DISABLE_TOOLBOX
TESTDEFS += -DSTTBX_PRINT
endif

# Temp solution to mediaref building prior to availabilty of config files.
ifdef MEDIAREF
TESTDEFS +=-DMEDIAREF
endif

ifndef DISABLE_TOOLBOX
CFLAGS +=  -DSTTBX_PRINT -DSTTBX_REPORT
endif

ifdef STUART_DMA_SUPPORT
CFLAGS +=  -DSTUART_DMA_SUPPORT
endif

# cache control
ifdef DISABLE_ICACHE
CFLAGS += -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
CFLAGS += -DDISABLE_DCACHE
endif

CFLAGS := $(CFLAGS) $(TESTDEFS)

include $(DVD_MAKE)/defrules.mak

# Objects
ifeq "$(ARCHITECTURE)" "ST40"
soak$(OBJ_SUFFIX): uart_test.c
	$(CC) $(CFLAGS) -DSOAK $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

stuffing$(OBJ_SUFFIX): uart_test.c
	$(CC) $(CFLAGS) -DSTUFFING $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

terminal$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTERMINAL $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

mtsafe$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTEST_MULTITHREADSAFE $(CFLAGS)  $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

producer$(OBJ_SUFFIX): uart_test.c
	$(CC) -DPRODUCER $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

consumer$(OBJ_SUFFIX): uart_test.c
	$(CC) -DCONSUMER $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

rtscts$(OBJ_SUFFIX): uart_test.c
	$(CC) -DRTSCTS $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

xonxoff$(OBJ_SUFFIX): uart_test.c
	$(CC) -DXONXOFF $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

uart_dma$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTEST_UARTDMA $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

endif

ifeq "$(ARCHITECTURE)" "ST200"

soak$(OBJ_SUFFIX): uart_test.c
	$(CC) $(CFLAGS)  -mcore=st231 -DSOAK $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

stuffing$(OBJ_SUFFIX): uart_test.c
	$(CC) $(CFLAGS)  -mcore=st231 -DSTUFFING $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

terminal$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTERMINAL  -mcore=st231 $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

mtsafe$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTEST_MULTITHREADSAFE -mcore=st231 $(CFLAGS)  $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

producer$(OBJ_SUFFIX): uart_test.c
	$(CC) -DPRODUCER $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

consumer$(OBJ_SUFFIX): uart_test.c
	$(CC) -DCONSUMER $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

rtscts$(OBJ_SUFFIX): uart_test.c
	$(CC) -DRTSCTS $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

xonxoff$(OBJ_SUFFIX): uart_test.c
	$(CC) -DXONXOFF $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

uart_dma$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTEST_UARTDMA $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -o $@ -c $<

endif

ifeq "$(ARCHITECTURE)" "ST20"
soak$(OBJ_SUFFIX): uart_test.c
	$(CC) $(CFLAGS) -DSOAK $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

stuffing$(OBJ_SUFFIX): uart_test.c
	$(CC) $(CFLAGS) -DSTUFFING $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

terminal$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTERMINAL $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

mtsafe$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTEST_MULTITHREADSAFE $(CFLAGS)  $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

producer$(OBJ_SUFFIX): uart_test.c
	$(CC) -DPRODUCER $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

consumer$(OBJ_SUFFIX): uart_test.c
	$(CC) -DCONSUMER $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

rtscts$(OBJ_SUFFIX): uart_test.c
	$(CC) -DRTSCTS $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

xonxoff$(OBJ_SUFFIX): uart_test.c
	$(CC) -DXONXOFF $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

uart_dma$(OBJ_SUFFIX): uart_test.c
	$(CC) -DTEST_UARTDMA $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) $^ -o $@

endif

# Applications
uart_test$(EXE_SUFFIX): uart_test$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

soak$(EXE_SUFFIX): soak$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

stuffing$(EXE_SUFFIX): stuffing$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

mtsafe$(EXE_SUFFIX): mtsafe$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

terminal$(EXE_SUFFIX): terminal$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

producer$(EXE_SUFFIX): producer$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

consumer$(EXE_SUFFIX): consumer$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

rtscts$(EXE_SUFFIX): rtscts$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

xonxoff$(EXE_SUFFIX): xonxoff$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

uart_dma$(EXE_SUFFIX): uart_dma$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

ST200_LKFLAGS =  -Wl,--M $(basename $@).map

clean:
	-$(RM) *$(OBJ_SUFFIX)
	-$(RM) *.dbg
	-$(RM) *$(EXE_SUFFIX)
else
include $(DVD_MAKE)/builddir.mak
endif
