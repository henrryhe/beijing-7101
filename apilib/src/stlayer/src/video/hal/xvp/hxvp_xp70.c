/*******************************************************************************

File name   : hxvp_xp70.c

Description : XP70 source file
              
COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Feb 2007        Created                                           JPB
                   
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
/*#define STTBX_REPORT*/
#include "sttbx.h"

#include "stsys.h"
#include "halv_lay.h"

#include "hxvp_xp70.h"

/*******************************************************
 * hxvp_XP70Init                                       *
 *******************************************************/

ST_ErrorCode_t  hxvp_XP70Init(  XVP_XP70Init_t      *InitParam_p,
                                XP70Handle_t        *XP70Handle_p)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    int                             Cpt;
    U32                             *Ptr;
    U32                             XP70ResetDone = 0;
    XP70_CommonData_t               *XP70Properties_p;
#ifdef XVP_READ_FW_FROM_FILE
    FILE                            *Fd;
    size_t                          Transfer;
#endif
    
    /* allocate memory */           
    XP70Properties_p = (void*)STOS_MemoryAllocate(  InitParam_p->CPUPartition_p,
                                                    sizeof(XP70_CommonData_t));        
    if (!XP70Properties_p)
    {
        return ST_ERROR_NO_MEMORY;
    }
    
    /* store data */
    XP70Properties_p->CPUPartition_p        = InitParam_p->CPUPartition_p;   
    XP70Properties_p->MailboxBaseAddress    = InitParam_p->MailboxBaseAddress; 
    XP70Properties_p->IMEMBaseAddress       = InitParam_p->IMEMBaseAddress; 
    XP70Properties_p->IMEMSize              = InitParam_p->IMEMSize;       
    
    *XP70Handle_p = (XP70Handle_t*)XP70Properties_p;
        
    /* Init mailbox */ 
    memset(     (void*)(XP70Properties_p->MailboxBaseAddress), 0,
                InitParam_p->MailboxSize);
   
    /* Init IMEM memory with 0 */    
    memset(     (void*)(InitParam_p->IMEMBaseAddress), 0,
                InitParam_p->IMEMSize);

    /* Init DMEM memory with 0 */   
    memset(     (void*)(InitParam_p->DMEMBaseAddress), 0,
                InitParam_p->DMEMSize);
                
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "mailbox, IMEM and DMEM initialized"));
    
    /* soft reset */
    STSYS_WriteRegDev32LE (XP70Properties_p->MailboxBaseAddress + MBOX_SW_RESET_CTRL_OFFSET, 0xFFFFFFFF);
    STSYS_WriteRegDev32LE (XP70Properties_p->MailboxBaseAddress + MBOX_SW_RESET_CTRL_OFFSET, 0x00000000);

    /* Host checks FlexVPE-2 bit ReadyForDownload */ 
    do 
    { 
        XP70ResetDone = (STSYS_ReadRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_STARTUP_CTRL_1_OFFSET)) & 0x1 ; 
    }
    while ( XP70ResetDone == 0 ) ;
            
    /* configure mailbox registers */
    STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_IRQ_TO_XP70_OFFSET,     0x00); 
    STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_IRQ_TO_HOST_OFFSET,     0x00); 
    STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_INFO_xP70_OFFSET,       0x00); 
    STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_STARTUP_CTRL_1_OFFSET,  0x04 | (1<<10));
    STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_AUTHORIZE_QUEUES_OFFSET,0x00); 
    STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_GP_STATUS_OFFSET,       0x00);
    
#ifdef XVP_READ_FW_FROM_FILE
    /* upload firmware from binary file */
    Fd = fopen (InitParam_p->FirmwareFileName, "rb");
    if ( Fd == NULL )
    {
        printf("unable to open file");
        return ST_ERROR_BAD_PARAMETER;
    }
    Ptr = (U32*)(InitParam_p->IMEMBaseAddress);
    Transfer = fread(Ptr, 1, 0x4000, Fd);
    fclose (Fd);
    if ( Transfer == 0x4000 )
    {
        STTBX_Report((  STTBX_REPORT_LEVEL_INFO,
                        "ERROR !!!! BILBO FGT CODE SIZE TOO "
                        "LARGE Max is 0x4000, actual size is %x\n", Transfer));
    }
#else
    /* upload firmware from array */
    if (InitParam_p->FirmwareSize <= 0x4000)
    {
        Ptr = (U32*)(InitParam_p->IMEMBaseAddress);
        for (Cpt = 0; Cpt < InitParam_p->FirmwareSize; Cpt++)
        {
            *Ptr++ = InitParam_p->Firmware_p[Cpt];
        }
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FGT Firmware uploaded successfully"));                
    }
    else
    {
        STTBX_Report((  STTBX_REPORT_LEVEL_INFO,
                        "ERROR !!!! BILBO FGT CODE SIZE TOO "
                        "LARGE Max is 0x4000, actual size is %x\n", InitParam_p->FirmwareSize));
    }                
#endif           
    
    return ErrorCode;
}

/*******************************************************
 * hxvp_XP70Term                                       *
 *******************************************************/

ST_ErrorCode_t hxvp_XP70Term(XP70Handle_t XP70Handle)
{
    ST_ErrorCode_t		ErrorCode = ST_NO_ERROR;
    XP70_CommonData_t    *XP70Properties_p;

    XP70Properties_p = (XP70_CommonData_t*)XP70Handle;
    if (XP70Properties_p)
    {
        /* Init IMEM memory with 0 */ 
/*           
        memset(     (void*)(XP70Properties_p->IMEMBaseAddress), 0,
                    XP70Properties_p->IMEMSize);
*/    
        /* stop running process and reset BILBO */
/*        
        STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_SW_RESET_CTRL_OFFSET, 0xFFFFFFFF);
        STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_SW_RESET_CTRL_OFFSET, 0x00000000);
*/        
        /* deallocate memory */
        STOS_MemoryDeallocate(  XP70Properties_p->CPUPartition_p,
                                XP70Properties_p);
    }
    return ErrorCode;
}

/*******************************************************
 * hxvp_XP70Start                                      *
 *******************************************************/

void hxvp_XP70Start(XP70Handle_t XP70Handle)
{
    ST_ErrorCode_t		ErrorCode = ST_NO_ERROR;
    XP70_CommonData_t    *XP70Properties_p;

    XP70Properties_p = (XP70_CommonData_t*)XP70Handle;
    if (XP70Properties_p)
    {
        STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_INFO_HOST_OFFSET, 0x03);
        STSYS_WriteRegDev32LE(XP70Properties_p->MailboxBaseAddress + MBOX_STARTUP_CTRL_2_OFFSET, 0x02);
    }    
}




