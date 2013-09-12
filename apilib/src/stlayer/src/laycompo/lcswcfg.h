/*****************************************************************************

File name   : subswcfg.h

COPYRIGHT (C) STMicroelectronics 2001.


****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SUBSWCFG_H
#define __SUBSWCFG_H


/* Includes --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ----------------------------------------------------- */

#define PROCESS_EVT_VSYNC_TASK_STACK_SIZE     512

#ifndef STLAYER_LAYCOMPO_TASK_PRIORITY
#define STLAYER_LAYCOMPO_TASK_PRIORITY (5)
#endif


/* Exported Types --------------------------------------------------------- */


/* Exported Variables ----------------------------------------------------- */


/* Exported Macros -------------------------------------------------------- */


/* Exported Functions ----------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SUBSWCFG_H */

/* End of subswcfg.h */

