#ifndef __EVT_TEST_H
#define __EVT_TEST_H

#if !defined( ST_OSLINUX )
#include "stlite.h"
#endif

#include "stos.h"
#include "stdevice.h"
#include "stddefs.h"

#include "app_data.h"
#include "stevt.h"

#if !defined( ST_OSLINUX )
#include "stboot.h"
#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif
#endif

#if defined(ST_OS21) || defined(ST_OSWINCE)
extern partition_t          *system_partition;
#else
extern partition_t          *SystemPartition;
#endif

#if !defined(STEVT_NO_TBX)
#define STEVT_Report(x)          STTBX_Report(x);
#else
#define STEVT_Report(x)                {};
#endif

#if !defined(STEVT_NO_TBX)
#define STEVT_Print(x)          STTBX_Print(x);
#else
#define STEVT_Print(x)          printf x;
#endif

/* TComp_1_1.c */
extern U32 TComp_1_1( void );

/* TComp_1_2.c */
extern U32 TComp_1_2( void );

/* TComp_1_3.c */
extern U32 TComp_1_3( void );

/* TComp_2_1.c */
extern U32 TComp_2_1( void );

/* TComp_2_2.c */
extern U32 TComp_2_2( void );

/* TComp_2_3.c */
extern U32 TComp_2_3( void );

/* TComp_3_1.c */
extern U32 TComp_3_1( void );

/* TComp_3_2.c */
extern U32 TComp_3_2( void );

/* TComp_3_3.c */
extern U32 TComp_3_3( void );

/* TComp_4_1.c */
extern U32 TComp_4_1( void );

/* TComp_4_2.c */
extern U32 TComp_4_2( void );

/* TComp_5_1.c */
extern U32 TComp_5_1( void );

/* TComp_5_2.c */
extern U32 TComp_5_2( void );

/* TComp_6_1.c */
extern U32 TComp_6_1( void );

/* TComp_7_1.c */
extern U32 TComp_7_1( void );

/* TComp_8_1.c */
extern U32 TComp_8_1( void );

/* MITests.c */
extern U32 TESTEVT_MultiInstance (void );

/* Multi name test */
extern U32 TESTEVT_MultiName ( void );

/* Multithreading test */
extern U32 TESTEVT_Threading ( void );

/* Re-enter test */
extern U32 TESTEVT_ReEnter ( void );

#endif /* __EVT_TEST_H */

