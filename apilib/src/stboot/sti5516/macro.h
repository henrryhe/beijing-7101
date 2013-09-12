/*******************************************************************************
File name : macro.h

Description : Back end macros

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                 				Name
----               ------------                 				----
Jan 2000           Created to manage different chips			FP
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BACKEND_MACRO_H
#define __BACKEND_MACRO_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- */
/* Macro definition */
/* ---------------- */
#define REFRESH(RefreshTime, Frequency) \
    { \
        RefreshTime = (U8)(((((U32)Frequency * (U32)2) + 0x800) >> 12) & 0x7F); \
    }

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BACKEND_MACRO_H */

/* End of macro.h */
