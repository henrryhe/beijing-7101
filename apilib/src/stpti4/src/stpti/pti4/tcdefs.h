/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: tcdefs.h
 Description: 

******************************************************************************/

#ifndef _TCDEFS_H_
#define _TCDEFS_H_


#ifndef _STPTI_DEVICE_PTR_T_
#define _STPTI_DEVICE_PTR_T_

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(STPTI_DevicePtr_t)
#endif

typedef volatile U32 *STPTI_DevicePtr_t;

#endif /* #endifndef _STPTI_DEVICE_PTR_T_ */

typedef struct

{
    STPTI_DevicePtr_t TC_CodeStart;
    size_t            TC_CodeSize;
    STPTI_DevicePtr_t TC_DataStart;

    STPTI_DevicePtr_t TC_LookupTableStart;
    STPTI_DevicePtr_t TC_GlobalDataStart;
    STPTI_DevicePtr_t TC_StatusBlockStart;
    STPTI_DevicePtr_t TC_MainInfoStart;
    STPTI_DevicePtr_t TC_DMAConfigStart;
    STPTI_DevicePtr_t TC_DescramblerKeysStart;
    STPTI_DevicePtr_t TC_TransportFilterStart;
    STPTI_DevicePtr_t TC_SCDFilterTableStart;
    STPTI_DevicePtr_t TC_PESFilterStart;
    STPTI_DevicePtr_t TC_SubstituteDataStart;
    STPTI_DevicePtr_t TC_SystemKeyStart;    /* Descambler support */
    STPTI_DevicePtr_t TC_SFStatusStart;
    STPTI_DevicePtr_t TC_InterruptDMAConfigStart;
    STPTI_DevicePtr_t TC_EMMStart;
    STPTI_DevicePtr_t TC_ECMStart;              /* Possibly redundant */
    STPTI_DevicePtr_t TC_MatchActionTable;
    STPTI_DevicePtr_t TC_SessionDataStart;
    STPTI_DevicePtr_t TC_VersionID;

    U16               TC_NumberCarousels;
    U16               TC_NumberSystemKeys;    /* Descambler support */
    U16               TC_NumberDMAs;
    U16               TC_NumberDescramblerKeys;
    U16               TC_SizeOfDescramblerKeys;
    U16               TC_NumberIndexs;
    U16               TC_NumberPesFilters;
    U16               TC_NumberSectionFilters;
    U16               TC_NumberSlots;
    U16               TC_NumberTransportFilters;
    U16               TC_NumberEMMFilters;
    U16               TC_NumberECMFilters;
    U16               TC_NumberOfSessions;
    U16               TC_NumberSCDFilters;

    BOOL              TC_AutomaticSectionFiltering;
    BOOL              TC_MatchActionSupport;
    BOOL              TC_SignalEveryTransportPacket;

} STPTI_TCParameters_t;

#ifdef STPTI_DEBUG_SUPPORT
#define TC_DebugStart TC_SubstituteDataStart
#endif

#endif /* __TCDEFS_H */
