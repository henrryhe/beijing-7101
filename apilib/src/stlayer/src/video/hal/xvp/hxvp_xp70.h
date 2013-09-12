/*******************************************************************************

File name   : hxvp_xp70.h

Description : XP70 hearder file
              
COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Feb 2007        Created                                           JPB
                   
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HXVP_XP70_H
#define __HXVP_XP70_H

/* Includes ----------------------------------------------------------------- */
#include "hxvp_main.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/****************************************** MAILBOX registers */
#define	MBOX_IRQ_TO_XP70_OFFSET                 0x00
#define	MBOX_INFO_HOST_OFFSET                   0x04
#define	MBOX_IRQ_TO_HOST_OFFSET                 0x08
#define	MBOX_INFO_xP70_OFFSET                   0x0C
#define	MBOX_SW_RESET_CTRL_OFFSET               0x10
#define	MBOX_STARTUP_CTRL_1_OFFSET              0x14
#define	MBOX_STARTUP_CTRL_2_OFFSET              0x18
#define	MBOX_AUTHORIZE_QUEUES_OFFSET            0x1C
#define	MBOX_GP_STATUS_OFFSET                   0x20
#define	MBOX_NEXT_CMD_OFFSET                    0x24
#define	MBOX_CURRENT_CMD_OFFSET                 0x28


typedef void * XP70Handle_t;

typedef struct XVP_XP70Init_s
{
    ST_Partition_t      *CPUPartition_p;
    void                *IMEMBaseAddress;
    U32                 IMEMSize;
    void                *DMEMBaseAddress;
    U32                 DMEMSize;
    void                *MailboxBaseAddress;
    U32                 MailboxSize;    
#ifdef XVP_READ_FW_FROM_FILE    
    char                *FirmwareFileName;
#else
    const unsigned int  *Firmware_p;
    U32                 FirmwareSize;
#endif        
} XVP_XP70Init_t;

typedef struct XP70_CommonData_s
{
    ST_Partition_t      *CPUPartition_p;
    void                *MailboxBaseAddress;
    void                *IMEMBaseAddress;
    U32                 IMEMSize;    
} XP70_CommonData_t;

/* prototypes ----------------------------------------------------------------*/
ST_ErrorCode_t hxvp_XP70Init(XVP_XP70Init_t *InitParam_p, XP70Handle_t *XP70Handle_p);
ST_ErrorCode_t hxvp_XP70Term(XP70Handle_t XP70Handle);
void hxvp_XP70Start(XP70Handle_t XP70Handle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __XVP_XP70_H */

