/*******************************************************************************

File name   : gfxswcfg.h

Description : user configurable parameters for:
              - task priorities,
              - stack sizes
              - message queue sizes

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-06-16         Created                                    Adriano Melis
*******************************************************************************/


/* Define to prevent recursive inclusion */

#ifndef __LAYERSWCFG_H
#define __LAYERSWCFG_H


/* Includes --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ----------------------------------------------------- */

#ifdef ST_OS20
#define STLAYER_TASK_STACK_SIZE               (512)
#endif
#if defined(ST_OS21) || defined(ST_OSLINUX)
#define STLAYER_TASK_STACK_SIZE               (16*1024)/*(512)*/
#endif

#ifndef STLAYER_GAMMAGFX_TASK_PRIORITY
#ifdef ST_OSLINUX
#define STLAYER_GAMMAGFX_TASK_PRIORITY       STLAYER_GFX_THREAD_PRIORITY
#else
#define STLAYER_GAMMAGFX_TASK_PRIORITY       (10)  /* min = 0   - max = 15 */
#endif /* LINUX */
#endif
#define STLAYER_MSG_QUEUE_SIZE                (256)


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LAYERSWCFG_H */

/* End of gfxswcfg.h */






