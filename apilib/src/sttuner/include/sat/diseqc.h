/*Copyright (C) 2001-2002 STMicroelectronics

History:

   04/02/00        original implementation by NDG.

    date: 22-July-2002
version: 
author: NDG ,Noida
comment: 
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_DISEQC_H
#define __STTUNER_DISEQC_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/******************************************************************
 *						Framing Byte
 *
 */
 
 /*From Master */

#define   FB_COMMAND_NO_REPLY					0xE0 /* 1110 0000 No reply required, First transmission*/
#define   FB_COMMAND_NO_REPLY_REPEATED			0xE1 /* No reply required, Repeated transmission*/
#define   FB_COMMAND_REPLY						0xE2 /* Reply required, First transmission*/
#define	  FB_COMMAND_REPLY_REPEATED				0xE3 /* Reply required, Repeated transmission*/

/*Reply from Slave */ 
#define   FB_REPLY_OK							0xE4 /* OK, no errors detected*/
#define   FB_REPLY_COMMAND_NOT_SUPPORTED		0xE5 /* Command not supported by slave*/
#define   FB_REPLY_PARITY_ERROR_REPEAT			0xE6 /* Parity Error detected - Request repeat*/
#define	  FB_REPLY_COMMAND_NOT_RECOGNISED_REPEAT	0xE7 /*Command not recognised - Request repeat*/




/*******************************************************************
 *						Address Byte
 *
 */
#define   AB_ANY_DEVICE					0x00 /* Any Device (Master to all devices)*/
#define   AB_ANY_LNB					0x10 /* Any LNB, Switcher or SMATV (Master to all...)*/
#define   AB_LNB						0x11 /* LNB*/
#define	  AB_LNB_LOOP_THROUOGH			0x12 /* LNB with Loop-through switching*/

#define   AB_SWITCHER_DC_BLOCKING		0x14 /* Switcher (d.c. blocking)*/
#define	  AB_SWITCHER_DC_LOOP_THROUOGH	0x15 /* Switcher with d.c. Loop-through*/
#define	  AB_SMATV						0x18
#define	  AB_ANY_POLARISER				0x20 /* Master to all Polarisers */

#define	  AB_LINEAR_POLARISER			0x21 /* Linear Polarisation (Skew) Controller */
#define	  AB_ANY_POSITIONER				0x30 /* Master to all Positioners */
#define	  AB_PLAR_AZIMUTH_POSITIONER	0x31 /* Polar / Azimuth Positioner */
#define	  AB_ELEVATION_POSITIONER		0x32 /* Elevation Positioner */

#define	  AB_ANY_INSTATLLER_AID			0x40 /* Master to all Installer Aids*/
#define	  AB_SIGNAL_STRENGTH_ANALOGUE_VALUE		0x41 

/*6x  Family reserved for address re-allocations */

#define	  AB_INTELLIGENT_SLAVE_INTERFACES		0x70 /* Any Intelligent Slave Interfaces (Master to all)*/
#define	  AB_INTERFACE_FOR_SUBCRIBER_CONTROLLED_HEADENDS		0x71 

/*Fx  Reserved for OEM Extensions*/


/******************************************************************
 *						Command Byte
 *
 */

#define   CB_RESET				0x00	/* Reset DiSEqCmicrocontroller */
#define   CB_CLR_RESET			0x01	/* Clear the Reset flag */
#define   CB_STANDBY			0x02	/* Switch peripheral power supply OFF */
#define   CB_POWER_ON			0x03	/* Switch peripheral power supply on */
#define	  CB_SET_CONTEND		0x04	/* Set Contention flag*/

#define   CB_CONTEND			0x05	/* Return address only if Contention flag is set*/
#define   CB_CLR_CONTEND		0x06	/* Clear Contention flag */
#define	  ADDRESS				0x07	/* Return address unless Contention flag is set */
#define   MOVE_C				0x08	/* Change address only if Contention flag is set */
#define	  MOVE					0x09 	/* Change address unless Contention flag is set */

#define   CSTATUS				0x10	/* register flags 3 Status*/
#define   CONFIG				0x11	/* Read Configuration flags (peripheral hardware) */
#define   SWITCH_0				0x14	/* Read Switching state flags (Committed port) */
#define   SWITCH_1				0x15	/* Read Switching state flags (Uncommitted port) */
#define	  SWITCH_2				0x16	/* expansion option */
#define   SWITCH_3				0x17	/* expansion option */ 


#define SET_LO					0x20	/* Select the Low Local Oscillator frequency */ 
#define	SET_VR					0x21	/* Select Vertical Polarisation (or Right circular)*/ 
#define SET_POS_A				0x22	/* Select Satellite position A (or position C) */ 
#define SETS0A					0x23	/* Select Switch Option A (e.g. positions A/B) */ 
#define SET_HI					0x24	/* Select the High Local Oscillator frequency */ 

#define SET_HL					0x25	/* Select Horizontal Polarisation (or Left circular)*/ 
#define SET_POS_B				0x26	/* Select Satellite position B (or position D) */ 
#define SET_S0B					0x27	/* Select Switch Option B (e.g. positions C/D) */ 
#define SET_S1A					0x28	/* Select switch S1 input A (deselect input B) */ 
#define SET_S2A					0x29	/* Select switch S2 input A (deselect input B) */ 

#define SET_S3A					0x2A	/* Select switch S3 input A (deselect input B) */ 
#define SET_S4A					0x2B	/* Select switch S4 input A (deselect input B) */ 
#define SET_S1B					0x2C	/* Select switch S1 input B (deselect input A) */ 
#define SET_S2B					0x2D	/* Select switch S2 input B (deselect input A) */ 
#define SET_S3B					0x2E	/* Select switch S3 input B (deselect input A) */ 
#define SET_S4B					0x2F	/* Select switch S4 input B (deselect input A) */ 

#define SLEEP					0x30	/* Ignore all bus commands except */
#define AWAKE					0x31	/* Respond to future bus commands */
#define WRITE_N0				0x38	/* Write to Port group 0 (Committed switches) */
#define WRITE_N1				0x39    /* Write to Port group 1 (Uncommitted switches) */
#define WRITE_N2				0x3A	/* expansion option */
#define WRITE_N3				0x3B	/* expansion option */

#define READ_A0					0x40	/* Read Analogue value A0 */
#define READ_A1					0x41	/* Read Analogue value A1 */
#define WRITE_A0				0x48    /* Write Analogue value A0 (e.g. Skew) */
#define WRITE_A1				0x49    /* Write Analogue value A1 */

#define LO_STRING				0x50	/* Read current frequency [Reply = BCD string] */
#define LO_NOW					0x51	/* Read current frequency table entry number */
#define LO_LO					0x52	/* Read Lo frequency table entry number */
#define LO_HI					0x53	/* Read Hi frequency table entry number */
#define WRITE_FREQ				0x58	/* Write channel frequency (BCD string) */
#define CH_NO					0x59 	/* Write (receivers) selected channel number */

#define HALT					0x60	/* Stop Positioner movement */
#define LIMITS_OFF				0x63	/* Disable Limits */
#define POSSTAT					0x64	/* Read Positioner Status Register */
#define LIMIT_E					0x66	/* Set East Limit (& Enable recommended) */
#define LIMIT_W					0x67	/* Set West Limit (& Enable recommended) */

#define DRIVE_EAST				0x68	/* Drive Motor East (with optional timeout/steps)*/
#define DRIVE_WEST				0x69	/* Drive Motor West (with optional timeout/steps) */
#define STORE_NN				0x6A	/* Store Satellite Position & Enable Limits */
#define GOTO_NN					0x6B	/* Drive Motor to Satellite Position nn */
#define GOTO_ANGXX				0x6E	/* Drive Motor to Angular Position (0) */
#define SET_POSNS				0x6F	/* (Re-) Calculate Satellite Positions */




/*************Added datastructure to get control over DiSEqC Tone on and off*****************************/
typedef enum STTUNER_DiSEqCToneState_e
    {
        STTUNER_DiSEqC_TONE_CONTINUOUS,   /* Diseqc mode is in continuous state */
        STTUNER_DiSEqC_TONE_CONTINUOUS_OFF       /* Diseqc mode is in default state */
        
    } STTUNER_DiSEqCToneState;

typedef struct
{
   STTUNER_DiSEqCCommand_t Command ;
   STTUNER_DiSEqCToneState ToneState  ;

} STTUNER_DiSEqCConfig_t;
/************************************************************/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~ Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

ST_ErrorCode_t STTUNER_SAT_DiSEqCSendReceive (STTUNER_Handle_t Handle, 
									   STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									   STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									   );




/**********************/

#define DiSEqC_Delay_ms(m_sec) STOS_TaskDelayUs(m_sec *  1000)

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_DISEQC_H */

/* End of diseqc.h */
