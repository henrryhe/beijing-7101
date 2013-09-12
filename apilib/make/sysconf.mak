# sysconf.mak file
#
# Copyright (C) 1999 STMicroelectronics
#
# This makesystem file sets up the defaults for various build
# configuration options.
#

# Default to mb231 - this may be set in platform.mak too.
ifndef DVD_PLATFORM
  DVD_PLATFORM := mb231
endif

# Look-up table of platforms/frontend
ifeq "$(ARCHITECTURE)" "ST20"
ST_PLATFORMS := :mb282:5512    :mb282b:5512   :mb275:5508     :mb193:TP3 \
                :mb314:5514    :mb5518:5518   :mediaref:5514  :walkiry:5700 \
                :mb361:5516    :mb382:5517    :mb376:5528     :espresso:5528 \
                :mb390:5100    :mb391:7710    :mb400:5105     :mb385:5700 \
                :maly3s:5105   :mb395:5100    :mb457:5188     :mb436:5107 \
                :DTT5107:5107  :CAB5107:5107  :SAT5107:5107
endif
ifeq "$(ARCHITECTURE)" "SPARC"
ST_PLATFORMS := :explorer4010:7015 :explorer8010:7020
endif
ifeq "$(ARCHITECTURE)" "ST40"
ST_PLATFORMS := :mb317a:GX1    :mb317b:GX1    :overdrive:7750 \
                :mediaref:GX1  :mb376:5528    :espresso:5528 \
                :mb411:7100    :mb519:7200
endif
ifeq "$(ARCHITECTURE)" "ST200"
ST_PLATFORMS := :mb390:5301    :mb421:8010    :mb426:8010     :mb428:5525
endif

# Calculate DVD_FRONTEND from platform list
ifndef DVD_FRONTEND
  DVD_FRONTEND := $(strip \
                    $(foreach i,$(ST_PLATFORMS),\
                      $(if $(findstring :$(DVD_PLATFORM):,$(i)),\
                        $(subst :$(DVD_PLATFORM):,,$(i)),)))
endif

ifndef DVD_FRONTEND
  DVD_FRONTEND := 5510
endif

ifndef DVD_BACKEND
  DVD_BACKEND := $(DVD_FRONTEND)
endif

ifndef DVD_SERVICE
  DVD_SERVICE := DVB
endif

ifndef DVD_TRANSPORT
  ifeq "$(findstring $(DVD_FRONTEND),5508 5518)" "$(DVD_FRONTEND)"
    DVD_TRANSPORT := link
  else
    ifeq "$(findstring $(DVD_FRONTEND),8010)" "$(DVD_FRONTEND)"
      DVD_TRANSPORT := demux
    else
    DVD_TRANSPORT := pti
  endif
endif
endif

# Make sure they are in lower case
ifeq "$(DVD_TRANSPORT)" "PTI"
  override DVD_TRANSPORT := pti
endif

ifeq "$(DVD_TRANSPORT)" "LINK"
  override DVD_TRANSPORT := link
endif

ifeq "$(DVD_TRANSPORT)" "STPTI"
  override DVD_TRANSPORT := stpti
endif

ifeq "$(DVD_TRANSPORT)" "STPTI4"
  override DVD_TRANSPORT := stpti4
endif

ifeq "$(DVD_TRANSPORT)" "DEMUX"
  override DVD_TRANSPORT := demux
endif

ifneq "$(DVD_TRANSPORT)" "pti"
  ifneq "$(DVD_TRANSPORT)" "link"
    ifneq "$(DVD_TRANSPORT)" "stpti"
      ifneq "$(DVD_TRANSPORT)" "stpti4"
        ifneq "$(DVD_TRANSPORT)" "demux"
          $(error Invalid DVD_TRANSPORT ($(DVD_TRANSPORT)), options (pti link stpti stpti4 demux))
        endif
      endif
    endif
  endif
endif

ifeq "$(DVD_TRANSPORT)" "pti"
  DVD_TRANSPORT_UPPERCASE := PTI
endif

ifeq "$(DVD_TRANSPORT)" "link"
  DVD_TRANSPORT_UPPERCASE := LINK
endif

ifeq "$(DVD_TRANSPORT)" "stpti"
  DVD_TRANSPORT_UPPERCASE := STPTI
endif

ifeq "$(DVD_TRANSPORT)" "stpti4"
  DVD_TRANSPORT_UPPERCASE := STPTI4
endif

ifeq "$(DVD_TRANSPORT)" "demux"
  DVD_TRANSPORT_UPPERCASE := DEMUX
endif

# Default linker procedure to call
ifndef DVD_LINK_INIT
  DVD_LINK_INIT := board_init
endif

# End of file
