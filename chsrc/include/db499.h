/************************************************************************
File Name   : db499.h

Description : System-wide devices header file for the DB499 board.
              All defintions are relative to the STEM base address
              which is particular to each motherboard.

COPYRIGHT (C) STMicroelectronics 2001.

************************************************************************/

#ifndef __DB499_H
#define __DB499_H

#ifdef __cplusplus
extern "C" {
#endif
    
/* DB499 board EPLD IC6 Memory locations */
#define DB499_TRANSPORT_SELECT_0_OFFSET   (0x140000)
#define DB499_TRANSPORT_SELECT_1_OFFSET   (0x140004)
#define DB499_TRANSPORT_SELECT_2_OFFSET   (0x140008)
#define DB499_SWAP_VAL_PCKTCLK_OFFSET     (0x14000C)
#define DB499_SWAP_VAL_BITCLK_OFFSET      (0x140014)
#define DB499_EPLD_REV_OFFSET             (0x140018)
#define DB499_STEM_DEVICEID_OFFSET        (0x14001C)
        
#ifdef __cplusplus
}
#endif

#endif /* __DB499_H */
