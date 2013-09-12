/*******************************************************************************

File name   : ball.h

Description : ball header file

COPYRIGHT (C) STMicroelectronics 2001.

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BALL_H
#define __BALL_H


/* Includes ----------------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif
/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

    BOOL BALL_InitCommand (void);
    BOOL BALL_Start(void);
    BOOL BALL_Stop(void);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BALL_H */

/* End of ball.h */

