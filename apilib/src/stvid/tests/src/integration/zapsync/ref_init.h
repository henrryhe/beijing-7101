/*******************************************************************************

File name   : ref_init.h

Description : video sync reference file
              Linux file ONLY

COPYRIGHT (C) STMicroelectronics 2006

Note : 

Date               Modification                                     Name
----               ------------                                     ----
30 Jan  2005        Created                                          LM

*******************************************************************************/

/* Assumptions :
   Only one PTI device
   Two video slots
   One audio slot
   7100 Rev B Board
   DVD_TRANSPORT_STPTI4
   DVB
   STVOUT_OUTPUT_ANALOG_CVBS
   Single decode (MPEG or H264), Single display (MAIN) CVBS
   PAL or NTSC format
   VID1 ONLY
   LMI_VID used                                                                              */

/* Includes ----------------------------------------------------------------- */

/* Compilation switch ------------------------------------------------------- */

/* Function prototypes ---------------------------------------------- */

BOOL CLKRV_Init(void);
BOOL EVT_Init(void);
BOOL MERGE_Init(void);
BOOL PTI_Init(void);
BOOL AUD_Init(void);
BOOL DENC_Init(void);
BOOL VTG_Init(void);
BOOL VMIX_Init(void);
BOOL LAYER_Init(void);
BOOL VID_Init(void);
BOOL VOUT_Init(void);
BOOL VIDTEST_LocalInit(void);

/* End of ref_sync.h */
