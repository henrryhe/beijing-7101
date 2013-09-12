#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
#
# EMBXSHM (cached) build management
#

EMBXSHM_CFLAGS = -DEMBXSHM_CACHED_HEAP
EMBXSHM_DIR = ../embxshm
EMBXSHM_LIB = embxshmc

# The following prevents the copying of sources into the embxshmc directory
BUILDING_SHMC = 1 

vpath %.c $(EMBXSHM_DIR)
vpath %.h $(EMBXSHM_DIR)

CLEAN_DIRS += *.c *.h
DIST_SRC = makefile

include $(EMBXSHM_DIR)/makefile

