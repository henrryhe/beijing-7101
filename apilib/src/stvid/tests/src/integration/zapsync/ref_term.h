/*******************************************************************************

File name   : ref_sync.h

Description : video sync reference file
              Linux file ONLY

COPYRIGHT (C) STMicroelectronics 2006

Note : 

Date               Modification                                     Name
----               ------------                                     ----
23 Jan  2005        Created                                          LM

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
   LMI_VID used                                                                              
   
*/

/* Includes ----------------------------------------------------------------- */

/* Compilation switch ------------------------------------------------------- */

/* Function prototypes ---------------------------------------------- */

void CLKRV_Term(void);
void PTI_Term(void);
void MERGE_Term(void);
void EVT_Term(void);
void AUD_Term(void);
void DENC_Term(void);
void VTG_Term(void);
void VMIX_Term(void);
void LAYER_Term(void);
void VID_Term(void);
void VOUT_Term(void);
void VIDTEST_Term(void);

/* End of ref_sync.h */
