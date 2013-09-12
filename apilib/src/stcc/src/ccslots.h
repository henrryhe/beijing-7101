/*******************************************************************************

File name   : ccslots.h

Description :STCC slots manager header. 

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
10 Aug 2001        Created                                   Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STCC_SLOT_H
#define __STCC_SLOT_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Functions ------------------------------------------------------- */

void stcc_SlotsRecover(stcc_Device_t * const Device_p);
U32  stcc_GetSlotIndex(stcc_Device_t * const Device_p);
void stcc_FreeSlot(stcc_Device_t * const Device_p);
void stcc_InsertSlot(stcc_Device_t * const Device_p, U32 NewSlot);
void stcc_ReleaseSlot(stcc_Device_t * const Device_p,U32 SlotIndex);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STCC_SLOT_H */

/* End of ccslots.h */

