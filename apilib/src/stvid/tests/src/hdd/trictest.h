/*****************************************************************************

  File name   : trictest.h

  Description : Contains the high level calls of the hdd libray

  COPYRIGHT (C) ST-Microelectronics 2005.

  Date               Modification                 Name
  ----               ------------                 ----
  07/15/99           Created                      FR
  08/01/00           Adapted for STVID (STAPI)    AN
*****************************************************************************/
/* --- Define to prevent recursive inclusion ------------------------------ */
#ifndef __TRICTEST_H
#define __TRICTEST_H

#ifndef STFAE
/* Definitions from STFAE to make trickmod.c module compatible with both video
 * test application and STFAE tree */

#define PLAYREC_DRIVER_ID               (0x7FAE)
#define PLAYREC_DRIVER_BASE             (PLAYREC_DRIVER_ID<<16)

/* PLAYREC objects events */
/* ---------------------- */
typedef enum PLAYREC_Event_s
{
    PLAYREC_EVT_END_OF_PLAYBACK=PLAYREC_DRIVER_BASE, /* Last picture displayed and last audio frame played */
    PLAYREC_EVT_END_OF_FILE,                         /* Reach end of file                                  */
    PLAYREC_EVT_ERROR,                                /* An error has occur, check
                                                       * error parameter
                                                       * */
    PLAYREC_EVT_PLAY_STOPPED,
    PLAYREC_EVT_PLAY_SPEED_CHANGED,
    PLAYREC_EVT_PLAY_STARTED,
    PLAYREC_EVT_PLAY_JUMP
} PLAYREC_Event_t;
#endif /* !STFAE */

ST_ErrorCode_t TRICKMOD_Debug(void);

#endif /* TRICTEST_H */
