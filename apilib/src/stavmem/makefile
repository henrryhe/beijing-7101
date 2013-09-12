#######################################
# MODULE : stavmem.lib / libstavmem.a #
#######################################

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Dependencies
HEADER_IMPORTS := stsys stos

ST20_IMPORTS := sttbx stuart stpio


ifeq ($(STAVMEM_NO_GPDMA),)
  ifeq "$(DVD_FRONTEND)" "5514"
    STAVMEM_USE_GPDMA := 1
  endif
  ifeq "$(DVD_FRONTEND)" "5528"
    STAVMEM_USE_GPDMA := 1
  endif
# Use the GPDMA simulator to enable test GPDMA with simulator on target without no GPDMA hardware.
  ifdef STGPDMA_SIM
   STAVMEM_USE_GPDMA := 1
  endif
endif

ifeq ($(STAVMEM_NO_DEBUG_MEMORY_STATUS),)
  ifeq "$(DVD_FRONTEND)" "5514"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5528"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5516"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5517"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_FRONTEND)" "5100"
     STAVMEM_USE_MEMORY_STATUS :=1
  endif

  ifeq "$(DVD_BACKEND)" "7020"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif
 ifeq "$(DVD_FRONTEND)" "TP3"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

 ifeq "$(DVD_FRONTEND)" "7710"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

 ifeq "$(DVD_FRONTEND)" "7100"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

 ifeq "$(DVD_FRONTEND)" "5105"
     STAVMEM_USE_MEMORY_STATUS :=1
  endif

 ifeq "$(DVD_FRONTEND)" "5301"
     STAVMEM_USE_MEMORY_STATUS :=1
  endif

 ifeq "$(DVD_FRONTEND)" "7109"
    STAVMEM_USE_MEMORY_STATUS :=1
  endif

 ifeq "$(DVD_FRONTEND)" "7200"
    STAVMEM_USE_MEMORY_STATUS :=1
 endif

 ifeq "$(DVD_FRONTEND)" "5188"
     STAVMEM_USE_MEMORY_STATUS :=1
 endif

 ifeq "$(DVD_FRONTEND)" "5525"
     STAVMEM_USE_MEMORY_STATUS :=1
  endif

 ifeq "$(DVD_FRONTEND)" "5107"
     STAVMEM_USE_MEMORY_STATUS :=1
  endif


endif

ifneq ($(STAVMEM_USE_GPDMA),)
    ST20_IMPORTS := $(ST20_IMPORTS) stgpdma
endif

#ifneq ($(STAVMEM_USE_MEMORY_STATUS),)
#   ST20_IMPORTS := $(ST20_IMPORTS)
#endif

ifeq ($(STAVMEM_NO_FDMA),)
  ifeq "$(DVD_FRONTEND)" "5517"
    STAVMEM_USE_FDMA := 1
  endif

  ifeq "$(DVD_FRONTEND)" "5100"
    STAVMEM_USE_FDMA := 1
  endif

 ifeq "$(DVD_FRONTEND)" "7710"
    STAVMEM_USE_FDMA := 1
 endif

ifeq "$(DVD_FRONTEND)" "7100"
    STAVMEM_USE_FDMA := 1
endif

 ifeq "$(DVD_FRONTEND)" "5105"
    STAVMEM_USE_FDMA := 1
  endif
ifeq "$(DVD_FRONTEND)" "5301"
    STAVMEM_USE_FDMA := 1
endif

ifeq "$(DVD_FRONTEND)" "5525"
    STAVMEM_USE_FDMA := 1
  endif
#ifeq "$(DVD_FRONTEND)" "5188"
#    STAVMEM_USE_FDMA := 1
#endif

 #ifeq "$(DVD_FRONTEND)" "5107"
 #   STAVMEM_USE_FDMA := 1
 # endif

ifeq "$(DVD_FRONTEND)" "7109"
    STAVMEM_USE_FDMA := 1
endif

ifeq "$(DVD_FRONTEND)" "7200"
    STAVMEM_USE_FDMA := 1
endif

endif
ifneq ($(STAVMEM_USE_FDMA),)
    ST20_IMPORTS := $(ST20_IMPORTS) stfdma stos
endif


ST40_IMPORTS := $(ST20_IMPORTS)
ST200_IMPORTS := $(ST20_IMPORTS)


# Define the targets
#-------------------

ST20_TARGETS := stavmem.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))


ST20_EXPORTS := stavmem.h stavmem.lib
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))


# Copy methods target dependent
#------------------------------
ifeq "$(DVD_FRONTEND)" "5510"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5512"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5508"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
  ifeq ($(STAVMEM_NO_1DBM),)
      STAVMEM_USE_1DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5518"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
  ifeq ($(STAVMEM_NO_1DBM),)
      STAVMEM_USE_1DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5514"
# use of GPDMA method already selected at beginning of makefile
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5516"
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "5517"
# use of FDMA method already selected at beginning of makefile
  ifeq ($(STAVMEM_NO_2DBM),)
      STAVMEM_USE_2DBM := 1
  endif
endif

ifeq "$(DVD_FRONTEND)" "TP3"
  ifeq ($(STAVMEM_NO_BMP),)
      STAVMEM_USE_BMP := 1
  endif
endif

ifneq ($(STAVMEM_NO_COPY_FILL),)
  CFLAGS := $(CFLAGS) -D STAVMEM_NO_COPY_FILL
endif

# Common objects
#---------------
ST20_OBJS := avm_init.tco avm_allo.tco avm_acc.tco acc_best.tco acc_bptr.tco

# Object and compilation flag always set
#---------------------------------------

ifneq ($(DVD_SECURED_CHIP),)
     	CFLAGS := $(CFLAGS) -D DVD_SECURED_CHIP
endif
ifneq ($(STAVMEM_NO_STMES),)
        CFLAGS := $(CFLAGS) -D STAVMEM_NO_STMES
endif

	ifeq ($(STAVMEM_NO_COPY_FILL),)
		ST20_OBJS := $(ST20_OBJS) acc_bptr.tco
        CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_C_STANDARD_FUNCTIONS
        ST20_OBJS := $(ST20_OBJS) acc_cstd.tco
	endif

# Clean .lib and obj, even old ones compiled for another target
#--------------------------------------------------------------
CLEAN_ST20_OBJS := $(ST20_OBJS) acc_bmp.tco acc_1dbm.tco acc_2dbm.tco acc_gpd.tco acc_fdma.tco
CLEAN_ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(CLEAN_ST20_OBJS))
CLEAN_ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(CLEAN_ST20_OBJS))

# Objects and compilation flag chosen
#------------------------------------
ifeq ($(STAVMEM_NO_COPY_FILL),)
 ifeq ($(DVD_SECURED_CHIP),)
    ifneq ($(STAVMEM_USE_BMP),)
	CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
	ST20_OBJS := $(ST20_OBJS) acc_bmp.tco
    endif

    ifneq ($(STAVMEM_USE_1DBM),)
   	CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
	ST20_OBJS := $(ST20_OBJS) acc_1dbm.tco
    endif

    ifneq ($(STAVMEM_USE_2DBM),)
   	CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
	ST20_OBJS := $(ST20_OBJS) acc_2dbm.tco
    endif

    ifneq ($(STAVMEM_USE_GPDMA),)
   	CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_GPDMA
   	ST20_OBJS := $(ST20_OBJS) acc_gpd.tco
    endif
 endif
 #ifneq ($(STAVMEM_USE_MEMORY_STATUS),)
 #  CFLAGS := $(CFLAGS) -D STAVMEM_DEBUG_MEMORY_STATUS
 #  ST20_OBJS := $(ST20_OBJS)
 #endif

 ifneq ($(STAVMEM_USE_FDMA),)
   CFLAGS := $(CFLAGS) -D STAVMEM_MEM_ACCESS_FDMA
   ST20_OBJS := $(ST20_OBJS) acc_fdma.tco
 endif
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
endif

ifneq ($(STAVMEM_MAX_MAX_PARTITION),)
	CFLAGS += -DSTAVMEM_MAX_MAX_PARTITION=$(STAVMEM_MAX_MAX_PARTITION)
endif

ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))


# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
endif


ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)
endif


clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $(CLEAN_$(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

# Local header dependencies
avm_init.tco avm_init.o : avm_init.h avm_allo.h
avm_allo.tco avm_allo.o : avm_init.h avm_allo.h
avm_acc.tco  avm_acc.o  : avm_init.h avm_acc.h acc_best.h
acc_best.tco acc_best.o : avm_init.h avm_acc.h acc_best.h
acc_bptr.tco acc_bptr.o : avm_acc.h acc_bptr.h
acc_cstd.tco acc_cstd.o : avm_acc.h acc_cstd.h
acc_bmp.tco  acc_bmp.o  : avm_acc.h acc_bmp.h
acc_2dbm.tco acc_2dbm.o : avm_acc.h acc_2dbm.h
acc_1dbm.tco acc_1dbm.o : avm_acc.h acc_1dbm.h
acc_gpd.tco  acc_gpd.o  : avm_acc.h acc_gpd.h
acc_fdma.tco acc_fdma.o : avm_acc.h acc_fdma.h

else
include $(DVD_MAKE)/builddir.mak
endif
