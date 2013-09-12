/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.

   File Name: stfe.h
 Description: Internal defines and prototypes for access STFE helper functions

******************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __STFE_H
#define __STFE_H
/* Includes ------------------------------------------------------------ */
#include "stcommon.h"
#include "stdevice.h"
#include "stpti.h"
/* Exported Types ------------------------------------------------------ */

/* Exported Constants -------------------------------------------------- */
#define STFE_BASE_ADDRESS                     (ST7200_STFE_BASE_ADDRESS)
#define STFE_INTERNAL_RAM_BASE_ADDRESS        (STFE_BASE_ADDRESS + 0x01100000)
#define STFE_INTERNAL_RAM_SIZE                (1024*16)
#define STFE_PID_RAM_SIZE                     (1024)

/* This offset is the bit number of the lsb of the pid within the first 8 bytes of the pkt,
counting from the lsb of the 8th byte to arrive. */
#define STFE_DVB_PID_BIT_OFFSET       (40)

#define STFE_DVB_DEFAULT_LOCK         (3)
#define STFE_DVB_DEFAULT_DROP         (3)

#define STFE_DVB_DMA_PKT_SIZE         (200)

#define STFE_INPUT_FORMAT_SERIAL_NOT_PARALLEL (0x00000001)
#define STFE_INPUT_FORMAT_BYTE_ENDIANNESS     (0x00000002)
#define STFE_INPUT_FORMAT_ASYNC_NOT_SYNC      (0x00000004)
#define STFE_INPUT_FORMAT_ALIGN_BYTE_SOP      (0x00000008)
#define STFE_INPUT_FORMAT_INVERT_TS_CLK       (0x00000010)
#define STFE_INPUT_FORMAT_IGNORE_ERROR_BYTE   (0x00000020)
#define STFE_INPUT_FORMAT_IGNORE_ERROR_PKT    (0x00000040)
#define STFE_INPUT_FORMAT_IGNORE_ERROR_SOP    (0x00000080)

#define STFE_STATUS_FIFO_OVERFLOW             (0x00000001)
#define STFE_STATUS_BUFFER_OVERFLOW           (0x00000002)
#define STFE_STATUS_OUT_OF_ORDER_RP           (0x00000004)
#define STFE_STATUS_PID_OVERFLOW              (0x00000008)
#define STFE_STATUS_PKT_OVERFLOW              (0x00000010)

#define STFE_STATUS_ERROR_PKTS                (0x00000F00)
#define STFE_STATUS_ERROR_PKTS_POSITION       (8)

#define STFE_STATUS_ERROR_SHORT_PKTS          (0x0000F000)
#define STFE_STATUS_ERROR_SHORT_PKTS_POSITION (12)

/* Exported Variables -------------------------------------------------- */
/* Exported Macros ----------------------------------------------------- */

/* Macro to extract a data from U32 word register word where more than one bit of data is required */
#define STFE_EXTRACT_DATA_FROM_U32 ( DATA, MASK, POSITION )    ((DATA & MASK) >> POSITION) 

/* Exported Functions -------------------------------------------------- */
void StfeHal_ReadTagCounter         ( STPTI_StreamID_t StreamID, U32 * MSW, U32 * LSW                  );
U32  StfeHal_ReadSystemReg          ( STPTI_StreamID_t StreamID                                        );
U32  StfeHal_ReadStatusReg          ( STPTI_StreamID_t StreamID                                        );
U32  StfeHal_ReadWPReg              ( STPTI_StreamID_t StreamID                                        );
void StfeHal_Reset                  ( STPTI_StreamID_t StreamID                                        );
void StfeHal_WriteSLD               ( STPTI_StreamID_t StreamID, U8 Lock, U8 Drop, U8 Token            );
void StfeHal_Enable                 ( STPTI_StreamID_t StreamID, BOOL Enable                           );
void StfeHal_WriteInputFormat       ( STPTI_StreamID_t StreamID, U32 Format                            );
void StfeHal_WritePIDBase           ( STPTI_StreamID_t StreamID, U32 Address                           );
void StfeHal_PIDFilterConfigure     ( STPTI_StreamID_t StreamID, U8 PIDBitSize, U8 Offset, BOOL Enable );
void StfeHal_PIDFilterEnable        ( STPTI_StreamID_t StreamID, BOOL Enable                           );

void StfeHal_ClearPid               ( U32 PidTableVirtStartAddr, U16 Pid);
void StfeHal_SetPid                 ( U32 PidTableVirtStartAddr, U16 Pid);

void StfeHal_TAGBytesConfigure      ( STPTI_StreamID_t StreamID, U16 TagHeader              );
void StfeHal_TAGCounterSelect       ( STPTI_StreamID_t StreamID, U8 TagCounter              );

void StfeHal_WritePktLength         ( STPTI_StreamID_t StreamID, U32 Length                 );
void StfeHal_WriteDMAModeCircular   ( STPTI_StreamID_t StreamID                             );
void StfeHal_WriteDMAPointerBase    ( STPTI_StreamID_t StreamID, U32 Address                );
void StfeHal_WriteDMABusSize        ( STPTI_StreamID_t StreamID, U32 PktSize );
void StfeHal_WriteInternalRAMRecord ( STPTI_StreamID_t StreamID, 
                                      U32 Base,
                                      U32 Top,
                                      U32 NextBase,
                                      U32 NextTop,
                                      U32 PktSize                                           );
void StfeHal_ConfigureIBInternalRAM ( STPTI_StreamID_t StreamID                             );

void StfeHal_WriteTagBytesHeader    ( STPTI_StreamID_t StreamID, U16 Header);
void StfeHAL_IBRegisterDump         ( STPTI_StreamID_t StreamID                             );

#endif /* #ifndef __STFE_H */
/* End of stfe.h */
