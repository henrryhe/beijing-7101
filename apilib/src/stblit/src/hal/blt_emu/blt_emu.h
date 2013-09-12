   /*******************************************************************************

   File name   : blt_emu.h

   Description : Gamma emulator header file

   COPYRIGHT (C) STMicroelectronics 2000.

   Date               Modification                                     Name
   ----               ------------                                     ----
   28 Nov 2000        Created                                           TM
   *******************************************************************************/

   /* Define to prevent recursive inclusion */

   #ifndef __BLT_EMU_H
   #define __BLT_EMU_H


   /* Includes ----------------------------------------------------------------- */

   #include "stddefs.h"
   #include "blt_init.h"


   /* C++ support */
   #ifdef __cplusplus
   extern "C" {
   #endif

   /* Exported Constants ------------------------------------------------------- */


   /* Exported Types ----------------------------------------------------------- */


   /* Exported Variables ------------------------------------------------------- */


   /* Exported Macros ---------------------------------------------------------- */


   /* Exported Functions ------------------------------------------------------- */
   void stblit_EmulatorProcess (void* data_p);
   U32 stblit_EmulatorProcessNode(stblit_Device_t* Device_p, U32 CpuNodeAddress, BOOL *EnaBlitCompletedIRQ_p);
   void stblit_EmulatorISRProcess (void* data_p);
   ST_ErrorCode_t stblit_EmulatorInit(stblit_Device_t * Device_p);
   ST_ErrorCode_t stblit_EmulatorTerm(stblit_Device_t * Device_p);

   /* C++ support */
   #ifdef __cplusplus
   }
   #endif

   #endif /* #ifndef __BLT_EMU_H */

   /* End of blt_emu.h */
