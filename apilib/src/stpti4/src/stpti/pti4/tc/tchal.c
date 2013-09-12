/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: tchal.c
 Description: Provides HAL interface to transport controller

******************************************************************************/


/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif


/* ------------------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <assert.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "cam.h"
#include "tchal.h"

#include "pti4.h"


/* ------------------------------------------------------------------------- */


#define TC_DSRAM_BASE 0x8000


/* ------------------------------------------------------------------------- */


TCMainInfo_t *TcHal_GetMainInfo(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber)
{
    return( &((TCMainInfo_t *)STPTI_TCParameters_p->TC_MainInfoStart)[SlotNumber] );
}


/* ------------------------------------------------------------------------- */


#if !defined ( STPTI_BSL_SUPPORT )
TCPESFilter_t *TcHal_GetPESFilter(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 PesFilterNumber)
{
    return( &((TCPESFilter_t *)STPTI_TCParameters_p->TC_PESFilterStart)[PesFilterNumber] );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/* ------------------------------------------------------------------------- */


/* for an existing slot & dma */

void TcHal_MainInfoAssociateDmaWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, U16 DmaNumber)
{
    U16 DMACtrl_indices;
    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( STPTI_TCParameters_p, SlotNumber );

    DMACtrl_indices  = STSYS_ReadRegDev16LE((void*)&MainInfo_p->DMACtrl_indices);
    DMACtrl_indices &= 0xFF00;            /* mask off main buffer index */
    DMACtrl_indices |= DmaNumber&0x00FF;  /* or in main buffer index */
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->DMACtrl_indices, DMACtrl_indices);
}

void TcHal_MainInfoUnlinkDmaWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber)
{
    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( STPTI_TCParameters_p, SlotNumber );
    STSYS_SetTCMask16LE((void*)&MainInfo_p->DMACtrl_indices, 0x00FF);       /* Mark main buffer DMAControl index as invalid */
}

/* ------------------------------------------------------------------------- */

#if !defined ( STPTI_BSL_SUPPORT )
void TcHal_MainInfoAssociateRecordDmaWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, U8 DmaNumber)
{
    U16 DMACtrl_indices;
    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( STPTI_TCParameters_p, SlotNumber );

    DMACtrl_indices  = STSYS_ReadRegDev16LE((void*)&MainInfo_p->DMACtrl_indices);
    DMACtrl_indices &= 0x00FF;            /* mask off record buffer index */
    DMACtrl_indices |= DmaNumber<<8;   /* or in record buffer index */
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->DMACtrl_indices, DMACtrl_indices);
}

void TcHal_MainInfoUnlinkRecordDmaWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber)
{
    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( STPTI_TCParameters_p, SlotNumber );
    STSYS_SetTCMask16LE((void*)&MainInfo_p->DMACtrl_indices, 0xFF00);       /* Mark record buffer DMAControl index as invalid */
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* ------------------------------------------------------------------------- */

TCGlobalInfo_t *TcHal_GetGlobalInfo( STPTI_TCParameters_t *STPTI_TCParameters_p )
{
    return( (TCGlobalInfo_t *)STPTI_TCParameters_p->TC_GlobalDataStart );
}


/* ------------------------------------------------------------------------- */


void TcHal_LookupTableSetPidForSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, STPTI_Pid_t Pid)
{
    PutTCData( &((U16 *)STPTI_TCParameters_p->TC_LookupTableStart)[SlotNumber], Pid );
}


/* ------------------------------------------------------------------------- */


#if !defined ( STPTI_BSL_SUPPORT )
void TcHal_LookupTableGetPidForSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, STPTI_Pid_t *Pid)
{
    GetTCData( &((U16 *)STPTI_TCParameters_p->TC_LookupTableStart)[SlotNumber], *Pid )
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* ------------------------------------------------------------------------- */


TCDMAConfig_t *TcHal_GetTCDMAConfig( STPTI_TCParameters_t *STPTI_TCParameters_p, U16 DmaNumber)
{
    return( &((TCDMAConfig_t *)STPTI_TCParameters_p->TC_DMAConfigStart)[DmaNumber] );
}


/* ------------------------------------------------------------------------- */

#if !defined ( STPTI_BSL_SUPPORT ) 
TCKey_t *TcHal_GetDescramblerKey( STPTI_TCParameters_t *STPTI_TCParameters_p, U16 KeyNumber)
{
    return(
    
    (TCKey_t *)
    
    ((U32)(&(STPTI_TCParameters_p->TC_DescramblerKeysStart)[0]) + (KeyNumber * STPTI_TCParameters_p->TC_SizeOfDescramblerKeys))
    
    );
}


/* ------------------------------------------------------------------------- */

TCSystemKey_t *TcHal_GetSystemKey( STPTI_TCParameters_t *STPTI_TCParameters_p, U16 KeyNumber)
{
    /* SystemKey is using the same memory than Carousel but there is no change in the structure for it for the moment */
    /* this has to be change in future */
    /* TO BE CHANGED */
    return( &((TCSystemKey_t *)STPTI_TCParameters_p->TC_SystemKeyStart)[0 /* Temporary fix TN investigating was KeyNumber */ ] );
}


/* ------------------------------------------------------------------------- */

/* on entry: full ST20/40 address for TCKey_p */

void TcHal_MainInfoAssociateDescramblerPtrWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, TCKey_t *TCKey_p)
{
    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( STPTI_TCParameters_p, SlotNumber );

    /* convert ST20.40 address in TCKey_p to one that the TC understands */
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->DescramblerKeys_p,(U32) ( (U8 *)TCKey_p - (U8 *)STPTI_TCParameters_p->TC_DataStart + (U8 *)TC_DSRAM_BASE ));
}


/* ------------------------------------------------------------------------- */

/* on entry: full ST20/40 address for TCPESFilter_p */

void TcHal_MainInfoAssociatePesFilterPtrWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, TCPESFilter_t *TCPESFilter_p)
{
    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( STPTI_TCParameters_p, SlotNumber );

    /* convert ST20.40 address in TCPESFilter_p to one that the TC understands */
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->SectionPesFilter_p,(U32) ( (U8 *)TCPESFilter_p - (U8 *)STPTI_TCParameters_p->TC_DataStart + (U8 *)TC_DSRAM_BASE ));
}


/* ------------------------------------------------------------------------- */


void TcHal_MainInfoDisassociatePesFilterWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber)
{
    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( STPTI_TCParameters_p, SlotNumber );

    STSYS_WriteTCReg16LE((void*)&MainInfo_p->SectionPesFilter_p,TC_INVALID_LINK);
}



/* ------------------------------------------------------------------------- */


void TcHal_MainInfoDisassociateDescramblerWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber)
{
    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( STPTI_TCParameters_p, SlotNumber );

    assert(STSYS_ReadRegDev16LE((void*)&MainInfo_p->DescramblerKeys_p) != TC_INVALID_LINK);   /* why? - GJP */
    STSYS_WriteTCReg16LE((U32)&MainInfo_p->DescramblerKeys_p,TC_INVALID_LINK);
}

#endif /* #if !defined (STPTI_BSL_SUPPORT ) */

/* EOF  -------------------------------------------------------------------- */
