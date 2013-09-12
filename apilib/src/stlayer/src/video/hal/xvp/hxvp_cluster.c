/*******************************************************************************

File name   : hxvp_cluster.c

Description : Cluster source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Feb 2007        Created                                           JPB
                   
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"

#include "hxvp_cluster.h"

/*#   define STTBX_REPORT*/
#include "sttbx.h"

#include "stsys.h"

#if defined(ST_OSLINUX)
#   include "layerapi.h"
#endif


/*******************************************************
 * hxvp_clusterInit                                    *
 *******************************************************/
 
ST_ErrorCode_t hxvp_clusterInit(    XVP_clusterInit_t   *InitParam)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    int                 Cpt;
    unsigned int        *Ptr;
    
    /* init cluster */
    Ptr = (unsigned int *)(InitParam->StreamerBaseAddress);
    for (Cpt = 0; Cpt < 0x400; Cpt++)
    {
        *Ptr++ = 0x00000000;
    }
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "cluster initialized"));
    
    /* Load RD and WR plug with microcode */
    Ptr = (unsigned int*)(InitParam->StbusRDPlugBaseAddress);
    for (Cpt = 0; Cpt < InitParam->MicrocodeRDSize; Cpt++)
    {
        *Ptr++ = InitParam->MicrocodeRD_p[Cpt];
    }
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "microcode of RD Plug uploaded")); 

    Ptr = (unsigned int*)(InitParam->StbusWRPlugBaseAddress);
    for (Cpt = 0; Cpt < InitParam->MicrocodeWRSize; Cpt++)
    {
        *Ptr++ = InitParam->MicrocodeWR_p[Cpt];
    }
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "microcode of WR Plug uploaded")); 
                        
    /* configure STBus RD PLUG registers */    
    STSYS_WriteRegDev32LE (InitParam->StbusRDPlugBaseAddress + STBUS_PLUG_PAGE_SIZE_OFFSET, 0x08);
    STSYS_WriteRegDev32LE (InitParam->StbusRDPlugBaseAddress + STBUS_PLUG_MIN_OPC_OFFSET,   0x03);
    STSYS_WriteRegDev32LE (InitParam->StbusRDPlugBaseAddress + STBUS_PLUG_MAX_OPC_OFFSET,   0x05);
    STSYS_WriteRegDev32LE (InitParam->StbusRDPlugBaseAddress + STBUS_PLUG_MAX_CHK_OFFSET,   0x01);
    STSYS_WriteRegDev32LE (InitParam->StbusRDPlugBaseAddress + STBUS_PLUG_MAX_MSG_OFFSET,   0x01);
    STSYS_WriteRegDev32LE (InitParam->StbusRDPlugBaseAddress + STBUS_PLUG_MIN_SPACE_OFFSET, 0x00);
    STSYS_WriteRegDev32LE (InitParam->StbusRDPlugBaseAddress + STBUS_PLUG_CONTROL_OFFSET,   0x01);
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "registers of RD Plug configured")); 
    
    /* configure STBus WR PLUG registers */
    STSYS_WriteRegDev32LE (InitParam->StbusWRPlugBaseAddress + STBUS_PLUG_PAGE_SIZE_OFFSET, 0x08);
    STSYS_WriteRegDev32LE (InitParam->StbusWRPlugBaseAddress + STBUS_PLUG_MIN_OPC_OFFSET,   0x03);
    STSYS_WriteRegDev32LE (InitParam->StbusWRPlugBaseAddress + STBUS_PLUG_MAX_OPC_OFFSET,   0x05);
    STSYS_WriteRegDev32LE (InitParam->StbusWRPlugBaseAddress + STBUS_PLUG_MAX_CHK_OFFSET,   0x01);
    STSYS_WriteRegDev32LE (InitParam->StbusWRPlugBaseAddress + STBUS_PLUG_MAX_MSG_OFFSET,   0x01);
    STSYS_WriteRegDev32LE (InitParam->StbusWRPlugBaseAddress + STBUS_PLUG_MIN_SPACE_OFFSET, 0x00);
    STSYS_WriteRegDev32LE (InitParam->StbusWRPlugBaseAddress + STBUS_PLUG_CONTROL_OFFSET,   0x01);
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "registers of WR Plug configured"));

    return ErrorCode;    
}
