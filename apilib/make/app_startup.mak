ifneq (,$(findstring valiaudio,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D AUDIO_INIT
endif

ifneq (,$(findstring dhryston,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D DHRYSTON_INIT
endif

ifneq (,$(findstring hddfs,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D HDDFS_INIT
endif

ifneq (,$(findstring max4562,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D MAX4562_INIT
endif

ifneq (,$(findstring ptivalid,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D PTIVALID_INIT
endif

ifneq (,$(findstring saa7114,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D SAA7114_INIT
endif

ifneq (,$(findstring staud,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D AUD_INIT
endif

ifneq (,$(findstring staud2,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D AUD2_INIT
endif

ifneq (,$(findstring stavmem,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D AVMEM_INIT
endif

ifneq (,$(findstring stboot,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D BOOT_INIT
endif

ifneq (,$(findstring stclkrv,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D CLKRV_INIT
endif

ifneq (,$(findstring stcommon,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D COMMON_INIT
endif

ifneq (,$(findstring stcount,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D COUNT_INIT
endif

ifneq (,$(findstring stdenc,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D DENC_INIT
endif

ifneq (,$(findstring stdisp,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D DISP_INIT
endif

ifneq (,$(findstring stevt,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D EVT_INIT
endif

ifneq (,$(findstring sti2c,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D I2C_INIT
endif

ifneq (,$(findstring stlli,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D LLI_INIT
endif

ifneq (,$(findstring stpio,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D PIO_INIT
endif

ifneq (,$(findstring stpti,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D PTI_INIT
endif

ifneq (,$(findstring stpwm,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D PWM_INIT
endif

ifneq (,$(findstring sttbx,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D TBX_INIT
endif

ifneq (,$(findstring stuart,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D UART_INIT
endif

ifneq (,$(findstring valivideo,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D VID_INIT
endif

ifneq (,$(findstring stvos,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D VOS_INIT
endif

ifneq (,$(findstring stvtg,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D VTG_INIT
endif

ifneq (,$(findstring tda8752,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D TDA8752_INIT
endif

ifneq (,$(findstring testmem,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D TESTMEM_INIT
endif

ifneq (,$(findstring testreg,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D TESTREG_INIT
endif

ifneq (,$(findstring testtool,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D TST_INIT
endif

ifneq (,$(findstring trace,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D TRACE_INIT
endif

ifneq (,$(findstring tasks,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D TASKS_INIT
endif

ifneq (,$(findstring valigamma,$(IMPORTS)))
  CFLAGS := $(CFLAGS) -D GAMMA_INIT
endif

ifdef AUD_INIT
  ifdef AUDIO_INIT
    $(error FATAL_ERROR: audio and staud can not both be included)
  endif
endif

ifdef PTIVALID_INIT
  ifdef STPTI_INIT
    $(error FATAL_ERROR: ptivalid and stpti can not both be included)
  endif
endif
