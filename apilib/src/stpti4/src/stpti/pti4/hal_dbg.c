/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: hal_dbg.c
 Description: debugging functions. 

******************************************************************************/

#if !defined(ST_OSLINUX)
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#endif

#include "stddefs.h"
#include "stdevice.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#include "pti4.h"
#include "tchal.h"
#include "memget.h"
#include "stpti.h"


/* Private Variables ------------------------------------------------------- */
#ifdef STPTI_DEBUG_SUPPORT
STPTI_DebugInterruptStatus_t *DebugInterruptStatus = NULL;
STPTI_DebugStatistics_t      DebugStatistics = {0};

int  IntInfoCapacity = 0;
BOOL IntInfoStart = FALSE;
int  IntInfoIndex =0;
#endif



int stptiHAL_read_proctcdata( char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int len = 0;
    static int cur_pos = 0;
    int limit = count - 64;
    U16 *tcdata_p;
    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCPrivateData_t     *TCPrivateData_p;
    U8                  *tc_version_string;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    TCPrivateData_p = (TCPrivateData_t*)&Device_p->TCPrivateData;       

    tcdata_p = (U16*)((TCDevice_t *)Device_p->TCDeviceAddress_p)->TC_Data;
    tc_version_string = (U8*) TCPrivateData_p->TC_Params.TC_VersionID;
    
    if( offset == 0 )
    {
        cur_pos = 0;
    
        len += sprintf( buf+len,"; PTI_VERSION: %s\n", STPTI_REVISION );
        len += sprintf( buf+len,";  TC_VERSION: %c%c%c%c%c%c ", tc_version_string[1], tc_version_string[0], tc_version_string[3], tc_version_string[2], tc_version_string[5], tc_version_string[4] );
        len += sprintf( buf+len,"%x ", (signed) (tc_version_string[9]<<8 | tc_version_string[8]) );
        len += sprintf( buf+len,"(TC_ID is %d)", (signed) (tc_version_string[7]<<8 | tc_version_string[6]) );
    }

    while( (cur_pos<(TC_DATA_RAM_SIZE/sizeof(U16))) && (len<limit) ){
        if(cur_pos%8 == 0)
           len += sprintf( buf+len,"\n#%08x  ",(int)(tcdata_p+cur_pos) );
        len += sprintf( buf+len,"%04x ", (U32) *(tcdata_p+cur_pos) );

        cur_pos++;
    }

    *start = buf;
    if( cur_pos == (TC_DATA_RAM_SIZE/sizeof(U16)) )
    {
        if(len>0)
        {
            len +=  sprintf( buf+len, "\n" );
        }
        else
        {
            /* Not sure why we need to do this, but we found that for some kernels eof is ignored unless */
            /* start equals NULL ? */
            *start = NULL;
        }
        *eof = 1;
    }
    else
    {
        *eof = 0;
    }
    
    return len;
}

#define PRINTED_LENGTH  126

/* Display tcparams info */
int stptiHAL_read_proc_cam( char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int len = 0;
    int        line;
    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCSectionFilterArrays_t *cam;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    
#if defined(ST_5528)
    /***** STD Cam **************************************************************/
    
    *eof = 1;
#else
    /***** RAM CAM **************************************************************/
    cam  = (TCSectionFilterArrays_t*)(Device_p->TCDeviceAddress_p + 0x4000/4);
    
    for (line = offset / PRINTED_LENGTH;
            (line < (SF_NUM_BLOCKS_PER_CAM*SF_CAMLINE_WIDTH));
                line++)
    {
        if ((count - len) < PRINTED_LENGTH)
        {
            /* Out of space */
            break;
        }
        else
        {
            int i;
            
            if(line % SF_CAMLINE_WIDTH == 0 )
            {
                len+=sprintf(buf+len,"%04x : ", (U32)(cam->CamA_Block + (line / SF_CAMLINE_WIDTH)) & 0XFFFF);
            }
            else
            {
                len+=sprintf(buf+len,"       ");
            }

            /* CAM A Data */
            for (i = 0; (i < SF_FILTER_LENGTH); i++)
            {
                len+=sprintf(buf+len,"%02x ", cam->CamA_Block[line / SF_CAMLINE_WIDTH].Index[i].Data.Element.Filter[line % SF_CAMLINE_WIDTH]);
            }

            len+=sprintf(buf+len,": ");

            /* CAM B Data */
            for (i = 0; (i < SF_FILTER_LENGTH); i++)
            {
                len+=sprintf(buf+len,"%02x ", cam->CamB_Block[line / SF_CAMLINE_WIDTH].Index[i].Data.Element.Filter[line % SF_CAMLINE_WIDTH]);
            }

            len+=sprintf(buf+len,"\n       ");


            /* CAM A Mask */
            for (i = 0; (i < SF_FILTER_LENGTH); i++)
            {
                len+=sprintf(buf+len,"%02x ", cam->CamA_Block[line / SF_CAMLINE_WIDTH].Index[i].Mask.Element.Filter[line % SF_CAMLINE_WIDTH]);
            }

            len+=sprintf(buf+len,": ");

            /* CAM B Mask */
            for (i = 0; (i < SF_FILTER_LENGTH); i++)
            {
                len+=sprintf(buf+len,"%02x ", cam->CamB_Block[line / SF_CAMLINE_WIDTH].Index[i].Mask.Element.Filter[line % SF_CAMLINE_WIDTH]);
            }

            len+=sprintf(buf+len,": %08X\n", (line < TC_NUMBER_OF_HARDWARE_NOT_FILTERS)?(cam->NotFilter[line]):(0));
        }
    }
        
    *start = buf;
    if(line == (SF_NUM_BLOCKS_PER_CAM*SF_CAMLINE_WIDTH))
    {
        *eof = 1;
    }
    else
    {
        *eof = 0;
    }
    
    /* Some kernels seem to ignore eof unless start is NULL ? */
    if( (*eof == 1) && len==0)
    {
        start = NULL;
    }
#endif
    
    return len;
}


/*=============================================================================

   stptiHAL_read_procregs

   Print out all the PTI registers

  ===========================================================================*/
int stptiHAL_read_procregs( char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int len = 0, i;
    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCDevice_t          *addr_p;
    U32                  iptr;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    addr_p =        (TCDevice_t*)Device_p->TCDeviceAddress_p;
    
    len+=sprintf(buf+len,"TCDevice_t @ address 0x%08x:\n",(int)addr_p);

    len+=sprintf(buf+len,"PTIIntStatus0:0x%08x   PTIIntStatus1:0x%08x   PTIIntStatus2:0x%08x   PTIIntStatus3:0x%08x\n",
                 addr_p->PTIIntStatus3,addr_p->PTIIntStatus2,
                 addr_p->PTIIntStatus1,addr_p->PTIIntStatus0);

    len+=sprintf(buf+len,"PTIIntEnable0:0x%08x   PTIIntEnable1:0x%08x   PTIIntEnable2:0x%08x   PTIIntEnable3:0x%08x\n",
                 addr_p->PTIIntEnable0,addr_p->PTIIntEnable1,
                 addr_p->PTIIntEnable2,addr_p->PTIIntEnable3);

    len+=sprintf(buf+len,"TCMode       :0x%08x   DMAempty_STAT:0x%08x   DMAempty_EN  :0x%08x   TCPaddng_0   :0x%08x\n\n",
                 addr_p->TCMode,addr_p->DMAempty_STAT,
                 addr_p->DMAempty_EN,addr_p->TCPadding_0);

    len+=sprintf(buf+len,"PTIAudPTS_31_0: 0x%08x   PTIAudPTS_32:0x%08x\n",
                 addr_p->PTIAudPTS_31_0,addr_p->PTIAudPTS_32);

    len+=sprintf(buf+len,"PTIVidPTS_31_0: 0x%08x   PTIVidPTS_32:0x%08x\n\n",
                 addr_p->PTIVidPTS_31_0,addr_p->PTIVidPTS_32);

    len+=sprintf(buf+len,"STCTimer0     : 0x%08x   STCTimer1   :0x%08x\n\n",
                 addr_p->STCTimer0,addr_p->STCTimer1);

    len+=sprintf(buf+len,"DMAs:\n");
    len+=sprintf(buf+len,"0 - Base:0x%08x Top:0x%08x Write:0x%08x Read:0x%08x Setup:0x%08x DMA0Holdoff:0x%08x Status:0x%08x\n",
                 addr_p->DMA0Base,addr_p->DMA0Top,
                 addr_p->DMA0Write,addr_p->DMA0Read,
                 addr_p->DMA0Setup,addr_p->DMA0Holdoff,
                 addr_p->DMA0Status);

    len+=sprintf(buf+len,"1 - Base:0x%08x Top:0x%08x Write:0x%08x Read:0x%08x Setup:0x%08x DMA0Holdoff:0x%08x CDAddr:0x%08x\n",
                 addr_p->DMA1Base,addr_p->DMA1Top,
                 addr_p->DMA1Write,addr_p->DMA1Read,
                 addr_p->DMA1Setup,addr_p->DMA1Holdoff,
                 addr_p->DMA1CDAddr);

    len+=sprintf(buf+len,"2 - Base:0x%08x Top:0x%08x Write:0x%08x Read:0x%08x Setup:0x%08x DMA0Holdoff:0x%08x CDAddr:0x%08x\n",
                 addr_p->DMA2Base,addr_p->DMA2Top,
                 addr_p->DMA2Write,addr_p->DMA2Read,
                 addr_p->DMA2Setup,addr_p->DMA2Holdoff,
                 addr_p->DMA2CDAddr);

    len+=sprintf(buf+len,"3 - Base:0x%08x Top:0x%08x Write:0x%08x Read:0x%08x Setup:0x%08x DMA0Holdoff:0x%08x CDAddr:0x%08x\n\n",
                 addr_p->DMA3Base,addr_p->DMA3Top,
                 addr_p->DMA3Write,addr_p->DMA3Read,
                 addr_p->DMA3Setup,addr_p->DMA3Holdoff,
                 addr_p->DMA3CDAddr);

    len+=sprintf(buf+len,"Enable:0x%08x   SecStart:0x%08x   Flush:0x%08x   PTI3Prog:0x%08x\n\n",
                 addr_p->DMAEnable,addr_p->DMASecStart,
                 addr_p->DMAFlush,addr_p->DMAPTI3Prog);

    len+=sprintf(buf+len,"IIFCAMode :0x%08x\n",addr_p->IIFCAMode);
    
    len+=sprintf(buf+len,"IIF 0x%08x\n",(U32)&addr_p->IIFFIFOCount);

    len+=sprintf(buf+len,"FIFOCount :0x%08x AltFIFOCount:0x%08x FIFOEnable:0x%08x\n",
                 addr_p->IIFFIFOCount,
                 addr_p->IIFAltFIFOCount,addr_p->IIFFIFOEnable);

    len+=sprintf(buf+len,"AltLatency:0x%08x   SyncLock    :0x%08x   SyncDrop  :0x%08x   SyncConfig:0x%08x\n",
                 addr_p->IIFAltLatency,addr_p->IIFSyncLock,
                 addr_p->IIFSyncDrop,addr_p->IIFSyncConfig);

    len+=sprintf(buf+len,"SyncPeriod:0x%08x\n\n",addr_p->IIFSyncPeriod);

    len+=sprintf(buf+len,"TCRegA :0x%04x   TCRegB :0x%04x   TCRegC :0x%04x   TCRegD :0x%04x     ",
                 addr_p->TCRegA,addr_p->TCRegB,
                 addr_p->TCRegC,addr_p->TCRegD);

    len+=sprintf(buf+len,"TCRegP :0x%04x   TCRegQ :0x%04x   TCRegI :0x%04x   TCRegO :0x%04x\n",
                 addr_p->TCRegP,addr_p->TCRegQ,
                 addr_p->TCRegI,addr_p->TCRegO);

    len+=sprintf(buf+len,"TCRegE0:0x%04x   TCRegE1:0x%04x   TCRegE2:0x%04x   TCRegE3:0x%04x     ",
                 addr_p->TCRegE0,addr_p->TCRegE1,
                 addr_p->TCRegE2,addr_p->TCRegE3);

    len+=sprintf(buf+len,"TCRegE4:0x%04x   TCRegE5:0x%04x   TCRegE6:0x%04x   TCRegE7:0x%04x\n",
                 addr_p->TCRegE4,addr_p->TCRegE5,
                 addr_p->TCRegE6,addr_p->TCRegE7);

    iptr=addr_p->TCIPtr;
    len+=sprintf(buf+len,"TCIPtr :0x%04x ", iptr);
    for(i=8;i>0;i--)
    {
        if( addr_p->TCIPtr != iptr ) break;
    }
    if(i==0)
    {
        len+=sprintf(buf+len,"(Stalled!) ");
    }
    else
    {
        len+=sprintf(buf+len,"(FreeRunning) ");
    }
    len+=sprintf(buf+len,"\n");
    
    *eof = 1;
    return len;
}



/* Display global info */
int stptiHAL_read_procglobal( char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int len = 0;

    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCGlobalInfo_t      *TCGlobalInfo_p;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    
    TCGlobalInfo_p =  (TCGlobalInfo_t*)Device_p->TCPrivateData.TC_Params.TC_GlobalDataStart;
    
    len += sprintf( buf+len,"Global info addr 0x%08x\n",(unsigned int)TCGlobalInfo_p);

    len += sprintf( buf+len,"GlobalPktCount        :0x%08x\n", TCGlobalInfo_p->GlobalPktCount );

    len += sprintf( buf+len,"PacketHeader          :0x%04x HeaderDesignator   :0x%04x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalPacketHeader ),
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalHeaderDesignator ));

    len += sprintf( buf+len,"ModeFlags             :0x%04x LastQWrite         :0x%08x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalModeFlags ),
                    (unsigned int) STSYS_ReadRegDev32LE( (void*)&TCGlobalInfo_p->GlobalLastQWrite ));

    len += sprintf( buf+len,"QPointsInPacket       :0x%04x SlotMode           :0x%04x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalQPointsInPacket ),
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalSlotMode ));

    len += sprintf( buf+len,"DMACntrl_p            :0x%04x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalDMACntrl_p ));

    len += sprintf( buf+len,"SignalModeFlags       :0x%04x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalSignalModeFlags ));

    len += sprintf( buf+len,"Scratch               :0x%08x\n",
                    (unsigned int) STSYS_ReadRegDev32LE( (void*)&TCGlobalInfo_p->GlobalScratch ));

    len += sprintf( buf+len,"CAMArbiterInhibit     :0x%04x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalCAMArbiterInhibit ));

    len += sprintf( buf+len,"GlobalCAMArbiterIdle  :0x%04x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalCAMArbiterIdle ));

    len += sprintf( buf+len,"NegativePidSlotIdent  :0x%04x NegativePidMatchingEnable : 0x%04x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalNegativePidSlotIdent ),
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalNegativePidMatchingEnable ));

    len += sprintf( buf+len,"GlobalSFTimeouts      :0x%04x\n",
                    (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCGlobalInfo_p->GlobalSFTimeouts ));
    
    *eof = 1;
    return len;
}


/* Display filter structures */
int stptiHAL_read_procfilters( char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int        i         = 0;
    int        len       = 0;
    unsigned   maxFilter = 0;
    unsigned   priorFlt  = 0;
    static int cur_pos   = 0;

    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCPrivateData_t     *TCPrivateData_p;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    
    TCPrivateData_p = (TCPrivateData_t*)&Device_p->TCPrivateData;
    
    if( offset == 0 ){
        cur_pos = 0;
    }
    
    /*==== Transport Filters ===============================================*/

    maxFilter = TCPrivateData_p->TC_Params.TC_NumberTransportFilters ;

    if( cur_pos == priorFlt ){
        
        /* First page */
        len += sprintf( buf+len,"Number of Transport Filters = %d\n", maxFilter);

        len += sprintf( buf+len,"Addr Filt Mask\n");
    }

    /* Be carefull not to output more than count. */
    for(i = cur_pos -priorFlt; ((i < maxFilter) && (len < count-80)); i++, cur_pos++ ){

        TCTransportFilter_t *Filter_p =  &((TCTransportFilter_t *)TCPrivateData_p->TC_Params.TC_TransportFilterStart)[cur_pos];
                       /*        ----.----.----*/
        len += sprintf( buf+len,"%04x %04x %04x\n",
                        (U32)Filter_p & 0XFFFF,
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->DataMode ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->MaskWord ) );
    }
    priorFlt += maxFilter;
    
    /*==== PES Filters ======================================================*/

    maxFilter = TCPrivateData_p->TC_Params.TC_NumberPesFilters ;

    if( cur_pos == priorFlt ){
        
        /* First page */
        len += sprintf( buf+len,"\n\nNumber of PES Filters = %d\n", maxFilter);

        len += sprintf( buf+len,"Addr Filt Mask Data DTS(Min.) DTS(Max.) PTS(Min.) PTS(Max.)\n");
    }

    /* Be carefull not to output more than count. */
    for(i = cur_pos -priorFlt; ((i < maxFilter) && (len < count-80)); i++, cur_pos++ ){
        
        TCPESFilter_t *Filter_p = &((TCPESFilter_t *)TCPrivateData_p->TC_Params.TC_PESFilterStart)[cur_pos];
        U16 Flags               = STSYS_ReadRegDev16LE((void*)&Filter_p->Bit32);

                       /*        ----.----.----.----.---------.---------.---------.---------*/
        len += sprintf( buf+len,"%04x %04x %04x %04x %01x%08x %01x%08x %01x%08x %01x%08x\n",
                        (U32)Filter_p & 0XFFFF,
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->FlagsData ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->FlagsMask ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->PESData   ),
                        ((Flags >> 0)&1), (unsigned int) STSYS_ReadRegDev32LE((void*)&Filter_p->DTSMinLSW),
                        ((Flags >> 1)&1), (unsigned int) STSYS_ReadRegDev32LE((void*)&Filter_p->DTSMaxLSW),
                        ((Flags >> 2)&1), (unsigned int) STSYS_ReadRegDev32LE((void*)&Filter_p->PTSMinLSW),
                        ((Flags >> 3)&1), (unsigned int) STSYS_ReadRegDev32LE((void*)&Filter_p->PTSMaxLSW) );
    }
    priorFlt += maxFilter;
    
    /*==== Section Filters ================================================*/

    maxFilter = TCPrivateData_p->TC_Params.TC_NumberSectionFilters ;

#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
    if( cur_pos == priorFlt ){
        
        /* First page */
        len += sprintf( buf+len,"\n\nNumber of Section Filters = %d\n", maxFilter);

        len += sprintf( buf+len,"Addr Association..................... Hdr0 Hdr1 Hdr2 Hdr3 Hdr4 Hdr5 Hdr6 Hdr7 Hdr8 HCnt CRC..... SCnt State\n");
    }

    /* Be carefull not to output more than count. */
    for(i = cur_pos -priorFlt; ((i < maxFilter) && (len < count-120)); i++, cur_pos++ ){

        TCSectionFilterInfo_t *Filter_p =  &((TCSectionFilterInfo_t *)TCPrivateData_p->TC_Params.TC_SFStatusStart)[cur_pos];
                       /*        ----.----------------.----.----.----.----.----.----.--------.----.----*/
        len += sprintf( buf+len,"%04x %04x%04x%04x%04x%04x%04x%04x%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x%04x %04x %04x\n",
                        (U32)Filter_p & 0XFFFF,
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation7 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation6 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation5 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation4 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation3 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation2 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation1 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation0 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader0 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader1 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader2 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader3 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader4 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader5 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader6 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader7 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader8 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeaderCount ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionCrc1 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionCrc0 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionCount ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionState ) );
    }
    priorFlt += maxFilter;
#else       /* not PTI4SL */
    if( cur_pos == priorFlt ){
        
        /* First page */
        len += sprintf( buf+len,"\n\nNumber of Section Filters = %d\n", maxFilter);

        len += sprintf( buf+len,"Addr Association..... Hdr0 Hdr1 Hdr2 Hdr3 Hdr4 HCnt CRC..... SCnt State\n");
    }

    /* Be carefull not to output more than count. */
    for(i = cur_pos -priorFlt; ((i < maxFilter) && (len < count-120)); i++, cur_pos++ ){

        TCSectionFilterInfo_t *Filter_p =  &((TCSectionFilterInfo_t *)TCPrivateData_p->TC_Params.TC_SFStatusStart)[cur_pos];
                       /*        ----.----------------.----.----.----.----.----.----.--------.----.----*/
        len += sprintf( buf+len,"%04x %04x%04x%04x%04x %04x %04x %04x %04x %04x %04x %04x%04x %04x %04x\n",
                        (U32)Filter_p & 0XFFFF,
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation3 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation2 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation1 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionAssociation0 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader0 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader1 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader2 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader3 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeader4 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionHeaderCount ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionCrc1 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionCrc0 ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionCount ),
                        (unsigned int) STSYS_ReadRegDev16LE((void*)&Filter_p->SectionState ) );
    }
    priorFlt += maxFilter;
#endif
    
    /*==== End Game =======================================================*/

    *start = buf;
    if( cur_pos == priorFlt)
    {
        *eof = 1;
    }
    else
    {
        *eof = 0;
    }
    
    /* Some kernels seem to ignore eof unless start is NULL ? */
    if( (*eof == 1) && len==0)
    {
        start = NULL;
    }

    return len;
}


/* Display main info */
int stptiHAL_read_procmaininfo( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    int len = 0;
    unsigned numslots;
    static int cur_pos = 0;

    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCPrivateData_t     *TCPrivateData_p;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    
    TCPrivateData_p = (TCPrivateData_t*)&Device_p->TCPrivateData;
    
    numslots = TCPrivateData_p->TC_Params.TC_NumberSlots ;

    if( offset == 0 ){
        cur_pos = 0;
        /* First page */
        len += sprintf( buf+len,"Number of Slots = %d\n",TCPrivateData_p->TC_Params.TC_NumberSlots);

        len += sprintf( buf+len,"     Slot PckC Mode KEYp DMAs IdxM SFp  RPL  PES  RECp\n");
    }

    /* Be carefull no to output more than count. */
    for(;((cur_pos<numslots) && (len < count-120)); cur_pos++ ){

        TCMainInfo_t *TCMainInfo_p =  &((TCMainInfo_t *)TCPrivateData_p->TC_Params.TC_MainInfoStart)[cur_pos];
                       /*        ----.----.----.------.------.-----.------.-------.------.------- -------*/
        len += sprintf( buf+len,"%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
                        cur_pos,
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->SlotState ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->PacketCount ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->SlotMode ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->DescramblerKeys_p ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->DMACtrl_indices ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->IndexMask ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->SectionPesFilter_p ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->RemainingPESLength ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->PESStuff ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCMainInfo_p->StartCodeIndexing_p ));
    }

    *start = buf;
    if( cur_pos == numslots )
    {
        *eof = 1;
    }
    else
    {
        *eof = 0;
    }

    /* Some kernels seem to ignore eof unless start is NULL ? */
    if( (*eof == 1) && len==0)
    {
        start = NULL;
    }
    
    return len;
}

/* Display session info */
int stptiHAL_read_proc_sess( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    int len = 0;
    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCPrivateData_t     *TCPrivateData_p;
    TCSessionInfo_t     *TCSessionInfo_p;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    
    TCPrivateData_p = (TCPrivateData_t*)&Device_p->TCPrivateData;
    
    TCSessionInfo_p = &((TCSessionInfo_t *)TCPrivateData_p->TC_Params.TC_SessionDataStart)[Device_p->Session];

    len += sprintf( buf+len,"Session %d data from address 0x%08x\n",Device_p->Session,(U32)TCSessionInfo_p);

    len += sprintf( buf+len,"InputPacketCount   : 0x%04x     InputErrorCount            : 0x%04x     CAMFilterStartAddr         : 0x%04x\n",
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionInputPacketCount ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionInputErrorCount ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionCAMFilterStartAddr ));

    len += sprintf( buf+len,"CAMConfig          : 0x%04x     PIDFilterStartAddr         : 0x%04x     PIDFilterLength            : 0x%04x\n",
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionCAMConfig ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionPIDFilterStartAddr ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionPIDFilterLength ));

    len += sprintf( buf+len,"SectionParams      : 0x%04x     TSmergerTag                : 0x%04x     ProcessState               : 0x%04x\n",
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionSectionParams ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionTSmergerTag ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionProcessState ));

    len += sprintf( buf+len,"ModeFlags          : 0x%04x     NegativePidSlotIdent       : 0x%04x     NegativePidMatchingEnable  : 0x%04x\n",
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionModeFlags ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionNegativePidSlotIdent ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionNegativePidMatchingEnable ));

    len += sprintf( buf+len,"UnmatchedSlotMode  : 0x%04x     UnmatchedDMACntrl_p        : 0x%04x     InterruptMask0             : 0x%04x\n",                        
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionUnmatchedSlotMode ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionUnmatchedDMACntrl_p ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionInterruptMask0 ));

    len += sprintf( buf+len,"InterruptMask1     : 0x%04x     SessionSTCWord0            : 0x%04x     SessionSTCWord1            : 0x%04x\n",
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionInterruptMask1 ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionSTCWord0 ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionSTCWord1 ));

    len += sprintf( buf+len,"SessionSTCWord2    : 0x%04x     DiscardParams              : 0x%04x\n\n",
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionSTCWord2 ),
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionDiscardParams ));

    len += sprintf( buf+len,"SectionEnables_0_31: 0x%08x     SectionEnables_32_63       : 0x%08x\n",
                        (unsigned int) STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_0_31 ),
                        (unsigned int) STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SectionEnables_32_63 ));

    len += sprintf( buf+len,"PIPFilterBytes     : 0x%08x     PIPFilterMask              : 0x%08x\n",
                        (unsigned int) STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SessionPIPFilterBytes ),
                        (unsigned int) STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SessionPIPFilterMask));

    len += sprintf( buf+len,"CAPFilterBytes     : 0x%08x     CAPFilterMask              : 0x%08x\n",
                        (unsigned int) STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SessionCAPFilterBytes ),
                        (unsigned int) STSYS_ReadRegDev32LE( (void*)&TCSessionInfo_p->SessionCAPFilterMask));
    
    *eof = 1;
    return len;
}

int stptiHAL_read_proc_globaltcdmacfg( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    int dmas,len = 0;
    static int cur_pos = 0;
    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCPrivateData_t     *TCPrivateData_p;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    
    TCPrivateData_p = (TCPrivateData_t*)&Device_p->TCPrivateData;    

    dmas = TCPrivateData_p->TC_Params.TC_NumberDMAs;

    if( offset == 0 ){
        cur_pos = 0;
        /* First page */
        len += sprintf( buf+len,"Number of DMAs = %d\n",dmas);
    }

    /* Be carefull no to output more than count. */
    for(;((cur_pos<dmas) && (len < count-120)); cur_pos++ ){
        TCDMAConfig_t *DMAConfig_p = TcHal_GetTCDMAConfig( (STPTI_TCParameters_t *)&TCPrivateData_p->TC_Params, cur_pos );

        /* Base_p :0x01234567 Top_p :0x01234567 Write_p :0x01234567 Read_p :0x01234567        */
        /* QWrite_p :0x01234567 BffrPcktCnt :0x01234567 SigModeFlgs :0x0123 Threshold :0x0123 LevelThreshold :0x1234*/

        len += sprintf( buf+len,"%3d:  ",cur_pos);
        len += sprintf( buf+len,"Base:0x%08x  Top:0x%08x  Write:0x%08x  Read:0x%08x  QWrite:0x%08x\n",
                        DMAConfig_p->DMABase_p,  DMAConfig_p->DMATop_p,
                        DMAConfig_p->DMAWrite_p,DMAConfig_p->DMARead_p,
                        DMAConfig_p->DMAQWrite_p);
        len += sprintf( buf+len,"      BffrPcktCnt:0x%08x  SigModeFlgs:0x%04x  Threshold:0x%04x  ",
                        DMAConfig_p->BufferPacketCount,
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&DMAConfig_p->SignalModeFlags) ,
                        (unsigned int) STSYS_ReadRegDev16LE( (void*)&DMAConfig_p->Threshold ));
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        len += sprintf( buf+len,"LevelThreshold:0x%04x", DMAConfig_p->BufferLevelThreshold );
#endif
        len += sprintf( buf+len,"\n" );
        
    }

    *start = buf;
    if( cur_pos == dmas ){
        *eof = 1;
    }
    else{
        *eof = 0;
    }

    /* Some kernels seem to ignore eof unless start is NULL ? */
    if( (*eof == 1) && len==0)
    {
        start = NULL;
    }
    return len;
}

int stptiHAL_read_proc_lut( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    int i,slots,len = 0;
    U16 *lut;
    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCPrivateData_t     *TCPrivateData_p;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    
    TCPrivateData_p = (TCPrivateData_t*)&Device_p->TCPrivateData;        

    lut = (U16 *)TCPrivateData_p->TC_Params.TC_LookupTableStart;
    len += sprintf( buf+len,"Slot Look up table for PID%d. Slot LUT addr: 0x%08x\n",(int)data,(int)lut);

    slots = TCPrivateData_p->TC_Params.TC_NumberSlots;

    len += sprintf( buf+len,"Number of Slots = %d\n",slots);

    len += sprintf( buf+len,"offset   +0     +2     +4     +6     +8     +A     +C     +E");
    for(i = 0;i < slots; i++){
        if( i % 8 == 0 )
            len += sprintf( buf+len,"\n0x%02x",i*2);
        len += sprintf( buf+len," 0x%04x",*(lut+i));
    }

    len += sprintf( buf+len,"\n");

    *eof = 1;
    return len;
}


/* Display tcparams info */
int stptiHAL_read_proc_tcparams( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    int len = 0;
    FullHandle_t FullHandle;
    Device_t            *Device_p;
    TCPrivateData_t     *TCPrivateData_p;
    U8                  *tc_version_string;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );
    
    TCPrivateData_p = (TCPrivateData_t*)&Device_p->TCPrivateData;       

    tc_version_string = (U8*) TCPrivateData_p->TC_Params.TC_VersionID;
    len += sprintf( buf+len,"PTI_VERSION: %s\n", STPTI_REVISION );
    len += sprintf( buf+len," TC_VERSION: %c%c%c%c%c%c ", tc_version_string[1], tc_version_string[0], tc_version_string[3], tc_version_string[2], tc_version_string[5], tc_version_string[4] );
    len += sprintf( buf+len,"%x ", (signed) (tc_version_string[9]<<8 | tc_version_string[8]) );
    len += sprintf( buf+len,"(TC_ID is %d)\n", (signed) (tc_version_string[7]<<8 | tc_version_string[6]) );
    
    len += sprintf( buf+len,"TC_CodeStart                  : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_CodeSize);
    len += sprintf( buf+len,"TC_CodeSize                   : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_CodeSize);
    len += sprintf( buf+len,"TC_DataStart                  : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_DataStart);
    len += sprintf( buf+len,"TC_LookupTableStart           : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_LookupTableStart);
    len += sprintf( buf+len,"TC_GlobalDataStart            : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_GlobalDataStart);
    len += sprintf( buf+len,"TC_StatusBlockStart           : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_StatusBlockStart);
    len += sprintf( buf+len,"TC_MainInfoStart              : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_MainInfoStart);
    len += sprintf( buf+len,"TC_DMAConfigStart             : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_DMAConfigStart);
    len += sprintf( buf+len,"TC_DescramblerKeysStart       : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_DescramblerKeysStart);
    len += sprintf( buf+len,"TC_TransportFilterStart       : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_TransportFilterStart);
    len += sprintf( buf+len,"TC_PESFilterStart             : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_PESFilterStart);
    len += sprintf( buf+len,"TC_SubstituteDataStart        : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_SubstituteDataStart);
    len += sprintf( buf+len,"TC_SFStatusStart              : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_SFStatusStart);
    len += sprintf( buf+len,"TC_InterruptDMAConfigStart    : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_InterruptDMAConfigStart);
    len += sprintf( buf+len,"TC_EMMStart                   : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_EMMStart);
    len += sprintf( buf+len,"TC_ECMStart                   : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_ECMStart);
    len += sprintf( buf+len,"TC_MatchActionTable           : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_MatchActionTable);
    len += sprintf( buf+len,"TC_SessionDataStart           : 0x%08x\n",(unsigned int)TCPrivateData_p->TC_Params.TC_SessionDataStart);
    len += sprintf( buf+len,"TC_NumberCarousels            : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberCarousels);
    len += sprintf( buf+len,"TC_NumberDMAs                 : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberDMAs);
    len += sprintf( buf+len,"TC_NumberDescramblerKeys      : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberDescramblerKeys);
    len += sprintf( buf+len,"TC_NumberIndexs               : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberIndexs);
    len += sprintf( buf+len,"TC_NumberPesFilters           : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberPesFilters);
    len += sprintf( buf+len,"TC_NumberSectionFilters       : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberSectionFilters);
    len += sprintf( buf+len,"TC_NumberSlots                : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberSlots);
    len += sprintf( buf+len,"TC_NumberTransportFilters     : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberTransportFilters);
    len += sprintf( buf+len,"TC_NumberEMMFilters           : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberEMMFilters);
    len += sprintf( buf+len,"TC_NumberECMFilters           : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberECMFilters);
    len += sprintf( buf+len,"TC_NumberOfSessions           : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberOfSessions);
    len += sprintf( buf+len,"TC_NumberSCDFilters           : %d\n",(int)TCPrivateData_p->TC_Params.TC_NumberSCDFilters);

    len += sprintf( buf+len,"TC_AutomaticSectionFiltering  : %s\n",TCPrivateData_p->TC_Params.TC_AutomaticSectionFiltering?"TRUE":"FALSE");
    len += sprintf( buf+len,"TC_MatchActionSupport         : %s\n",TCPrivateData_p->TC_Params.TC_MatchActionSupport?"TRUE":"FALSE");
    len += sprintf( buf+len,"TC_SignalEveryTransportPacket : %s\n",TCPrivateData_p->TC_Params.TC_SignalEveryTransportPacket?"TRUE":"FALSE");

    *eof = 1;
    return len;
}


int stptiHAL_read_proc_objects( char *buf, char **start, off_t offset,
                           int count, int *eof, void *data )
{
    int len = 0;
    static int cur_pos = 0;
    int obj_count = 0;

    FullHandle_t FullHandle;
    Device_t  *Device_p;
    Session_t  *Session_p;
    int session, object;
    ObjectType_t ObjectType;

    FullHandle.word = (U32)data;
    Device_p      = stptiMemGet_Device( FullHandle );

    if( offset == 0 ){
        cur_pos = 0;
        /* First page */
    }

    for(session=0;session<Device_p->MemCtl.MaxHandles;session++)
    {
        FullHandle.word = Device_p->MemCtl.Handle_p[session].Hndl_u.word;
        /* Is this a real session or a previously deallocated one? */
        if(FullHandle.word!=STPTI_NullHandle())
        {
            Session_p = stptiMemGet_Session( FullHandle );
            if( (obj_count++) >= cur_pos )
            {
                len += sprintf( buf+len, "%08x ", FullHandle.word );
                len += sprintf( buf+len, "SESSION " );
                len += sprintf( buf+len,"\n" );
            }
            for(object=0;object<Session_p->MemCtl.MaxHandles;object++)
            {
                /* Be careful not to output more than count. */
                if(len >= count-256)
                {
                    *start = buf;
                    *eof = 0;
                    if(len>0)
                    {
                        cur_pos = obj_count;
                    }
                    return(len);
                }
                if( (obj_count++) >= cur_pos )
                {
                    FullHandle.word = Session_p->MemCtl.Handle_p[object].Hndl_u.word;
                    if(FullHandle.word!=STPTI_NullHandle())
                    {
                        len += sprintf( buf+len, "%08x ", FullHandle.word );
                        
                        ObjectType = (ObjectType_t) FullHandle.member.ObjectType;
                        switch(ObjectType)
                        {
                            case OBJECT_TYPE_BUFFER:
                                {
                                    Buffer_t *object_p = stptiMemGet_Buffer(FullHandle);
                                    len += sprintf( buf+len, "BUFFER ");
                                    len += sprintf( buf+len, "(TCid=%d, MappedStart_p=0x%08x, ", object_p->TC_DMAIdent, (U32) object_p->MappedStart_p );
#ifdef STPTI_CAROUSEL_SUPPORT
                                    len += sprintf( buf+len, "CarouselList=%08x, ", object_p->CarouselListHandle.word );
#endif
                                    len += sprintf( buf+len, "SlotList=%08x, SignalList=%08x) ", object_p->SlotListHandle.word, object_p->SignalListHandle.word );
                                }
                                break;
                            case OBJECT_TYPE_DESCRAMBLER:
                                {
                                    Descrambler_t *object_p = stptiMemGet_Descrambler(FullHandle);
                                    len += sprintf( buf+len, "DESCRAMBLER ");
                                    len += sprintf( buf+len, "(TCid=%d, SlotList=%08x, Pid=0x%x) ", object_p->TC_DescIdent, object_p->SlotListHandle.word, object_p->AssociatedPid );
                                }
                                break;
                            case OBJECT_TYPE_FILTER:
                                {
                                    Filter_t *object_p = stptiMemGet_Filter(FullHandle);
                                    len += sprintf( buf+len, "FILTER ");
                                    len += sprintf( buf+len, "(TCid=%d) ", object_p->TC_FilterIdent );
                                }
                                break;
                            case OBJECT_TYPE_SIGNAL:
                                {
                                    Signal_t *object_p = stptiMemGet_Signal(FullHandle);
                                    len += sprintf( buf+len, "SIGNAL ");
                                    len += sprintf( buf+len, "(AssociatedBufferList=%08x) ", object_p->BufferListHandle.word );
                                }
                                break;
                            case OBJECT_TYPE_SLOT:
                                {
                                    Slot_t *object_p = stptiMemGet_Slot(FullHandle);
                                    len += sprintf( buf+len, "SLOT ");
                                    len += sprintf( buf+len, "(TCid=%d ", object_p->TC_SlotIdent );
                                    if( object_p->Flags.SignalOnEveryTransportPacket ) len += sprintf( buf+len, "SOETP ");
                                    if( object_p->Flags.CollectForAlternateOutputOnly ) len += sprintf( buf+len, "CFAOO ");
                                    if( object_p->Flags.AlternateOutputInjectCarouselPacket ) len += sprintf( buf+len, "AOICP ");
                                    if( object_p->Flags.StoreLastTSHeader ) len += sprintf( buf+len, "SLTSH ");
                                    if( object_p->Flags.InsertSequenceError ) len += sprintf( buf+len, "ISE ");
                                    if( object_p->Flags.OutPesWithoutMetadata ) len += sprintf( buf+len, "PESNOMD ");
                                    if( object_p->Flags.ForcePesLengthToZero ) len += sprintf( buf+len, "FPESLENTZ ");
                                    if( object_p->Flags.AppendSyncBytePrefixToRawData ) len += sprintf( buf+len, "ASBPTRD ");
                                    if( object_p->Flags.SoftwareCDFifo ) len += sprintf( buf+len, "SWCDFIFO ");
                                    len += sprintf( buf+len, "DescList=%08x, ", object_p->DescramblerListHandle.word );
                                    len += sprintf( buf+len, "FiltList=%08x, ", object_p->FilterListHandle.word );
                                    len += sprintf( buf+len, "BufList=%08x, ", object_p->BufferListHandle.word );
#ifndef STPTI_NO_INDEX_SUPPORT
                                    len += sprintf( buf+len, "IndxList=%08x, ", object_p->IndexListHandle.word );
#endif
                                }
                                break;
#ifdef STPTI_CAROUSEL_SUPPORT
                            case OBJECT_TYPE_CAROUSEL:
                                {
                                    Carousel_t *object_p = stptiMemGet_Carousel(FullHandle);
                                    len += sprintf( buf+len, "CAROUSEL ");
                                    len += sprintf( buf+len, "(StartEntry=%08x, CurrentEntry=%08x, EndEntry=%08x, BufferListHandle=%08x) ", object_p->StartEntry.word, object_p->CurrentEntry.word, object_p->EndEntry.word, object_p->BufferListHandle.word );
                                }
                                break;
                            case OBJECT_TYPE_CAROUSEL_ENTRY:
                                /* Need to work out if it is Timed or Simple */
                                len += sprintf( buf+len, "CAROUSEL ENTRY" );
                                break;
#endif
#ifndef STPTI_NO_INDEX_SUPPORT
                                case OBJECT_TYPE_INDEX:
                                {
                                    Index_t *object_p = stptiMemGet_Index(FullHandle);
                                    len += sprintf( buf+len, "INDEX ");
                                    len += sprintf( buf+len, "(IndexMask=%08x, SCD_TCid=%d, SlotList=%08x) ", object_p->IndexMask.word, object_p->TC_SCDFilterIdent, object_p->SlotListHandle.word );
                                }
                                break;
#endif
#if defined (STPTI_FRONTEND_HYBRID)
                            case OBJECT_TYPE_FRONTEND:
                                {
                                    Frontend_t *object_p = stptiMemGet_Frontend(FullHandle);
                                    len += sprintf( buf+len, "FRONTEND ");
                                    len += sprintf( buf+len, "(PIDTable_p=0x%08x, RAMBuffer_p=0x%08x, PIDFilterEnabled=%d, IsAssociatedWithHW=%d) ", (U32) object_p->PIDTableRealStart_p, (U32) object_p->RAMRealStart_p, (signed)object_p->PIDFilterEnabled, (signed)object_p->IsAssociatedWithHW);
                                }
                                break;
#endif
                            case OBJECT_TYPE_LIST:
                                {
                                    List_t *list_p = stptiMemGet_List(FullHandle);
                                    FullHandle_t ItemHandle;
                                    BOOL CommaFlag = FALSE;
                                    int item, i;
                                    
                                    /* Highlight any list which are for managing sessions */
                                    for(i=OBJECT_TYPE_BUFFER;i<=OBJECT_TYPE_LIST;i++)
                                    {
                                        if(Session_p->AllocatedList[i].word == FullHandle.word)
                                        {
                                            len += sprintf( buf+len, "LIST (session management obj %d, %d items) ", i, list_p->MaxHandles );
                                            break;
                                        }

                                    }
                                    if(i>OBJECT_TYPE_LIST)
                                    {
                                        len += sprintf( buf+len, "LIST (%d items) ", list_p->MaxHandles );
                                    }
                                    
                                    for(item=0;item<list_p->MaxHandles;item++)
                                    {
                                        ItemHandle.word = ( &stptiMemGet_List( FullHandle )->Handle )[item]; 
                                        if ( ItemHandle.word != STPTI_NullHandle())
                                        {
                                            if(CommaFlag)
                                            {
                                                len += sprintf( buf+len, ", " );
                                            }
                                            else
                                            {
                                                CommaFlag=TRUE;
                                            }
                                            len += sprintf(buf+len, "%08x", ItemHandle.word);
                                        }
                                        
                                        /* need to avoid lines getting too long to make sure we don't overflow our output char buffer */
                                        if(item>8) break;
                                    }
                                    if(item<list_p->MaxHandles) sprintf(buf+len, "...");
                                }
                                break;

                            default:
                                len += sprintf( buf+len,"??" );
                        }
                        len += sprintf( buf+len,"\n" );
                    }
                }
            }
        }
    }

    *start = buf;
    *eof = 1;
    
    /* Some kernels seem to ignore eof unless start is NULL ? */
    if(len==0)
    {
        start = NULL;
    }
    else
    {
        cur_pos = obj_count;
    }

    return len;
}




/*=============================================================================

   stptiHAL_read_procmisc

   Print out miscellaneous statistics and inrormation.

   It is intended for this proc file to used for the general purpose collection
   of statistics and any additional info that is not best in another file or one   of its own.

  ===========================================================================*/
int stptiHAL_read_procmisc( char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int len = 0;

    len += sprintf( buf+len,"Miscellaneous statistics and info\n");
    len += sprintf( buf+len,"=================================\n");  
    
    *eof = 1;      
    
    return len;    
}


/* EOF */
