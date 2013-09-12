/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005, 2006.  All rights reserved.

   File Name: pti4.h
 Description: HAL defines and structures

******************************************************************************/

#ifndef __PTI4_H
 #define __PTI4_H

/* Includes ------------------------------------------------------------ */
#ifndef __KERNEL__
#include <stdlib.h>
#include <stddef.h>
#endif
                                               
#include "stddefs.h"
#include "stpti.h"

#include "cam.h"


/* ------------------------------------------------------------------------- */

/*
    Note that these values MUST NOT be surrounded by parentheses as is usually advised. They 
    are substituted into a structure name in a macro, which will fail if parentheses are added. 
*/

#define  BITFIELD_ELEMENT_1       1
#define  BITFIELD_ELEMENT_2       2
#define  BITFIELD_ELEMENT_3       3
#define  BITFIELD_ELEMENT_4       4
#define  BITFIELD_ELEMENT_5       5
#define  BITFIELD_ELEMENT_6       6
#define  BITFIELD_ELEMENT_7       7
#define  BITFIELD_ELEMENT_8       8
#define  BITFIELD_ELEMENT_9       9
#define BITFIELD_ELEMENT_10      10
#define BITFIELD_ELEMENT_11      11
#define BITFIELD_ELEMENT_12      12
#define BITFIELD_ELEMENT_13      13
#define BITFIELD_ELEMENT_14      14
#define BITFIELD_ELEMENT_15      15
#define BITFIELD_ELEMENT_16      16
#define BITFIELD_ELEMENT_17      17
#define BITFIELD_ELEMENT_18      18



#if defined( STPTI_ARCHITECTURE_PTI4 )
#define TC_DATA_RAM_SIZE           6656         /* (6.5 * 1024)  */
#define TC_CODE_RAM_SIZE           7680         /* (7.5 * 1024)  */
#else
#if defined( STPTI_ARCHITECTURE_PTI4_LITE )
#define TC_DATA_RAM_SIZE           6656         /* (6.5 * 1024)  */
#define TC_CODE_RAM_SIZE           9216         /* (9.0 * 1024)  */
#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1)
#define TC_DATA_RAM_SIZE          13824         /* (13.5 * 1024) */
#define TC_CODE_RAM_SIZE          13312         /* (13 * 1024)   */
#else
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
#define TC_DATA_RAM_SIZE          13824         /* (13.5 * 1024) */
#define TC_CODE_RAM_SIZE          16384         /* (16 * 1024)   */
#endif
#endif
#endif
#endif


#define INDEX_FLAGS  ( STATUS_FLAGS_PUSI_FLAG                 | \
                       STATUS_FLAGS_SCRAMBLE_CHANGE           | \
                       STATUS_FLAGS_DISCONTINUITY_INDICATOR   | \
                       STATUS_FLAGS_RANDOM_ACCESS_INDICATOR   | \
                       STATUS_FLAGS_PRIORITY_INDICATOR        | \
                       STATUS_FLAGS_PCR_FLAG                  | \
                       STATUS_FLAGS_OPCR_FLAG                 | \
                       STATUS_FLAGS_SPLICING_POINT_FLAG       | \
                       STATUS_FLAGS_PRIVATE_DATA_FLAG         | \
                       STATUS_FLAGS_ADAPTATION_EXTENSION_FLAG | \
                       STATUS_FLAGS_FIRST_RECORDED_PACKET     | \
                       STATUS_FLAGS_BUNDLE_BOUNDARY           | \
                       STATUS_FLAGS_AUXILIARY_PACKET          | \
                       STATUS_FLAGS_MODIFIABLE_FLAG           | \
                       STATUS_FLAGS_START_CODE_FLAG           | \
                       STATUS_FLAGS_TIME_CODE                 | \
                       STATUS_FLAGS_INDX_TIMEOUT_TICK )


/* Exported Types ------------------------------------------------------ */


/* The order of the enum below matches the order 'known' by the 
   TC code and MUST NOT be changed */

typedef enum TCInterrupt_e 
{
    TC_INTERRUPT_GENERAL,
    TC_INTERRUPT_INTERRUPT_BUFFER_FULL
} TCInterrupt_t;


/* ------------------------------------------------------------------------- */


/* format of the metadata at the start of a section DMAd to a buffer by the TC */
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCSectionFilterMetaData_t)
#endif

typedef volatile struct TCSectionFilterMetaData_s  
{
    U32 SectionAssociation_0_31;
    U32 SectionAssociation_32_63;
} TCSectionFilterMetaData_t;

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCSectionFilterInfo_t)
#endif

typedef volatile struct TCSectionFilterInfo_s
{
    U16 SectionAssociation0;
    U16 SectionAssociation1;
    U16 SectionAssociation2;
    U16 SectionAssociation3;
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
    U16 SectionAssociation4;
    U16 SectionAssociation5;
    U16 SectionAssociation6;
    U16 SectionAssociation7;
#endif
    U16 SectionHeader0;
    U16 SectionHeader1;
    U16 SectionHeader2;
    U16 SectionHeader3;
    U16 SectionHeader4;
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
    U16 SectionHeader5;
    U16 SectionHeader6;
    U16 SectionHeader7;
    U16 SectionHeader8;
#endif
    U16 SectionHeaderCount;
    U16 SectionCrc0;
    U16 SectionCrc1;
    U16 SectionCount;
    U16 SectionState;
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
    U16 SectionFilterMode;
    U16 Spare;
#endif
} TCSectionFilterInfo_t;


/*
  Use for   TCSectionFilterInfo_t
            TCSessionInfo_t
            TCMainInfo_t
            TCGlobalInfo_t
            TCKey_t
            TCDMAConfig_t

   The TC registers definded in pti4.h that are U32:16 bit fields
   need be read modified and written back.

   A single 16 bit write to ether the high or low part of a U32 is written
   to both the low and high parts.

   So the unchanged part needs to be read and ORed with the changed part and
   written as a U32.

   This function does just that.

   It may not be the best way to do it but it works!!
*/

/* This macro determines whether the write is to the high or low word.
   Reads in the complete 32 bit word writes in the high or low portion and
   then writes the whole word back.
   It gets around the TC read modify write problem. */
#define STSYS_WriteTCReg16LE( reg, u16value ) \
{ \
    U32 u32value = STSYS_ReadRegDev32LE( (void*)((U32)(reg) & 0xfffffffC) );    \
    if (((U32)(reg) & 2)==2)                                                    \
    {                                                                           \
        u32value = (u32value & 0x0000FFFF) | (((U32)u16value)<<16);             \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        u32value = (u32value & 0xFFFF0000) | (((U32)u16value)&0x0000FFFF);      \
    }                                                                           \
    STSYS_WriteRegDev32LE( (void*)((U32)(reg) & 0xfffffffC), u32value );        \
}

/* Read the register and OR in the value and write it back. */
#define STSYS_SetTCMask16LE( reg, u16value)\
{\
    STSYS_WriteTCReg16LE(reg, STSYS_ReadRegDev16LE(reg) | u16value);\
}

/* Read the register and AND in the complement(~) of the value and write it back. */
#define STSYS_ClearTCMask16LE( reg, u16value)\
{\
    STSYS_WriteTCReg16LE(reg, STSYS_ReadRegDev16LE(reg) & ~u16value);\
}


/* ------------------------------------------------------------------------- */

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCSessionInfo_t)
#endif

typedef volatile struct TCSessionInfo_s
{
    U16 SessionInputPacketCount;
    U16 SessionInputErrorCount;
    
    U16 SessionCAMFilterStartAddr;   /* PTI4L (ram cam) only */
    U16 SessionCAMConfig;            /* PTI4L (ram cam) only */

    U16 SessionPIDFilterStartAddr;
    U16 SessionPIDFilterLength;

    U16 SessionSectionParams;        /* SF_Config (crc & filter type etc.) */
    U16 SessionTSmergerTag;

    U16 SessionProcessState;
    U16 SessionModeFlags;

    U16 SessionNegativePidSlotIdent;
    U16 SessionNegativePidMatchingEnable;

    U16 SessionUnmatchedSlotMode;
    U16 SessionUnmatchedDMACntrl_p;    

    U16 SessionInterruptMask0;
    U16 SessionInterruptMask1;

    U32 SectionEnables_0_31;
    U32 SectionEnables_32_63;
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    U32 SectionEnables_64_95;
    U32 SectionEnables_96_127;
#endif
    U16 SessionSTCWord0;
    U16 SessionSTCWord1;

    U16 SessionSTCWord2;
    U16 SessionDiscardParams;

    U32 SessionPIPFilterBytes;
    U32 SessionPIPFilterMask;

    U32 SessionCAPFilterBytes;
    U32 SessionCAPFilterMask;
    
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)

/* Only used for STPTI_TIMER_TICK feature */

    U16 SessionLastEvtTick_0;
    U16 SessionLastEvtTick_1;

    U16 SessionTickDMA_p;
    U16 SessionTickDMA_Slot;
#endif
} TCSessionInfo_t;


/* SessionSectionParams: */

#define TC_SESSION_INFO_FILTER_TYPE_FIELD       0xF000

#define TC_SESSION_INFO_FILTER_TYPE_IREDETO_ECM 0x2000
#define TC_SESSION_INFO_FILTER_TYPE_IREDETO_EMM 0x3000
#define TC_SESSION_INFO_FILTER_TYPE_SHORT       0x4000
#define TC_SESSION_INFO_FILTER_TYPE_LONG        0x5000
#define TC_SESSION_INFO_FILTER_TYPE_MAC         0x6000
#define TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH   0x7000


#define TC_SESSION_INFO_FORCECRCSTATE           0x0001
#define TC_SESSION_INFO_DISCARDONCRCERROR       0x0002

#define TC_SESSION_DVB_PACKET_FORMAT            0x0010

#define SESSION_USE_MERGER_FOR_STC              0x8000
#define SESSION_MASK_STC_SOURCE                 0x7FFF


#define TC_SESSION_MODE_FLAGS_DISCARD_SYNC_BYTE 0x0001    /* Written & cleared by Host only */
#define TC_SESSION_MODE_FLAGS_WILDCARD_PID_SET  0x0002    /* Written & cleared by Host only */
#define TC_SESSION_MODE_FLAGS_DMA_DISABLE_SET   0x0004    /* Written & cleared by Host only */
/* ------------------------------------------------------------------------- */

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCGlobalInfo_t)
#endif

typedef volatile struct TCGlobalInfo_s
{
    U16 GlobalPacketHeader;              /* (TC)   */
    U16 GlobalHeaderDesignator;          /* (TC)   */

    U32 GlobalLastQWrite;                /* (TC)   GlobalLastQWrite_0:16  GlobalLastQWrite_1:16 */

    U16 GlobalQPointsInPacket;           /* (TC)   */
    U16 GlobalProcessFlags;              /* (TC)   */

    U16 GlobalSlotMode;                  /* (TC)   */
    U16 GlobalDMACntrl_p;                /* (TC)   */

    U32 GlobalPktCount;                  /* (TC)   Global Input Packet Count */

    U16 GlobalSignalModeFlags;           /* (TC)   */
    U16 GlobalCAMArbiterIdle;            /* (TC)   For GNBvd18811 */
    
    U16 GlobalSFTimeouts;                /* (TC)   */
    U16 GlobalDTVPktBufferPayload_p;     /* (TC)   */
    
    U16 GlobalResidue;                   /* (TC)   */
    U16 GlobalPid;                       /* (TC)   */

    U16 GlobalIIFCtrl;                   /* (TC)   */
    U16 GlobalProfilerCount;             /* (TC)   */

    U16 GlobalRecordDMACntrl_p;          /* (TC)   Holds the address of the DMA structure for the Record Buffer */
    U16 GlobalSpare1;                    /* (TC)   Filter Address for the Start Code Detector */

    U16 GlobalCAMArbiterInhibit;         /* (Host) For GNBvd18811 */
    U16 GlobalModeFlags;                 /* (Host) */
    
    U32 GlobalScratch;                   /* (Host) GlobalScratch_0 GlobalScratch_1 */

    U16 GlobalNegativePidSlotIdent;      /* (Host) */
    U16 GlobalNegativePidMatchingEnable; /* (Host) */

    U16 GlobalSwts_Params;               /* (Host) */
    U16 GlobalSFTimeout;                 /* (Host) */

/* Only used for STPTI_TIMER_TICK feature */
    U16 GlobalTSDumpDMA_p;               /* (Host) */
    U16 GlobalTSDumpCount;               /* (Host) */

    U16 GlobalLastTickSessionNumber;     /* (TC )  */
    U16 GlobalTickEnable;                /* (Host) */

    U16 GlobalTickTemp_0;                /* (TC)   */
    U16 GlobalTickTemp_1;                /* (TC)   */
    
    U16 GlobalLastTickSlotNumber;        /* (TC )  */
    U16 GlobalSkipSetDiscard;            /* (TC )  */
 /* End of STPTI_TIMER_TICK feature variables */

} TCGlobalInfo_t;

#define GLOBAL_DMA_STATUS_BLOCK_POINTER_MASK 0x0000000f /* GlobalMiscTC & GlobalMiscCPU */


/* ------------------------------------------------------------------------- */

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCMainInfo_t)
#endif

typedef volatile struct TCMainInfo_s
{
    U16 SlotState;
    U16 PacketCount;

    U16 SlotMode;
    U16 DescramblerKeys_p;          /* a.k.a. CAPAccumulatedCount, CAPAccumulatedCount, ECMFilterData */

    U16 DMACtrl_indices;            /* MSByte is RecordBuffer index, LSByte is main Buffer index */
    U16 IndexMask;

    U16 SectionPesFilter_p;         /* a.k.a. SlotFeaturesRegister - used for RAW slot specific features (DLNA,Out TSMerger tags bytes)*/
    U16 RemainingPESLength;         /* a.k.a. ECMFilterMask */
    
    U16 PESStuff;                   /* a.k.a. CAPFilterResult, RawCorruptionParams */
    U16 StartCodeIndexing_p;        /* a.k.a. RecordBufferMode */
} TCMainInfo_t;

#define RawCorruptionParams PESStuff 

#define TC_MAIN_INFO_PES_STREAM_ID_FILTER_ENABLED               0x0100

#define TC_MAIN_INFO_SLOT_STATE_ODD_SCRAMBLED                   0x0001
#define TC_MAIN_INFO_SLOT_STATE_TRANSPORT_SCRAMBLED             0x0002
#define TC_MAIN_INFO_SLOT_STATE_SCRAMBLED                       0x0004
#define TC_MAIN_INFO_SLOT_STATE_SCRAMBLE_STATE_FIELD            (TC_MAIN_INFO_SLOT_STATE_SCRAMBLED | \
                                                                TC_MAIN_INFO_SLOT_STATE_TRANSPORT_SCRAMBLED| \
                                                                TC_MAIN_INFO_SLOT_STATE_ODD_SCRAMBLED) 

#define TC_MAIN_INFO_SLOT_STATE_DMA_IN_PROGRESS                 0x1000
#define TC_MAIN_INFO_SLOT_STATE_SEEN_PACKET                     0x2000
#define TC_MAIN_INFO_SLOT_STATE_SEEN_TS_SCRAMBLED               0x4000
#define TC_MAIN_INFO_SLOT_STATE_SEEN_PES_SCRAMBLED              0x8000
#define TC_MAIN_INFO_SLOT_STATE_SEEN_FIELD                      (TC_MAIN_INFO_SLOT_STATE_SEEN_PACKET | \
                                                                TC_MAIN_INFO_SLOT_STATE_SEEN_TS_SCRAMBLED | \
                                                                TC_MAIN_INFO_SLOT_STATE_SEEN_PES_SCRAMBLED)
                                                             
#define TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_CLEAR           0x0010
#define TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_DESCRAMBLED     0x0020
#define TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_FIELD           (TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_CLEAR | \
                                                                 TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_DESCRAMBLED)
                                                                 
#define TC_MAIN_INFO_SLOT_MODE_IGNORE_SCRAMBLING                0x0040
#define TC_MAIN_INFO_SLOT_MODE_INJECT_SEQ_ERROR_MODE            0x0080
#define TC_MAIN_INFO_SLOT_MODE_SUBSTITUTE_STREAM                0x0100

#define TC_MAIN_INFO_SLOT_MODE_DMA_1                            0x0200
#define TC_MAIN_INFO_SLOT_MODE_DMA_2                            0x0400
#define TC_MAIN_INFO_SLOT_MODE_DMA_3                            0x0600
#define TC_MAIN_INFO_SLOT_MODE_DMA_FIELD                        0x0600

#define TC_MAIN_INFO_SLOT_MODE_DISABLE_CC_CHECK                 0x0800

/* For Watch & record reuses Startcode detection word*/
#define TC_MAIN_INFO_REC_BUFFER_MODE_ENABLE                     0x0100
#define TC_MAIN_INFO_REC_BUFFER_MODE_DESCRAMBLE                 0x0200

#define TC_MAIN_INFO_STARTCODE_DETECTION_OFFSET_MASK            0x00FF
                                                                
#define TC_MAIN_INFO_PREFIX_27MHZ_TIMESTAMP_WITH_TS_PACKETS     0x0001  /* Output = 4 Bytes of 27 MHz TimeStamp + 188 Bytes of TS Packet data */
#define TC_MAIN_INFO_OUTPUT_TS_PACKETS_WITH_TAG_BYTES           0x0002  /* Output (TSMERGER based System) => SYNC_BYTE + 6 bytes of TSMerger tag + 187 Bytes of TS Packet data 
                                                                           Output (TSGDMA based System)   => 8 bytes of TSGDMA tag + 188 Bytes of TS Packet data */

/* ------------------------------------------------------------------------- */
                                                                 
                                                                        /* WORKAROUND START */
#define TC_GLOBAL_DMAX_RESET_DMA0                               0x0001                                                                           
#define TC_GLOBAL_DMAX_RESET_DMA1                               0x0002  /* used to tell the TC which DMA to reset */
#define TC_GLOBAL_DMAX_RESET_DMA2                               0x0004  /* used to tell the TC which DMA to reset */ 
#define TC_GLOBAL_DMAX_RESET_DMA3                               0x0008  /* used to tell the TC which DMA to reset */                                                              
#define TC_GLOBAL_DMAX_RESET_DMA_MASK                           0x000f  /* used to tell the TC which DMA to reset */
                                                                        /* WORKAROUND END */

#define TC_GLOBAL_DMA_ENABLE_DMA0                               0x0001                                                                           
#define TC_GLOBAL_DMA_ENABLE_DMA1                               0x0002   
#define TC_GLOBAL_DMA_ENABLE_DMA2                               0x0004  /* Bit values in Device->DMAEnable */
#define TC_GLOBAL_DMA_ENABLE_DMA3                               0x0008                                                                 
#define TC_GLOBAL_DMA_ENABLE_MASK                               0x000f

/* Bit allocations for GlobalPIPFilterStatus */
#define TC_GLOBAL_DATA_PIP_FILTER_REPEATED                      0x0001
#define TC_GLOBAL_DATA_PIP_FILTER_ENABLED                       0x0002

/* Bit allocations for GlobalPIPFilterStatus */
#define TC_GLOBAL_SWTS_PARAMS_CHANGE_SYNC_BYTE_ENABLE_SET       0x0100
#define TC_GLOBAL_SWTS_PARAMS_CHANGED_SESSION_SYNC_ID           0x0600

/*#ifdef STPTI_WORKAROUND_SFCAM*/
/* --- Special values used in GlobalCAMArbiterInhibit  --- */
#define TC_GLOBAL_DATA_CAM_ARBITER_INHIBIT_REQUEST              0x0001
#define TC_GLOBAL_DATA_CAM_ARBITER_INHIBIT_CLEAR                0x0000 

/* DMA register mutex for PTI4_LITE values in GlobalCAMArbiterInhibit written by Host */
#define TC_GLOBAL_MUTEX_FLAGS_HOST_REQUEST                      0x0002

/* --- Special values used in GlobalCAMArbiterIdle  --- */
#define TC_GLOBAL_DATA_CAM_ARBITER_IDLE_CLEAR                   0x0001
#define TC_GLOBAL_DATA_CAM_ARBITER_IDLE_BUSY                    0x0000 

/* DMA register mutex for PTI4_LITE values in GlobalCAMArbiterIdle written by TC */
#define TC_GLOBAL_MUTEX_FLAGS_TC_RUNNING                        0x0002
#define TC_GLOBAL_MUTEX_FLAGS_TC_GRANT                          0x0004

/*#endif*/



/* ------------------------------------------------------------------------- */
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCSystemKey_t)
#endif
typedef volatile struct TCSystemKey_s
{
    U32 SystemKey7;
    U32 SystemKey6;
    U32 SystemKey5;
    U32 SystemKey4;
    U32 SystemKey3;
    U32 SystemKey2;
    U32 SystemKey1;
    U32 SystemKey0;
    
} TCSystemKey_t;

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCKey_t)
#endif

/* Note that when writing host to TC the key undergoes an endian swap,
this is more complex for the extended AES keys as they are split in 2 sections.
Thus even keys 3:0 are effectively keys 7:4 for AES 16 byte keys... etc
This maintains compatibility with 8 byte keys, and gives the simplest (fastest)
TC access to the extended AES keys */

typedef volatile struct TCKey_s
{
    U16 KeyValidity;
    U16 KeyMode;

    U16 EvenKey3;
    U16 EvenKey2;

    U16 EvenKey1;
    U16 EvenKey0;

    U16 OddKey3;
    U16 OddKey2;

    U16 OddKey1;
    U16 OddKey0;
/* This is the end of the used portion for non-AES keys */

    U16 EvenKey7;
    U16 EvenKey6;
    U16 EvenKey5;
    U16 EvenKey4;

    U16 EvenIV7;
    U16 EvenIV6;
    U16 EvenIV5;
    U16 EvenIV4;
    U16 EvenIV3;
    U16 EvenIV2;
    U16 EvenIV1;
    U16 EvenIV0;

    U16 OddKey7;
    U16 OddKey6;
    U16 OddKey5;
    U16 OddKey4;

    U16 OddIV7;
    U16 OddIV6;
    U16 OddIV5;
    U16 OddIV4;
    U16 OddIV3;
    U16 OddIV2;
    U16 OddIV1;
    U16 OddIV0;
    
} TCKey_t;


#define TCKEY_VALIDITY_TS_EVEN     0x0001
#define TCKEY_VALIDITY_TS_ODD      0x0002
#define TCKEY_VALIDITY_PES_EVEN    0x0100
#define TCKEY_VALIDITY_PES_ODD     0x0200

#define TCKEY_ALGORITHM_DVB        0x0000
#define TCKEY_ALGORITHM_DSS        0x1000
#define TCKEY_ALGORITHM_FAST_I     0x2000
#define TCKEY_ALGORITHM_AES        0x3000
#define TCKEY_ALGORITHM_MULTI2     0x4000
#define TCKEY_ALGORITHM_TDES       0x5000
#define TCKEY_ALGORITHM_MASK       0xF000

#define TCKEY_CHAIN_ALG_ECB        0x0000
#define TCKEY_CHAIN_ALG_CBC        0x0010
#define TCKEY_CHAIN_ALG_ECB_IV     0x0020
#define TCKEY_CHAIN_ALG_CBC_IV     0x0030
#define TCKEY_CHAIN_ALG_OFB        0x0040
#define TCKEY_CHAIN_ALG_CTS        0x0050
#define TCKEY_CHAIN_ALG_IPTV_CSA   0x0060
#define TCKEY_CHAIN_ALG_NSA_MDD    0x00F0
#define TCKEY_CHAIN_ALG_NSA_MDI    0x0070
#define TCKEY_CHAIN_ALG_MASK       0x00F0
#define TCKEY_CHAIN_MODE_LR        0x0004

#define TCKEY_MODE_EAVS            0x0001
#define TCKEY_MODE_PERM0           0x0004
#define TCKEY_MODE_PERM1           0x0006
#define TCKEY_MODE_ClearSCB        0x0010
#define TCKEY_MODE_MddnotMdi       0x0060
/* ------------------------------------------------------------------------- */

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCDMAConfig_t)
#endif

typedef volatile struct TCDMAConfig_s
{
    U32 DMABase_p;
    U32 DMATop_p;
    U32 DMAWrite_p;
    U32 DMARead_p;
    U32 DMAQWrite_p;
    U32 BufferPacketCount;

    U16 SignalModeFlags;
    U16 Threshold;
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    U16 BufferLevelThreshold;
    U16 DMAConfig_Spare;
#endif
} TCDMAConfig_t;


#define TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE    0x1
#define TC_DMA_CONFIG_SIGNAL_MODE_TYPE_MASK               0xe
#define TC_DMA_CONFIG_SIGNAL_MODE_TYPE_NO_SIGNAL          0x0
#define TC_DMA_CONFIG_SIGNAL_MODE_TYPE_QUANTISATION       0x2
#define TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS           0x4
#define TC_DMA_CONFIG_SIGNAL_MODE_SWCDFIFO                0x8
#define TC_DMA_CONFIG_OUTPUT_WITHOUT_META_DATA            0x10
#define TC_DMA_CONFIG_WINDBACK_ON_ERROR                   0x20

/* ------------------------------------------------------------------------- */
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCInterruptDMAConfig_t) 
#endif
typedef volatile struct TCInterruptDMAConfig_s 
{
    U32 DMABase_p;
    U32 DMATop_p;
    U32 DMAWrite_p;
    U32 DMARead_p;
} TCInterruptDMAConfig_t;


/* ------------------------------------------------------------------------- */


typedef struct StartCode_s 
{
    U8 Offset;
    U8 Code;
} StartCode_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct TCStatus_s 
{
    U32 Flags;
    
    U32 SlotError:8;
    U32 SlotNumber:8;
    U32 Odd_Even:1;
    U32 PESScrambled:1;
    U32 Scrambled:1;
    U32 Padding:13;
    
    U32 ArrivalTime0:16;
    U32 ArrivalTime1:16;
    
    U32 ArrivalTime2:16;
    U32 DMACtrl:16;
    
    U32 Pcr0:16;
    U32 Pcr1:16;
    
    U32 Pcr2:16;
    U32 NumberStartCodes:8;
    U32 PayloadLength:8;            /* Only valid if StartCodeDetection Enabled */
    
    U32 BufferPacketNumber;
    U32 BufferPacketAddress;
    U32 RecordBufferPacketNumber;

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    U32 StartCodePreviousBPN:8;
    U32 StartCodePreviousRecordBPN:8;
    StartCode_t StartCodes[STPTI_MAX_START_CODES_SUPPORTED];
#endif
        
} TCStatus_t;


/* Bit Field definitions for Status.Flags */

#define STATUS_FLAGS_TRANSPORT_ERROR              0x00000001
#define STATUS_FLAGS_SECTION_CRC_ERROR            0x00000002
#define STATUS_FLAGS_INVALID_DESCRAMBLE_KEY       0x00000004
#define STATUS_FLAGS_INVALID_PARAMETER            0x00000008
#define STATUS_FLAGS_INVALID_CC                   0x00000010
#define STATUS_FLAGS_TC_CODE_FAULT                0x00000020
#define STATUS_FLAGS_PACKET_SIGNAL                0x00000040
#define STATUS_FLAGS_PES_ERROR                    0x00000080

#define STATUS_FLAGS_DMA_COMPLETE                 0x00000100
#define STATUS_FLAGS_SUBSTITUTE_COMPLETE          0x00000400
#define STATUS_FLAGS_RECORD_BUFFER_OVERFLOW       0x00000800
#define STATUS_FLAGS_CWP_FLAG                     0x00002000    /* (DSS only) */
#define STATUS_FLAGS_INVALID_LINK                 0x00004000
#define STATUS_FLAGS_BUFFER_OVERFLOW              0x00008000

#define STATUS_FLAGS_ADAPTATION_EXTENSION_FLAG    0x00010000    /* (DVB only) */
#define STATUS_FLAGS_PRIVATE_DATA_FLAG            0x00020000    /* (DVB only) */
#define STATUS_FLAGS_SPLICING_POINT_FLAG          0x00040000    /* (DVB only) */
#define STATUS_FLAGS_OPCR_FLAG                    0x00080000    /* (DVB only) */
#define STATUS_FLAGS_PCR_FLAG                     0x00100000
#define STATUS_FLAGS_PRIORITY_INDICATOR           0x00200000    /* (DVB only) */
#define STATUS_FLAGS_RANDOM_ACCESS_INDICATOR      0x00400000    /* (DVB only) */
#define STATUS_FLAGS_CURRENT_FIELD_FLAG           0x00400000    /* (DSS only) */
#define STATUS_FLAGS_DISCONTINUITY_INDICATOR      0x00800000    /* (DVB only) */
#define STATUS_FLAGS_MODIFIABLE_FLAG              0x00800000    /* (DSS only) */

#define STATUS_FLAGS_START_CODE_FLAG              0x01000000    /* (DVB only) */
#define STATUS_FLAGS_TIME_CODE                    0x01000000    /* (DSS only) */
#define STATUS_FLAGS_PUSI_FLAG                    0x02000000    /* (DVB only) */
#define STATUS_FLAGS_SCRAMBLE_CHANGE              0x04000000
#define STATUS_FLAGS_FIRST_RECORDED_PACKET        0x08000000
#define STATUS_FLAGS_BUNDLE_BOUNDARY              0x10000000    /* (DSS only) */
#define STATUS_FLAGS_AUXILIARY_PACKET             0x20000000    /* (DSS only) */
#define STATUS_FLAGS_SESSION_NUMBER_MASK          0x30000000    /* (DVB only) */
#define STATUS_FLAGS_PACKET_SIGNAL_RECORD_BUFFER  0x40000000
#define STATUS_FLAGS_INDX_TIMEOUT_TICK            0x80000000    /* Only used for STPTI_TIMER_TICK feature */



/* ------------------------------------------------------------------------- */
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCTransportFilter_t)
#endif

typedef volatile struct TCTransportFilter_s
{
    U16 DataMode;
    U16 MaskWord;
} TCTransportFilter_t;


/* ------------------------------------------------------------------------- */

/*
TODO remove!!!
*/
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCPESFilter_t)
#endif
typedef volatile struct TCPESFilter_s 
{
    U16 FlagsData;
    U16 FlagsMask;
    U16 PESData;
    U16 PESDataUnused;
    U32 DTSMinLSW;
    U32 DTSMaxLSW;
    U32 PTSMinLSW;
    U32 PTSMaxLSW;        
    U16 Bit32;           /* 15-4 : unused      */
                         /*    3 : PTSMaxBit32 */
                         /*    2 : PTSMinBit32 */ 
                         /*    1 : DTSMaxBit32 */ 
                         /*    0 : DTSMinBit32 */
    U16 MorePacking;
} TCPESFilter_t;


/* ------------------------------------------------------------------------- */

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCCarousel_t)
#endif

typedef volatile struct
{
    U16 CarouselControl;
    U16 MinTime_0;

    U16 MinTime_1;
    U16 TCReading;

    U32 CarouselData[188/sizeof(U32)];
} TCCarousel_t;



typedef volatile struct TCSCDFilterEntry_s
{
    U16 SCD_SlotState;                  /* TC Written only (except for reset) */
    U16 SCD_LastPacketsBufferCount;     /* TC Written only (except for reset) */
    U16 SCD_SlotStartCode;              /* host writable B7:0 StartCodeByte, B15:8 SCD_NextIndex */
    U16 SCD_SlotMask;                   /* host writable (B15 =0 MPEG2, =1 H264) */
} TCSCDFilterEntry_t;

/* ------------------------------------------------------------------------- */

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCDevice_t)
#endif

typedef volatile struct TCDevice_s 
{
    U32 PTIIntStatus0;
    U32 PTIIntStatus1;
    U32 PTIIntStatus2;
    U32 PTIIntStatus3;

    U32 PTIIntEnable0;
    U32 PTIIntEnable1;
    U32 PTIIntEnable2;
    U32 PTIIntEnable3;

    U32 PTIIntAck0;
    U32 PTIIntAck1;
    U32 PTIIntAck2;
    U32 PTIIntAck3;

    U32 TCMode;

    U32 DMAempty_STAT;   /* 3 bits RO */
    U32 DMAempty_EN;     /* 3 bits RW */

    U32 TCPadding_0;

    U32 PTIAudPTS_31_0;
    U32 PTIAudPTS_32;

    U32 PTIVidPTS_31_0;
    U32 PTIVidPTS_32;

    U32 STCTimer0;
    U32 STCTimer1;

    U32 TCPadding_1[(0x1000 - 22 * sizeof(U32)) / sizeof(U32)];

    U32 DMA0Base;
    U32 DMA0Top;
    U32 DMA0Write;
    U32 DMA0Read;
    U32 DMA0Setup;
    U32 DMA0Holdoff;
    U32 DMA0Status;
    U32 DMAEnable;
    
    U32 DMA1Base;
    U32 DMA1Top;
    U32 DMA1Write;
    U32 DMA1Read;
    U32 DMA1Setup;
    U32 DMA1Holdoff;
    U32 DMA1CDAddr;
    U32 DMASecStart;

    U32 DMA2Base;
    U32 DMA2Top;
    U32 DMA2Write;
    U32 DMA2Read;
    U32 DMA2Setup;
    U32 DMA2Holdoff;
    U32 DMA2CDAddr;
    U32 DMAFlush;

    U32 DMA3Base;
    U32 DMA3Top;
    U32 DMA3Write;
    U32 DMA3Read;
    U32 DMA3Setup;
    U32 DMA3Holdoff;
    U32 DMA3CDAddr;
    U32 DMAPTI3Prog;

    U32 TCPadding_2[(0x2000 - (0x1000 + 32 * sizeof(U32))) / sizeof(U32)];

    U32 TCPadding_4[(0x20e0 - 0x2000) / sizeof(U32)];

    U32 IIFCAMode;

    U32 TCPadding_5[(0x4000 - (0x2000 + 57 * sizeof(U32))) / sizeof(U32)];

    TCSectionFilterArrays_t TC_SectionFilterArrays; /* std or ram cam */

    U32 TCPadding_6[(0x6000 - (0x4000 + sizeof(TCSectionFilterArrays_t))) / sizeof(U32)];

    U32 IIFFIFOCount;
    U32 IIFAltFIFOCount;
    U32 IIFFIFOEnable;
    U32 TCPadding_3[1];
    U32 IIFAltLatency;
    U32 IIFSyncLock;
    U32 IIFSyncDrop;
    U32 IIFSyncConfig;
    U32 IIFSyncPeriod;

    U32 TCPadding_7[(0x7000 - (0x6000 + 9 * sizeof(U32))) / sizeof(U32)];

    U32 TCRegA;
    U32 TCRegB;
    U32 TCRegC;
    U32 TCRegD;
    U32 TCRegP;
    U32 TCRegQ;
    U32 TCRegI;
    U32 TCRegO;
    U32 TCIPtr;
    U32 TCRegE0;
    U32 TCRegE1;
    U32 TCRegE2;
    U32 TCRegE3;
    U32 TCRegE4;
    U32 TCRegE5;
    U32 TCRegE6;
    U32 TCRegE7;    
                
    U32 TCPadding_8[(0x8000 - (0x7000 + 17 * sizeof(U32))) / sizeof(U32)];                                

    U32 TC_Data[TC_DATA_RAM_SIZE / sizeof(U32)];

    U32 TCPadding_9[(0xC000 - (0x8000 + TC_DATA_RAM_SIZE)) / sizeof(U32)];

    U32 TC_Code[TC_CODE_RAM_SIZE / sizeof(U32)];
} TCDevice_t;


#define IIF_SYNC_CONFIG_USE_INTERNAL_SYNC   0x00
#define IIF_SYNC_CONFIG_USE_SOP             0x01

/* ------------------------------------------------------------------------- */


/* seen flags apply to 'SeenStatus' in Private data on TC, ( differnt from TC1 ) */

#define TC_SEEN_PACKET			            0x01
#define TC_SEEN_TRANSPORT_SCRAMBLED_PACKET 0x02
#define TC_SEEN_PES_SCRAMBLED_PACKET	    0x04


/* Exported Macros ----------------------------------------------------- */


#define GetActionMask(p, x) GetTCData((p), (x))
#define PutActionMask(p, x) PutTCData((p), (x))


/* -------------------------------------------- */
/* --- This section contains interrupt      --- */
/* --- where slot interrupts are 0..47      --- */
/* --- and others are 48..63                --- */
/* -------------------------------------------- */


#define TC_INTERRUPT_TRANSPORT_ERROR		    0
#define TC_INTERRUPT_CRC_ERROR			        1
#define TC_INTERRUPT_INVALID_DESCRAMBLE_KEY	    2
#define TC_INTERRUPT_INVALID_LINK		        3
#define TC_INTERRUPT_INVALID_PARAMETER		    4
#define TC_INTERRUPT_CC_ERROR			        5
#define TC_INTERRUPT_BUFFER_OVERFLOW		    6
#define TC_INTERRUPT_TC_CODE_FAULT		        7
#define TC_INTERRUPT_PCR_READY			        8

#define TC_INTERRUPT_SUBSTITUTION_DONE		    9

#define TC_ALL_INTERRUPTS			            0x03ff
#define TC_MAXIMUM_GENERAL_INTERRUPTS		    10


/* ------------------------------------------------ */
/* --- Macro's to access the main info table	--- */
/* --- 						                    --- */
/* --- Map of status words          			--- */
/* --- 						                    --- */
/* ---	   SloteState   		                --- */
/* --- 	   15..11	SlotStateUnused		        --- */
/* ---     10..9	PESState    	        	--- */
/* --- 	   8		PayloadPresent      	    --- */
/* --- 	   7..4		LastCC       	            --- */
/* --- 	   3		TSMatched		            --- */
/* --- 	   2..0		ScrambledState              --- */
/* --- 						                    --- */
/* ---	   SlotMode				                --- */
/* ---     11..15	SlotModeUnused   	        --- */
/* ---	   10..9	BackBufferDMA		        --- */
/* ---     8		SubstituteStream     	    --- */
/* ---	   7		InsertErrorOnCCFail	        --- */
/* ---	   6		IgnoreScrambling    	    --- */
/* ---	   5..4		AltOutMode          	    --- */
/* ---	   3..0		SlotType        	        --- */
/* --- 						                    --- */
/* ---	   CPU 1				                --- */
/* ---						                    --- */
/* ---	   15..4	SPARE			            --- */
/* --- 	   3..0		Alt out tag		            --- */
/* ---	NOTE alt out assumes only tag in cpu 1	--- */
/* ---						                    --- */
/* ------------------------------------------------ */

/* general bits of SlotMode word */

#define TC_SLOT_TYPE_NULL                       0x0000
#define TC_SLOT_TYPE_SECTION                    0x0001
#define TC_SLOT_TYPE_PES                        0x0002
#define TC_SLOT_TYPE_RAW                        0x0003
#define TC_SLOT_TYPE_EMM                        0x0005
#define TC_SLOT_TYPE_ECM                        0x0006
#define TC_SLOT_TYPE_DC2_PRIVATE                0x000E
#define TC_SLOT_TYPE_DC2_MIXED                  0x000F


/* bits for CarouselControl */
#define TC_CAROUSEL_ENTRY_READY                 0x1
#define TC_CAROUSEL_TIMED_ENTRY                 0x2                 
#define TC_CAROUSEL_OUTPUT_PTI_SESSION          0xC
#define TC_CAROUSEL_CPU_WRITING                 0x10
#define TC_CAROUSEL_OUTPUT_AS_DLNA_PACKET       0x20
#define TC_CAROUSEL_MASK_PERMANENT_FLAGS		0x0020	/* update this when new permanent flags are added. currently (DLNA flag)*/

/* bits for CarouselData3 */
#define TC_CAROUSEL_TC_READING                  0x1

/* ------------------------------------------------------------------------- */


#define ALTERNATE_OUTPUT_TYPE_NO_OUTPUT         0x0000
#define ALTERNATE_OUTPUT_TYPE_OUTPUT_AS_IS      0x0010
#define ALTERNATE_OUTPUT_TYPE_DESCRAMBLED       0X0020
#define SLOT_MODE_IGNORE_SCRAMBLING             0x0040  /* DVB only */
#define TC_SLOT_MODE_APPEND_SYNC_BYTE           0x0040  /* DTV only - reusing SLOT_MODE_IGNORE_SCRAMBLING */
#define SLOT_MODE_INSERT_ERROR_ON_CC_FAIL       0x0080
#define SLOT_MODE_SUBSTITUTE_STREAM             0x0100


/* BackbufferDMA types in here*/

/* general bits of SlotState word*/

#define SLOT_STATE_TS_MATCHED                  0x0008
#define SLOT_STATE_PAYLOAD_PRESENT             0x0100
#define SLOT_STATE_INITIAL_SCRAMBLE_STATE      0x4000 


/* ------------------------------------------------------------------------- */


/* the following is used for DC2Filter Types */
#ifdef STPTI_DC2_SUPPORT
#define MULTICAST16_FILTER_MODE                 0x0000
#define MULTICAST32_FILTER_MODE                 0x0001
#define MULTICAST48_FILTER_MODE                 0X0002
#endif



#endif  /* __PTI4_H */


/* ------------------------------------------------------------------------- */

/* Used as an error return value when searching for Sessions */
#define SESSION_ILLEGAL_INDEX                   0xFFFF

/* EOF */
