/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.

File Name:   stfe.c
Description: API functions to control STFE IP block


******************************************************************************/
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

/* Includes ---------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX)  */

#include "stsys.h"
#include "stcommon.h"
#include "stos.h"

#include "stdevice.h"

#include "stpti.h"
#include "pti_loc.h"

#if defined (STPTI_FRONTEND_HYBRID)

#include "tsgdma.h"
#include "stfe.h"

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Input Block defines */
#define IB_BASE_ADDRESS                             (STFE_BASE_ADDRESS)
#define IB_REG_SPACING                              (0x40)  /* Address size between each input's Input Block registers */

#define IB_REG_OFFSET_INPUT_FORMAT_CONFIG           (0x00)
#define IB_REG_OFFSET_SLAD_CONFIG                   (0x04)
#define IB_REG_OFFSET_TAG_BYTES                     (0x08)
#define IB_REG_OFFSET_PID_SETUP                     (0x0C)
#define IB_REG_OFFSET_PACKET_LENGTH                 (0x10)
#define IB_REG_OFFSET_BUFFER_START                  (0x14)
#define IB_REG_OFFSET_BUFFER_END                    (0x18)
#define IB_REG_OFFSET_READ_POINTER                  (0x1C)
#define IB_REG_OFFSET_WRITE_POINTER                 (0x20)
#define IB_REG_OFFSET_PRIORITY_THRESHOLD            (0x24)
#define IB_REG_OFFSET_STATUS                        (0x28)
#define IB_REG_OFFSET_MASK                          (0x2C)
#define IB_REG_OFFSET_SYSTEM                        (0x30)

#define IB_INTERNAL_FIFO_SIZE                       (0xA80)
#define IB_INTERNAL_FIFO_OFFSET                     (0xC0)

#define IB_REG_INPUT_FORMAT_CONFIG(Tsin_Ch_No)      (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_INPUT_FORMAT_CONFIG)
#define IB_REG_SLAD_CONFIG(Tsin_Ch_No)              (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_SLAD_CONFIG        )
#define IB_REG_TAG_BYTES(Tsin_Ch_No)                (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_TAG_BYTES          )
#define IB_REG_PID_SETUP(Tsin_Ch_No)                (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_PID_SETUP          )
#define IB_REG_PACKET_LENGTH(Tsin_Ch_No)            (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_PACKET_LENGTH      )
#define IB_REG_BUFFER_START(Tsin_Ch_No)             (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_BUFFER_START       )
#define IB_REG_BUFFER_END(Tsin_Ch_No)               (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_BUFFER_END         )
#define IB_REG_READ_POINTER(Tsin_Ch_No)             (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_READ_POINTER       )
#define IB_REG_WRITE_POINTER(Tsin_Ch_No)            (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_WRITE_POINTER      )
#define IB_REG_PRIORITY_THRESHOLD(Tsin_Ch_No)       (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_PRIORITY_THRESHOLD )
#define IB_REG_STATUS(Tsin_Ch_No)                   (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_STATUS             )
#define IB_REG_MASK(Tsin_Ch_No)                     (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_MASK               )
#define IB_REG_SYSTEM(Tsin_Ch_No)                   (IB_BASE_ADDRESS + ((Tsin_Ch_No) * IB_REG_SPACING) + IB_REG_OFFSET_SYSTEM             )

/* Tag Counter defines */
#define TAG_COUNTER_BASE_ADDRESS                    (STFE_BASE_ADDRESS + 0x0400)

#define TAG_COUNTER_REG_SPACING                     (0x40)  /* Address size between each input's TAG Counter registers */

#define TAG_COUNTER_REG_OFFSET_COUNTER_LSW          (0x00)
#define TAG_COUNTER_REG_OFFSET_COUNTER_MSW          (0x04)

#define TAG_COUNTER_REG_COUNTER_LSW(Tsin_Ch_No)     (TAG_COUNTER_BASE_ADDRESS + ((Tsin_Ch_No) * TAG_COUNTER_REG_SPACING) + TAG_COUNTER_REG_OFFSET_COUNTER_LSW )
#define TAG_COUNTER_REG_COUNTER_MSW(Tsin_Ch_No)     (TAG_COUNTER_BASE_ADDRESS + ((Tsin_Ch_No) * TAG_COUNTER_REG_SPACING) + TAG_COUNTER_REG_OFFSET_COUNTER_MSW )

#define TAG_COUNTER_MASK                            (0x06)

/* PID Filter defines*/
#define PID_FILTER_BASE_ADDRESS                     (STFE_BASE_ADDRESS + 0x0800)

#define PID_FILTER_REG_SPACING                      (0x04)  /* Address size between each input's PID Filter registers */

#define PID_FILTER_REG_BASE_ADDRESS(Tsin_Ch_No)     (PID_FILTER_BASE_ADDRESS + ((Tsin_Ch_No) * PID_FILTER_REG_SPACING) )


/* DMA Block defines */
#define DMA_REG_BASE_ADDRESS                        (STFE_BASE_ADDRESS + 0x0C00)

#define DMA_BLOCK_REG_DATA_INT_STATUS               (DMA_REG_BASE_ADDRESS + 0x00)
#define DMA_BLOCK_REG_DATA_INT_MASK                 (DMA_REG_BASE_ADDRESS + 0x04)
#define DMA_BLOCK_REG_ERROR_INT_STATUS              (DMA_REG_BASE_ADDRESS + 0x08)
#define DMA_BLOCK_REG_ERROR_INT_MASK                (DMA_REG_BASE_ADDRESS + 0x0C)
#define DMA_BLOCK_REG_ERROR_OVERRIDE                (DMA_REG_BASE_ADDRESS + 0x10)
#define DMA_BLOCK_REG_DMA_ABORT                     (DMA_REG_BASE_ADDRESS + 0x14)
#define DMA_BLOCK_REG_MESSAGE_BOUNDARY              (DMA_REG_BASE_ADDRESS + 0x18)
#define DMA_BLOCK_REG_IDLE_INT_STATUS               (DMA_REG_BASE_ADDRESS + 0x1C)
#define DMA_BLOCK_REG_IDLE_INT_MASK                 (DMA_REG_BASE_ADDRESS + 0x20)
#define DMA_BLOCK_REG_DMA_MODE                      (DMA_REG_BASE_ADDRESS + 0x24)
#define DMA_BLOCK_REG_POINTER_BASE                  (DMA_REG_BASE_ADDRESS + 0x40)

#define DMA_BLOCK_REG_BUS_BASE                      (DMA_REG_BASE_ADDRESS + 0x80)
#define DMA_BLOCK_REG_BUS_TOP                       (DMA_REG_BASE_ADDRESS + 0x84)
#define DMA_BLOCK_REG_BUS_WP                        (DMA_REG_BASE_ADDRESS + 0x88)
#define DMA_BLOCK_REG_PKT_SIZE                      (DMA_REG_BASE_ADDRESS + 0x8C)
#define DMA_BLOCK_REG_BUS_NEXT_BASE                 (DMA_REG_BASE_ADDRESS + 0x90)
#define DMA_BLOCK_REG_BUS_NEXT_TOP                  (DMA_REG_BASE_ADDRESS + 0x94)
#define DMA_BLOCK_REG_MEM_BASE                      (DMA_REG_BASE_ADDRESS + 0x98)
#define DMA_BLOCK_REG_MEM_TOP                       (DMA_REG_BASE_ADDRESS + 0x9C)
#define DMA_BLOCK_REG_MEM_RP                        (DMA_REG_BASE_ADDRESS + 0xA0)

#define DMA_BLOCK_REG_BUS_SIZE(Tsin_Ch_No)          (DMA_REG_BASE_ADDRESS + 0x200 + (4* Tsin_Ch_No ))
#define DMA_BLOCK_REG_BUS_LEVEL(Tsin_Ch_No)         (DMA_REG_BASE_ADDRESS + 0x240 + (4* Tsin_Ch_No ))
#define DMA_BLOCK_REG_BUS_THRES(Tsin_Ch_No)         (DMA_REG_BASE_ADDRESS + 0x280 + (4* Tsin_Ch_No ))

/* Internal RAM defines */
#define INTERNAL_RAM_BASE_ADDRESS                   (STFE_BASE_ADDRESS + 0x01100000)
#define INTERNAL_RAM_REG_SPACING                    (0x20)  /* Address size between each input's TAG Counter registers */

#define INTERAL_RAM_REG_OFFSET_BUS_BASE             (0x00)    /* dst */
#define INTERAL_RAM_REG_OFFSET_BUS_TOP              (0x04)
#define INTERAL_RAM_REG_OFFSET_BUS_WP               (0x08)
#define INTERAL_RAM_REG_OFFSET_TS_PKT_SIZE          (0x0C)
#define INTERAL_RAM_REG_OFFSET_BUS_NEXT_BASE        (0x10)
#define INTERAL_RAM_REG_OFFSET_BUS_NEXT_TOP         (0x14)
#define INTERAL_RAM_REG_OFFSET_MEM_BASE             (0x18)    /* src */
#define INTERAL_RAM_REG_OFFSET_MEM_TOP              (0x1C)

#define INTERAL_RAM_REG_BUS_BASE(Tsin_Ch_No)        (INTERNAL_RAM_BASE_ADDRESS + ((Tsin_Ch_No) * INTERNAL_RAM_REG_SPACING) + INTERAL_RAM_REG_OFFSET_BUS_BASE      )
#define INTERAL_RAM_REG_BUS_TOP(Tsin_Ch_No)         (INTERNAL_RAM_BASE_ADDRESS + ((Tsin_Ch_No) * INTERNAL_RAM_REG_SPACING) + INTERAL_RAM_REG_OFFSET_BUS_TOP       )
#define INTERAL_RAM_REG_BUS_WP(Tsin_Ch_No)          (INTERNAL_RAM_BASE_ADDRESS + ((Tsin_Ch_No) * INTERNAL_RAM_REG_SPACING) + INTERAL_RAM_REG_OFFSET_BUS_WP        )
#define INTERAL_RAM_REG_TS_PKT_SIZE(Tsin_Ch_No)     (INTERNAL_RAM_BASE_ADDRESS + ((Tsin_Ch_No) * INTERNAL_RAM_REG_SPACING) + INTERAL_RAM_REG_OFFSET_TS_PKT_SIZE   )
#define INTERAL_RAM_REG_BUS_NEXT_BASE(Tsin_Ch_No)   (INTERNAL_RAM_BASE_ADDRESS + ((Tsin_Ch_No) * INTERNAL_RAM_REG_SPACING) + INTERAL_RAM_REG_OFFSET_BUS_NEXT_BASE )
#define INTERAL_RAM_REG_BUS_NEXT_TOP(Tsin_Ch_No)    (INTERNAL_RAM_BASE_ADDRESS + ((Tsin_Ch_No) * INTERNAL_RAM_REG_SPACING) + INTERAL_RAM_REG_OFFSET_BUS_NEXT_TOP  )
#define INTERAL_RAM_REG_MEM_BASE(Tsin_Ch_No)        (INTERNAL_RAM_BASE_ADDRESS + ((Tsin_Ch_No) * INTERNAL_RAM_REG_SPACING) + INTERAL_RAM_REG_OFFSET_MEM_BASE      )
#define INTERAL_RAM_REG_MEM_TOP(Tsin_Ch_No)         (INTERNAL_RAM_BASE_ADDRESS + ((Tsin_Ch_No) * INTERNAL_RAM_REG_SPACING) + INTERAL_RAM_REG_OFFSET_MEM_TOP       )

/* System Registers Block */
#define SYSTEM_BLOCK_BASE_ADDRESS                   (STFE_BASE_ADDRESS + 0x0A00)
#define SYSTEM_BLOCK_REG_STATUS                     (SYSTEM_BLOCK_BASE_ADDRESS)
#define SYSTEM_BLOCK_REG_MASK                       (SYSTEM_BLOCK_BASE_ADDRESS + 0x04)

/* General values for masking data */
#define STFE_SLD_MASK                               (0x0000FFFF)
#define STFE_TAG_ENABLE_MASK                        (0x00000001)
#define STFE_TAG_HEADER_MASK                        (0xFFFF0000)
#define STFE_TAG_COUNTER_MASK                       (0x00000006)
#define STFE_SYSTEM_ENABLE                          (0x00000001)
#define STFE_PID_ENABLE                             (0x80000000)
#define STFE_SYSTEM_RESET                           (0x00000002)
#define STFE_PKT_SIZE_MASK                          (0x00000FFF)
#define STFE_TAG_STREAM_MASK                        (0x3F)

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */
/* Read the register and AND in the complement(~) of the mask and write it back including the new value */
#define STFE_ClearMaskAndSet32LE( reg, mask, u32value)\
{\
    U32 ReadValue = STSYS_ReadRegDev32LE(reg);\
    STSYS_WriteRegDev32LE ( reg, ((ReadValue & ~mask) | u32value));\
}

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */

/******************************************************************************
Function Name : StfeHal_ReadTagCounter
  Description : Read the Tag Counter of the input block
   Parameters :
******************************************************************************/
void StfeHal_ReadTagCounter ( STPTI_StreamID_t StreamID, U32 * MSW, U32 * LSW )
{
    U32 TagBytesVal;
    /* Find out which counter is being used from the TAGBytes register
       There are only 3 tag counter on the 7200 */

    TagBytesVal = STSYS_ReadRegDev32LE( (U32)IB_REG_TAG_BYTES(StreamID & STFE_TAG_STREAM_MASK));

    TagBytesVal = ( (TagBytesVal & TAG_COUNTER_MASK) >> 1 );

    *LSW = STSYS_ReadRegDev32LE(TAG_COUNTER_REG_COUNTER_LSW(TagBytesVal));
    *MSW = STSYS_ReadRegDev32LE(TAG_COUNTER_REG_COUNTER_MSW(TagBytesVal));
}

/******************************************************************************
Function Name : StfeHal_ReadSystemReg
  Description : Read the System register of the input block
   Parameters :
******************************************************************************/
U32 StfeHal_ReadSystemReg ( STPTI_StreamID_t StreamID )
{
    return (STSYS_ReadRegDev32LE((U32)IB_REG_SYSTEM(StreamID & STFE_TAG_STREAM_MASK)));
}

/******************************************************************************
Function Name : StfeHal_ReadStatusReg
  Description :
   Parameters :
******************************************************************************/
U32 StfeHal_ReadStatusReg ( STPTI_StreamID_t StreamID )
{
    return (STSYS_ReadRegDev32LE((U32)IB_REG_STATUS(StreamID & STFE_TAG_STREAM_MASK)));
}

/******************************************************************************
Function Name : StfeHal_ReadMaskReg
  Description :
   Parameters :
******************************************************************************/
U32 StfeHal_ReadMaskReg ( STPTI_StreamID_t StreamID )
{
    return (STSYS_ReadRegDev32LE((U32)IB_REG_MASK(StreamID & STFE_TAG_STREAM_MASK)));
}

/******************************************************************************
Function Name : StfeHal_ReadWPReg
  Description :
   Parameters :
******************************************************************************/
U32 StfeHal_ReadWPReg ( STPTI_StreamID_t StreamID )
{
    return (STSYS_ReadRegDev32LE( (U32)IB_REG_WRITE_POINTER (StreamID & STFE_TAG_STREAM_MASK) ) );
}

/******************************************************************************
Function Name : StfeHal_WriteSLD
  Description : Set the Sync Lock Drop params
   Parameters : StreamID
                Lock
                Drop
                Token
******************************************************************************/
void StfeHal_WriteSLD ( STPTI_StreamID_t StreamID, U8 Lock, U8 Drop, U8 Token )
{
    STTBX_Print(("StfeHal_WriteSLD  StreamID 0x%04x Lock 0x%02x Drop 0x%02x Token 0x%02x\n", (U32)StreamID, Lock, Drop, Token ));

    STFE_ClearMaskAndSet32LE( (U32)IB_REG_SLAD_CONFIG(StreamID & STFE_TAG_STREAM_MASK),
                               STFE_SLD_MASK, (U32)(Lock & 0x0F) |
                               ((U32)(Drop & 0x0F) << 4) | ((U32)(Token & 0xFF) << 8) );
}

/******************************************************************************
Function Name : StfeHal_WriteTagBytesEnable
  Description : Set the SOP token
   Parameters : StreamID
                Enable
******************************************************************************/
void StfeHal_WriteTagBytesEnable ( STPTI_StreamID_t StreamID, U8 Enable)
{
    STFE_ClearMaskAndSet32LE( (U32)IB_REG_TAG_BYTES(StreamID & STFE_TAG_STREAM_MASK),
                              STFE_TAG_ENABLE_MASK,
                              ((U32)Enable & 0x01) );
}

/******************************************************************************
Function Name : StfeHal_WriteTagBytesHeader
  Description : Set the Stream TAG token
   Parameters : StreamID
                Enable
******************************************************************************/
void StfeHal_WriteTagBytesHeader ( STPTI_StreamID_t StreamID, U16 Header)
{
    STTBX_Print(("StfeHal_WriteTagBytesHeader  StreamID 0x%04x Header 0x%04x\n", (U32)StreamID, Header ));

    STFE_ClearMaskAndSet32LE( (U32)IB_REG_TAG_BYTES(StreamID & STFE_TAG_STREAM_MASK),
                              STFE_TAG_HEADER_MASK,
                              ((U32)Header) << 16 );
}

/******************************************************************************
Function Name : StfeHal_WriteInputFormat
  Description : Set input format register
   Parameters : StreamID
                Format
******************************************************************************/
void StfeHal_WriteInputFormat ( STPTI_StreamID_t StreamID, U32 Format)
{
    STTBX_Print(("StfeHal_WriteInputFormat  StreamID 0x%04x Format 0x%08x\n", (U32)StreamID, Format ));

    STSYS_WriteRegDev32LE( (U32)IB_REG_INPUT_FORMAT_CONFIG((StreamID & STFE_TAG_STREAM_MASK)),
                            Format);
}

/******************************************************************************
Function Name : StfeHal_WritePktLength
  Description : Set input packet length
   Parameters : StreamID
                Length
******************************************************************************/
void StfeHal_WritePktLength ( STPTI_StreamID_t StreamID, U32 Length)
{
    STTBX_Print(("StfeHal_WritePktLength  StreamID 0x%04x Length 0x%08x\n", (U32)StreamID, Length ));

    STSYS_WriteRegDev32LE ( (U32)IB_REG_PACKET_LENGTH(StreamID & STFE_TAG_STREAM_MASK),
                            Length & STFE_PKT_SIZE_MASK );
}

/******************************************************************************
Function Name : StfeHal_WriteBufStart
  Description : Set Buffer start address of input block
                Lower 6 bits ignored for 7200 i.e. address must be 64 byte aligned
   Parameters : StreamID
                Address
****************************************************************************/
void StfeHal_WriteBufStart ( STPTI_StreamID_t StreamID, U32 Address)
{
    STSYS_WriteRegDev32LE ( (U32)IB_REG_BUFFER_START(StreamID & STFE_TAG_STREAM_MASK),
                            Address );
}

/******************************************************************************
Function Name : StfeHal_WriteBufEnd
  Description : Set Buffer end address of input block
                Lower 6 bits ignored for 7200 i.e. address must be 64 byte aligned
   Parameters : StreamID
                Address
****************************************************************************/
void StfeHal_WriteBufEnd ( STPTI_StreamID_t StreamID, U32 Address)
{
    STSYS_WriteRegDev32LE ( (U32)IB_REG_BUFFER_END(StreamID & STFE_TAG_STREAM_MASK),
                            Address );
}

/******************************************************************************
Function Name : StfeHal_Enable
  Description : Enable the selected input block - will always reset the IB too
   Parameters : StreamID
                Enable
****************************************************************************/
void StfeHal_Enable ( STPTI_StreamID_t StreamID, BOOL Enable)
{
    STTBX_Print(("StfeHal_Enable  StreamID 0x%04x Enable 0x%02x\n", (U32)StreamID, (U8) Enable ));

    STFE_ClearMaskAndSet32LE( (U32)IB_REG_SYSTEM(StreamID & STFE_TAG_STREAM_MASK),
                              STFE_SYSTEM_ENABLE,
                              ((U32)Enable & STFE_SYSTEM_ENABLE) | ( Enable ? STFE_SYSTEM_RESET:0) );

}

/******************************************************************************
Function Name : StfeHal_Reset
  Description : Reset the selected input block
   Parameters : StreamID
                Reset
****************************************************************************/
void StfeHal_Reset ( STPTI_StreamID_t StreamID )
{
    STFE_ClearMaskAndSet32LE( (U32)IB_REG_SYSTEM(StreamID & STFE_TAG_STREAM_MASK),
                              STFE_SYSTEM_RESET,
                              STFE_SYSTEM_RESET );
}

/******************************************************************************
Function Name : StfeHal_PIDFilterConfigure
  Description : Enable the PID filter and configure loction of PID in stream
   Parameters : StreamID
                PIDBitSize
                Offset
****************************************************************************/
void StfeHal_PIDFilterConfigure ( STPTI_StreamID_t StreamID, U8 PIDBitSize, U8 Offset, BOOL Enable )
{
    U32 EnableBit;

    STTBX_Print(("StfeHal_PIDFilterConfigure  StreamID 0x%04x PIDBitSize 0x%08x Offset 0x%02x Enable 0x%02x\n", (U32)StreamID, PIDBitSize, Offset, (U8)Enable ));

    if ( PIDBitSize > 13 )
    {
        PIDBitSize = 13;
    }

    EnableBit = (Enable ? 0x80000000:0x00000000);

    STSYS_WriteRegDev32LE( (U32)IB_REG_PID_SETUP(StreamID & STFE_TAG_STREAM_MASK),
                          EnableBit      /* bit 31 to set enable */
                          |( (U32)PIDBitSize  <<  6)      /* bits 6:11 to set the number of bits used for PID filtering */
                          |(Offset & 0x3F) /* bits 0:5 bit offset into stream to test for the PID */ );
}

/******************************************************************************
Function Name : StfeHal_PIDFilterEnable
  Description : Enable/disable the PID filter

   Parameters : StreamID
                PIDBitSize
                Offset
****************************************************************************/
void StfeHal_PIDFilterEnable ( STPTI_StreamID_t StreamID, BOOL Enable )
{
    U32 EnableBit;

    STTBX_Print(("StfeHal_PIDFilterEnable  StreamID 0x%04x Enable 0x%02x\n", (U32)StreamID, (U8)Enable ));

    EnableBit = (Enable ? STFE_PID_ENABLE:0x00000000);

    STFE_ClearMaskAndSet32LE( (U32)IB_REG_PID_SETUP(StreamID & STFE_TAG_STREAM_MASK),
                              STFE_PID_ENABLE,
                              EnableBit );
}

/******************************************************************************
Function Name : StfeHal_TAGBytesConfigure
  Description : Enable the TAG and set the header bytes
   Parameters : StreamID
                TagHeader
****************************************************************************/
void StfeHal_TAGBytesConfigure ( STPTI_StreamID_t StreamID, U16 TagHeader)
{
    STTBX_Print(("StfeHal_TAGBytesConfigure  StreamID 0x%04x TagHeader 0x%04x\n", (U32)StreamID, TagHeader ));

    STFE_ClearMaskAndSet32LE( (U32)IB_REG_TAG_BYTES(StreamID & STFE_TAG_STREAM_MASK),
                              (STFE_TAG_ENABLE_MASK | STFE_TAG_HEADER_MASK),
                              (STFE_TAG_ENABLE_MASK | ((U32)TagHeader << 16)) );
}

/******************************************************************************
Function Name : StfeHal_TAGCounterSelect
  Description : Select the TAG counter for clock recovery
   Parameters : StreamID
                TagHeader
****************************************************************************/
void StfeHal_TAGCounterSelect ( STPTI_StreamID_t StreamID, U8 TagCounter)
{
    STTBX_Print(("StfeHal_TAGCounterSelect  StreamID 0x%04x TagCounter 0x%02x\n", (U32)StreamID, TagCounter ));

    STFE_ClearMaskAndSet32LE( (U32)IB_REG_TAG_BYTES(StreamID & STFE_TAG_STREAM_MASK),
                              STFE_TAG_COUNTER_MASK,
                              ((U32)TagCounter << 1) );
}

/******************************************************************************
Function Name : StfeHal_WritePIDBase
  Description : Write the base address of the PID filter block
                Expects physical address
   Parameters : StreamID
                Address
****************************************************************************/
void StfeHal_WritePIDBase ( STPTI_StreamID_t StreamID, U32 Address )
{
    STTBX_Print(("StfeHal_WritePIDBase  StreamID 0x%04x Enable 0x%08x\n", (U32)StreamID, Address ));

    STSYS_WriteRegDev32LE( PID_FILTER_REG_BASE_ADDRESS(StreamID & STFE_TAG_STREAM_MASK), Address );
}

/******************************************************************************
Function Name : StfeHal_WriteDMAModeCircular
  Description : Write the base address of the PID filter block
   Parameters : StreamID
                Address
****************************************************************************/
void StfeHal_WriteDMAModeCircular ( STPTI_StreamID_t StreamID )
{
    STTBX_Print(("StfeHal_WriteDMAModeCircular  StreamID 0x%04x\n", (U32)StreamID ));

    STFE_ClearMaskAndSet32LE( (DMA_BLOCK_REG_DMA_MODE),
                              (1 << (StreamID & STFE_TAG_STREAM_MASK)),
                              (1 << (StreamID & STFE_TAG_STREAM_MASK)) );
}

/******************************************************************************
Function Name : StfeHal_WriteDMAPointerBase
  Description : Write the base address of the internal RAM records
   Parameters : StreamID
                Address
****************************************************************************/
void StfeHal_WriteDMAPointerBase ( STPTI_StreamID_t StreamID, U32 Address )
{
    STTBX_Print(("StfeHal_WriteDMAPointerBase  StreamID 0x%04x Address 0x%08x\n", (U32)StreamID, Address ));

    STSYS_WriteRegDev32LE( DMA_BLOCK_REG_POINTER_BASE, Address );
}

/******************************************************************************
Function Name : StfeHal_WriteInternalRAMRecord
  Description : Write the DMA address pointers
   Parameters : StreamID
                Base
                Top
                NextBase
                NextTop
                PktSize
****************************************************************************/
void StfeHal_WriteInternalRAMRecord ( STPTI_StreamID_t StreamID,
                                      U32 Base,
                                      U32 Top,
                                      U32 NextBase,
                                      U32 NextTop,
                                      U32 PktSize )
{
    /* 6 inputs of 32 byte records = 192 bytes Each input has 2688 bytes of RAM for FIFO */
    U32 InternalBase = (IB_INTERNAL_FIFO_OFFSET + ((StreamID & STFE_TAG_STREAM_MASK) * IB_INTERNAL_FIFO_SIZE));

    STTBX_Print(("StfeHal_WriteInternalRAMRecord  StreamID 0x%04x Base 0x%08x Top 0x%08x NextBase 0x%08x NextTop 0x%08x PktSize 0x%08x\n", (U32)StreamID, Base,Top,NextBase, NextTop, PktSize ));

    STSYS_WriteRegDev32LE( (U32)INTERAL_RAM_REG_BUS_BASE      (StreamID & STFE_TAG_STREAM_MASK) , Base                                     );
    STSYS_WriteRegDev32LE( (U32)INTERAL_RAM_REG_BUS_TOP       (StreamID & STFE_TAG_STREAM_MASK) , Top                                      );
    STSYS_WriteRegDev32LE( (U32)INTERAL_RAM_REG_BUS_WP        (StreamID & STFE_TAG_STREAM_MASK) , Base                                     );
    STSYS_WriteRegDev32LE( (U32)INTERAL_RAM_REG_BUS_NEXT_BASE (StreamID & STFE_TAG_STREAM_MASK) , NextBase                                 );
    STSYS_WriteRegDev32LE( (U32)INTERAL_RAM_REG_BUS_NEXT_TOP  (StreamID & STFE_TAG_STREAM_MASK) , NextTop                                  );
    STSYS_WriteRegDev32LE( (U32)INTERAL_RAM_REG_TS_PKT_SIZE   (StreamID & STFE_TAG_STREAM_MASK) , PktSize                                  );
    STSYS_WriteRegDev32LE( (U32)INTERAL_RAM_REG_MEM_BASE      (StreamID & STFE_TAG_STREAM_MASK) , InternalBase                             );
    STSYS_WriteRegDev32LE( (U32)INTERAL_RAM_REG_MEM_TOP       (StreamID & STFE_TAG_STREAM_MASK) , InternalBase + IB_INTERNAL_FIFO_SIZE - 1 );
}

/******************************************************************************
Function Name : StfeHal_WriteInternalRAMRecord
  Description : Write the DMA address pointers
   Parameters : StreamID
                PktSize
****************************************************************************/
void StfeHal_WriteDMABusSize( STPTI_StreamID_t StreamID, U32 PktSize )
{
    STTBX_Print(("StfeHal_WriteDMABusSize  StreamID 0x%04x PktSize 0x%08x\n", (U32)StreamID, PktSize ));

    STSYS_WriteRegDev32LE( (U32)DMA_BLOCK_REG_BUS_SIZE(StreamID & STFE_TAG_STREAM_MASK) , PktSize );
}


/******************************************************************************
Function Name : StfeHal_ConfigureInternalRAM
  Description : Configure pointers to internal RAM
                On the 7200 there is 16K bytes of memory of interrnal RAM.
                This holds 6 * records of DMA pointers and the rest is used for
                DMA input FIFO.
   Parameters : StreamID
                Address
****************************************************************************/
void StfeHal_ConfigureIBInternalRAM( STPTI_StreamID_t StreamID )
{
    U32 InternalBase = (IB_INTERNAL_FIFO_OFFSET + ((StreamID & STFE_TAG_STREAM_MASK) * IB_INTERNAL_FIFO_SIZE));

    STTBX_Print(("StfeHal_ConfigureIBInternalRAM  StreamID 0x%04x\n", (U32)StreamID ));

    /* Configure the IB pointers to internal RAM */

    STSYS_WriteRegDev32LE( (U32)IB_REG_BUFFER_START(StreamID & STFE_TAG_STREAM_MASK), InternalBase );
    STSYS_WriteRegDev32LE( (U32)IB_REG_BUFFER_END(StreamID & STFE_TAG_STREAM_MASK), InternalBase + IB_INTERNAL_FIFO_SIZE - 1 );
}


/******************************************************************************
Function Name : StfeHal_SetPid
  Description : Set a PID number in the PID filter by modifying the PID table
                in RAM.

                This function assumes PID width of 13 bits hence 8192 values and
                1 bit per PID hence 32 PIDs can be set in one 32 bit word.

   Parameters : PidTableVirtStartAddr
                Pid
****************************************************************************/
void StfeHal_SetPid( U32 PidTableVirtStartAddr, U16 Pid )
{
    U32 Value;
    U32 BitPosition;
    U32 AddressOffset;

    STTBX_Print(("StfeHal_SetPid Pid 0x%03x\n",Pid));

    AddressOffset = (Pid/32)*4;
    BitPosition    = 1 << (Pid%32);

    Value       = STSYS_ReadRegDev32LE( PidTableVirtStartAddr + AddressOffset);

    Value       |= BitPosition;

    STSYS_WriteRegDev32LE((PidTableVirtStartAddr + AddressOffset),Value);
}

/******************************************************************************
Function Name : StfeHal_ClearPid
  Description : Clear a PID number in the PID filter by modifying the PID table
                in RAM.

                This function assumes PID width of 13 bits hence 8192 values and
                1 bit per PID hence 32 PIDs can be set in one 32 bit word.

   Parameters : PidTableVirtStartAddr
                Pid
****************************************************************************/
void StfeHal_ClearPid( U32 PidTableVirtStartAddr, U16 Pid)
{
    U32 Value;
    U32 BitPosition;
    U32 BitMask;
    U32 AddressOffset;

    STTBX_Print(("StfeHal_ClearPid Pid 0x%03x\n",Pid));

    AddressOffset = (Pid/32)*4;
    BitPosition    = 1 << (Pid%32);

    Value       = STSYS_ReadRegDev32LE( PidTableVirtStartAddr + AddressOffset);

    BitMask     = (~BitPosition);
    Value       &= BitMask;

    STSYS_WriteRegDev32LE((PidTableVirtStartAddr + AddressOffset),Value);
}




/******************************************************************************
Function Name : Stfe_RegisterDump
  Description : Debug function to print registers
   Parameters : StreamID
******************************************************************************/
void StfeHAL_IBRegisterDump(STPTI_StreamID_t StreamID)
{
    U32 Tmp;
    U32 Channel = StreamID & 0x0F;

    Tmp = STSYS_ReadRegDev32LE( (U32)IB_REG_INPUT_FORMAT_CONFIG(Channel) );
    STTBX_Print(("----------- STFE Input Block %u debug -----------\n", (U8)Channel ));
    STTBX_Print(("INPUT_FORMAT :  0x%08X\n", Tmp ));
    STTBX_Print(("INPUT_FORMAT :  SerialNotParallel   : %u\n", Tmp & 0x01 ));
    STTBX_Print(("INPUT_FORMAT :  ByteEndianness      : %u\n", (Tmp >> 1 )& 0x01 ));
    STTBX_Print(("INPUT_FORMAT :  AsyncNotSync        : %u\n", (Tmp >> 2 )& 0x01 ));
    STTBX_Print(("INPUT_FORMAT :  AlignByteSOP        : %u\n", (Tmp >> 3 )& 0x01 ));
    STTBX_Print(("INPUT_FORMAT :  InvertTSClk         : %u\n", (Tmp >> 4 )& 0x01 ));
    STTBX_Print(("INPUT_FORMAT :  IgnoreErrorInByte   : %u\n", (Tmp >> 5 )& 0x01 ));
    STTBX_Print(("INPUT_FORMAT :  IgnoreErrorInPkt    : %u\n", (Tmp >> 6 )& 0x01 ));
    STTBX_Print(("INPUT_FORMAT :  IgnoreErrorAtSOP    : %u\n", (Tmp >> 7 )& 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));
    Tmp = STSYS_ReadRegDev32LE( (U32)IB_REG_SLAD_CONFIG(Channel) );
    STTBX_Print(("SLAD_CONFIG  :  0x%08X\n", Tmp ));
    STTBX_Print(("SLAD_CONFIG  :  Sync                : %u\n", Tmp & 0x0F ));
    STTBX_Print(("SLAD_CONFIG  :  Drop                : %u\n", (Tmp >> 4 )& 0x0F ));
    STTBX_Print(("SLAD_CONFIG  :  Token               : 0x%02X\n", (Tmp >> 8 )& 0xFF ));
    STTBX_Print(("SLAD_CONFIG  :  SLDEndianness       : %u\n", (Tmp >> 16)& 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));
    Tmp = STSYS_ReadRegDev32LE( (U32)IB_REG_TAG_BYTES(Channel));
    STTBX_Print(("TAG_BYTES    :  0x%08X\n", Tmp ));
    STTBX_Print(("TAG_BYTES    :  Enable              : %u\n", Tmp & 0x01 ));
    STTBX_Print(("TAG_BYTES    :  Counter             : %u\n", (Tmp >> 1 )& 0x03 ));
    STTBX_Print(("TAG_BYTES    :  Header              : 0x%04X\n", (Tmp >> 16 )& 0xFFFF ));
    STTBX_Print(("-------------------------------------------------\n"));
    STTBX_Print(("TAG_COUNTER  0x%08x%08x\n",STSYS_ReadRegDev32LE(TAG_COUNTER_REG_COUNTER_LSW( ((Tmp >> 1 )& 0x03)  )), STSYS_ReadRegDev32LE(TAG_COUNTER_REG_COUNTER_MSW(((Tmp >> 1 )& 0x03)))    )) ;
    STTBX_Print(("-------------------------------------------------\n"));
    
    Tmp = STSYS_ReadRegDev32LE( (U32)IB_REG_PID_SETUP(Channel) );
    STTBX_Print(("PID_SETUP    :  0x%08X\n", Tmp ));
    STTBX_Print(("PID_SETUP    :  Offset              : %u\n", Tmp & 0x3F ));
    STTBX_Print(("PID_SETUP    :  NumBits             : %u\n", (Tmp >> 6 )& 0x3F ));
    STTBX_Print(("PID_SETUP    :  Enable              : %u\n", (Tmp >> 31)& 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    STTBX_Print(("PKT_LENGTH   :  %u\n"    , STSYS_ReadRegDev32LE( (U32)IB_REG_PACKET_LENGTH     (Channel) ) & 0xFFF ));
    STTBX_Print(("BUFFER_START :  0x%08X\n", STSYS_ReadRegDev32LE( (U32)IB_REG_BUFFER_START      (Channel) )         ));
    STTBX_Print(("BUFFER_END   :  0x%08X\n", STSYS_ReadRegDev32LE( (U32)IB_REG_BUFFER_END        (Channel) )         ));
    STTBX_Print(("READ_POINTER :  0x%08X\n", STSYS_ReadRegDev32LE( (U32)IB_REG_READ_POINTER      (Channel) )         ));
    STTBX_Print(("WRITE_POINTER:  0x%08X\n", STSYS_ReadRegDev32LE( (U32)IB_REG_WRITE_POINTER     (Channel) )         ));
    STTBX_Print(("P_THRESHOLD  :  %u\n"    , STSYS_ReadRegDev32LE( (U32)IB_REG_PRIORITY_THRESHOLD(Channel) ) & 0xF   ));
    STTBX_Print(("-------------------------------------------------\n"));


    STTBX_Print(("STATUS       :  0x%08X\n", Tmp ));

    /* This has been commented out because of a hardware bug, to read this register
       an input must be present as the input clock latches this register */
#if 0
    Tmp = STSYS_ReadRegDev32LE( (U32)IB_REG_STATUS(Channel) );
    STTBX_Print(("STATUS       :  0x%08X\n", Tmp ));
    STTBX_Print(("STATUS       :  FifoOverFlow        : %u\n", Tmp & 0x01 ));
    STTBX_Print(("STATUS       :  BufferOverflow      : %u\n", (Tmp >> 1 )& 0x01 ));
    STTBX_Print(("STATUS       :  OutOfOrderRP        : %u\n", (Tmp >> 2 )& 0x01 ));
    STTBX_Print(("STATUS       :  PIDOverFlow         : %u\n", (Tmp >> 3 )& 0x01 ));
    STTBX_Print(("STATUS       :  PktOverFlow         : %u\n", (Tmp >> 4 )& 0x01 ));
    STTBX_Print(("STATUS       :  ErrorPkts           : %u\n", (Tmp >> 5 )& 0x01 ));
    STTBX_Print(("STATUS       :  ShortPkts           : %u\n", (Tmp >> 6 )& 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));
#endif
    Tmp = STSYS_ReadRegDev32LE( (U32)IB_REG_MASK(Channel) );
    STTBX_Print(("MASK         :  0x%08X\n", Tmp ));
    STTBX_Print(("MASK         :  FifoOverFlow        : %u\n", Tmp & 0x01 ));
    STTBX_Print(("MASK         :  BufferOverflow      : %u\n", (Tmp >> 1 )& 0x01 ));
    STTBX_Print(("MASK         :  OutOfOrderRP        : %u\n", (Tmp >> 2 )& 0x01 ));
    STTBX_Print(("MASK         :  PIDOverFlow         : %u\n", (Tmp >> 3 )& 0x01 ));
    STTBX_Print(("MASK         :  PktOverFlow         : %u\n", (Tmp >> 4 )& 0x01 ));
    STTBX_Print(("MASK         :  ErrorPkts           : %u\n", (Tmp >> 5 )& 0x01 ));
    STTBX_Print(("MASK         :  ShortPkts           : %u\n", (Tmp >> 6 )& 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    Tmp = STSYS_ReadRegDev32LE( (U32)IB_REG_SYSTEM(Channel));
    STTBX_Print(("SYSTEM       :  0x%08X\n", Tmp ));
    STTBX_Print(("SYSTEM       :  Enable              : %u\n", Tmp & 0x01 ));
    STTBX_Print(("SYSTEM       :  Reset               : %u\n", (Tmp >> 1 )& 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));
#if !defined(ST_OSLINUX)
    Tmp = STSYS_ReadRegDev32LE( (U32)PID_FILTER_REG_BASE_ADDRESS(Channel));
    STTBX_Print(("PID_BASE     :  0x%08X\n", Tmp ));
    STTBX_Print(("-------------------------------------------------\n"));

    {
        U32 i,j;

        STTBX_Print(("PID RAM......\n"));
        for ( i=0; i<32; i++)
        {

            for ( j=0;j<8;j++)
            {
                STTBX_Print(("%08x ", STSYS_ReadRegDev32LE( NONCACHED_TRANSLATION(Tmp) )));
                Tmp +=4;
            }
            STTBX_Print(("\n"));
        }
    }
#endif
    {
        U32 i,j;
        U32 addr= (INTERNAL_RAM_BASE_ADDRESS + IB_INTERNAL_FIFO_OFFSET + (IB_INTERNAL_FIFO_SIZE * (StreamID & 0x0F)));

        STTBX_Print(("Internal FIFO RAM......\n"));
        for ( i=0; i<(IB_INTERNAL_FIFO_SIZE/4/8); i++)
        {
            STTBX_Print(("0x%08X ", addr)); /* Print address */
            for ( j=0;j<8;j++)
            {
                STTBX_Print(("%08x ", STSYS_ReadRegDev32LE( addr )));
                addr +=4;
            }
            STTBX_Print(("\n"));
        }
    }



    Tmp = STSYS_ReadRegDev32LE( DMA_BLOCK_REG_DATA_INT_STATUS );
    STTBX_Print(("DINT_STATUS  :  0x%08X\n", Tmp ));
    STTBX_Print(("DINT_STATUS  :  Data Int            : %u\n", (Tmp >> Channel)  & 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    Tmp = STSYS_ReadRegDev32LE( DMA_BLOCK_REG_DATA_INT_MASK );
    STTBX_Print(("EINT_MASK    :  0x%08X\n", Tmp ));
    STTBX_Print(("EINT_MASK    :  Swap Mask           : %u\n", (Tmp >> Channel) & 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    Tmp = STSYS_ReadRegDev32LE( DMA_BLOCK_REG_ERROR_INT_STATUS );
    STTBX_Print(("EINT_STATUS  :  0x%08X\n", Tmp ));
    STTBX_Print(("EINT_STATUS  :  PtrError            : %u\n", (Tmp >> Channel) & 0x01 ));
    STTBX_Print(("EINT_STATUS  :  OverflowError       : %u\n", (Tmp >> (Channel + 16) ) & 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    Tmp = STSYS_ReadRegDev32LE( DMA_BLOCK_REG_ERROR_INT_MASK );
    STTBX_Print(("EINT_MASK    :  0x%08X\n", Tmp ));
    STTBX_Print(("EINT_MASK    :  PtrMask             : %u\n", (Tmp >> Channel) & 0x01 ));
    STTBX_Print(("EINT_MASK    :  OverflowError       : %u\n", (Tmp >> (Channel + 16) ) & 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    Tmp = STSYS_ReadRegDev32LE( DMA_BLOCK_REG_ERROR_OVERRIDE );
    STTBX_Print(("ERR_OVERRIDE :  0x%08X\n", Tmp ));
    STTBX_Print(("ERR_OVERRIDE :  OverRide            : %u\n", (Tmp >> (Channel + 16) ) & 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    Tmp = STSYS_ReadRegDev32LE( DMA_BLOCK_REG_IDLE_INT_STATUS );
    STTBX_Print(("IDLE_INT_STAT:  0x%08X\n", Tmp ));
    STTBX_Print(("IDLE_INT_STAT:  IdleInt             : %u\n", (Tmp >> Channel) & 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    Tmp = STSYS_ReadRegDev32LE( DMA_BLOCK_REG_IDLE_INT_MASK );
    STTBX_Print(("IDLE_INT_MASK:  0x%08X\n", Tmp ));
    STTBX_Print(("IDLE_INT_MASK:  IdleMask            : %u\n", (Tmp >> Channel) & 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    Tmp = STSYS_ReadRegDev32LE( DMA_BLOCK_REG_DMA_MODE );
    STTBX_Print(("DMA_MODE     :  0x%08X\n", Tmp ));
    STTBX_Print(("DMA_MODE     :  Mode                : %u\n", (Tmp >> Channel) & 0x01 ));
    STTBX_Print(("-------------------------------------------------\n"));

    STTBX_Print(("PTR_BASE     :  0x%08X\n", STSYS_ReadRegDev32LE((U32)DMA_BLOCK_REG_POINTER_BASE            ) ));
    STTBX_Print(("RAM_BUS_BASE :  0x%08X\n", STSYS_ReadRegDev32LE((U32)INTERAL_RAM_REG_BUS_BASE     (Channel)) ));
    STTBX_Print(("RAM_BUS_TOP  :  0x%08X\n", STSYS_ReadRegDev32LE((U32)INTERAL_RAM_REG_BUS_TOP      (Channel)) ));
    STTBX_Print(("RAM_BUS_WP   :  0x%08X\n", STSYS_ReadRegDev32LE((U32)INTERAL_RAM_REG_BUS_WP       (Channel)) ));
    STTBX_Print(("RAM_PKT_SIZE :  %u\n"    , STSYS_ReadRegDev32LE((U32)INTERAL_RAM_REG_TS_PKT_SIZE  (Channel)) ));
    STTBX_Print(("RAM_BUS_NBASE:  0x%08X\n", STSYS_ReadRegDev32LE((U32)INTERAL_RAM_REG_BUS_NEXT_BASE(Channel)) ));
    STTBX_Print(("RAM_BUS_NTOP :  0x%08X\n", STSYS_ReadRegDev32LE((U32)INTERAL_RAM_REG_BUS_NEXT_TOP (Channel)) ));
    STTBX_Print(("RAM_MEM_BASE :  0x%08X\n", STSYS_ReadRegDev32LE((U32)INTERAL_RAM_REG_MEM_BASE     (Channel)) ));
    STTBX_Print(("RAM_MEM_TOP  :  0x%08X\n", STSYS_ReadRegDev32LE((U32)INTERAL_RAM_REG_MEM_TOP      (Channel)) ));
    STTBX_Print(("-------------------------------------------------\n"));

    STTBX_Print(("BUS_BASE     :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_BUS_BASE     ) ));
    STTBX_Print(("BUS_TOP      :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_BUS_TOP      ) ));
    STTBX_Print(("BUS_WP       :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_BUS_WP       ) ));
    STTBX_Print(("PKT_SIZE     :  %u\n"    , STSYS_ReadRegDev32LE(DMA_BLOCK_REG_PKT_SIZE     ) ));
    STTBX_Print(("BUS_NBASE    :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_BUS_NEXT_BASE) ));
    STTBX_Print(("BUS_NTOP     :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_BUS_NEXT_TOP ) ));
    STTBX_Print(("MEM_BASE     :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_MEM_BASE     ) ));
    STTBX_Print(("MEM_TOP      :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_MEM_TOP      ) ));
    STTBX_Print(("MEM_RP       :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_MEM_RP       ) ));

    STTBX_Print(("DMA_BUS_SIZE :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_BUS_SIZE (Channel)  ) ));
    STTBX_Print(("DMA_BUS_LEV  :  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_BUS_LEVEL(Channel)  ) ));
    STTBX_Print(("DMA_BUS_THRES:  0x%08X\n", STSYS_ReadRegDev32LE(DMA_BLOCK_REG_BUS_THRES(Channel)  ) ));
}

#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */

/* End of stfe.c ----------------------------------------------------------- */
