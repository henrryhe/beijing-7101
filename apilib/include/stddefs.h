/******************************************************************************

File Name   : stddefs.h

Description : Contains a number of generic type declarations and defines.

Copyright (C) 1999-2005 STMicroelectronics

******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STDDEFS_H
#define __STDDEFS_H


/* Includes ---------------------------------------------------------------- */
#if defined(ST_OS20) || defined(ST_OS21)
#include "stlite.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ---------------------------------------------------------- */

/* Common unsigned types */
#ifndef DEFINED_U8
#define DEFINED_U8
typedef unsigned char  U8;
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
typedef unsigned short U16;
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned int   U32;
#endif

/* Common signed types */
#ifndef DEFINED_S8
#define DEFINED_S8
typedef signed char  S8;
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
typedef signed short S16;
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
typedef signed int   S32;
#endif

/*----------------- 64-bit unsigned type -----------------*/
#ifdef ARCHITECTURE_ST20

#ifndef DEFINED_U64
#define DEFINED_U64
typedef struct U64_s
{
    unsigned int LSW;
    unsigned int MSW;
}
U64;
#endif

/*The following S64 type is not truly a Signed number. You would have to use additional
  logic to use it alongwith the macro I64_IsNegative(A) provided below*/
#ifndef DEFINED_S64
#define DEFINED_S64
typedef U64 S64;
#endif


#ifdef PROCESSOR_C2

/*Value=A+B, where A & B is U64 type*/
#define I64_Add(A,B,Value)              { register U32 am = (A).MSW, bm = (B).MSW, l; \
                                        __optasm { \
                                        ldabc (A).LSW, (B).LSW, 0 ;\
                                        lsum    ;\
                                        st l ;\
                                        ldab am, bm ;\
                                        lsum    ;\
                                        st (Value).MSW ;\
                                        }   \
                                        ((Value).LSW) = l;}


/*Value=A+B, where A is U64 type & B is 32-bit atmost*/
#define I64_AddLit(A,B,Value)           { register U32 am = (A).MSW, b = (B), l; \
                                        __optasm { \
                                        ldabc (A).LSW, b, 0 ;\
                                        lsum    ;\
                                        st l ;\
                                        ld am ;\
                                        bsub    ;\
                                        st (Value).MSW ;\
                                        }   \
                                        ((Value).LSW) = l;}
#endif /*#ifdef PROCESSOR_C2*/


#ifdef PROCESSOR_C1

/*Value=A+B, where A & B are U64 type*/
#define I64_Add(A,B,Value)              ((Value).LSW = (A).LSW + (B).LSW,  \
                                         (Value).MSW = (A).MSW + (B).MSW + \
                                         ((((Value).LSW < (A).LSW) || ((Value).LSW < (B).LSW))?(1):(0)))

/*Value=A+B, where A is U64 type & B is 32-bit atmost*/
#define I64_AddLit(A,B,Value)           ((Value).LSW = (A).LSW + (B),  \
                                         (Value).MSW = (A).MSW + \
                                         ((((Value).LSW < (A).LSW) || ((Value).LSW < (B)))?(1):(0)))
#endif /*#ifdef PROCESSOR_C1*/


/*A==B, A & B are U64 type*/
#define I64_IsEqual(A,B)                (((A).LSW == (B).LSW) && ((A).MSW == (B).MSW))

#define I64_GetValue(Value,Lower,Upper) ((Lower) = (Value).LSW, (Upper) = (Value).MSW)

/*A>=B, A & B are U64 type*/
#define I64_IsGreaterOrEqual(A,B)       ( ((A).MSW >  (B).MSW) || \
                                         (((A).MSW == (B).MSW) && ((A).LSW >= (B).LSW)))

/*A>B, A & B are U64 type*/
#define I64_IsGreaterThan(A,B)          ( ((A).MSW >  (B).MSW) || \
                                         (((A).MSW == (B).MSW) && ((A).LSW > (B).LSW)))

/*A<B, A & B are U64 type*/
#define I64_IsLessThan(A,B)             ( ((A).MSW <  (B).MSW) || \
                                         (((A).MSW == (B).MSW) && ((A).LSW < (B).LSW)))

/*A<=B, A & B are U64 type*/
#define I64_IsLessOrEqual(A,B)          ( ((A).MSW <  (B).MSW) || \
                                         (((A).MSW == (B).MSW) && ((A).LSW <= (B).LSW)))

#define I64_IsNegative(A)               ((A).MSW & 0X80000000)

/*A==0, A is U64 type*/
#define I64_IsZero(A)                   (((A).LSW == 0) && ((A).MSW == 0))

/*A!=B, A & B are U64 type*/
#define I64_AreNotEqual(A,B)            (((A).LSW != (B).LSW) || ((A).MSW != (B).MSW))

#define I64_SetValue(Lower,Upper,Value) ((Value).LSW = (Lower), (Value).MSW = (Upper))

/*Value=A-B, where A & B are U64 type*/
#define I64_Sub(A,B,Value)              ((Value).MSW  = (A).MSW - (B).MSW - (((A).LSW < (B).LSW)?1:0), \
                                         (Value).LSW  = (A).LSW - (B).LSW)

/*Value=A-B, where A is U64 type & B is 32-bit atmost*/
#define I64_SubLit(A,B,Value)           ((Value).MSW  = (A).MSW - (((A).LSW < (B))?1:0), \
                                         (Value).LSW  = (A).LSW - (B))


#ifdef PROCESSOR_C2

/*Value=A/B, where A is U64 type & B is 32-bit atmost*/
#define I64_DivLit(A,B,Value)           { register U32 m, al = (A).LSW, b = (B); \
                                        __optasm { \
                                        ldabc b, (A).MSW, 0 ; ldiv ; st m ;\
                                        ldab  b, al ; ldiv ; st (Value).LSW ;\
                                        }   \
                                        ((Value).MSW) = m;}

/*Value=A%B, where A is U64 type & B is 32-bit atmost*/
#define I64_ModLit(A,B,Value)           { register U32 m, al = (A).LSW, b = (B); \
                                        __optasm { \
                                        ldabc b, (A).MSW, 0 ; ldiv ; st m ;\
                                        ldab  b, al ; ldiv ; pop ; st (Value).LSW ;\
                                        }   \
                                        }

/*Value=A*B, where A & B are U64 type*/
#define I64_Mul(A,B,Value)              { register U32 l, al = (A).LSW, am = (A).MSW, bl = (B).LSW, bm = (B).MSW; \
                                        __optasm { \
                                        ldc 0 ;\
                                        ldabc al, bl, 0 ; lmul ; st l ;\
                                        ldab  am, bl;     lmul ;\
                                        ldab  al, bm ;    lmul ; st (Value).MSW ;\
                                        }   \
                                        ((Value).LSW) = l;}

/*Value=A*B, where A is U64 type & B is 32-bit atmost*/
#define I64_MulLit(A,B,Value)           { register U32 l, am = ((A).MSW), b = (B); \
                                        __optasm { \
                                        ldabc b, ((A).LSW), 0 ; lmul ; st l ;\
                                        ldab  b, am ; lmul ; st ((Value).MSW) ;\
                                        }   \
                                        ((Value).LSW) = l;}


/*Value=Value<<Shift, where Value is U64 type*/
#define I64_ShiftLeft(Shift,Value)      if (((unsigned)(Shift)) >= 64)  \
                                        {(Value).LSW = (Value).MSW = 0;}   \
                                        else  \
                                        {__optasm { \
                                        ldabc (Shift), (Value).LSW, (Value).MSW ;\
                                        lshl    ;\
                                        stab (Value).LSW, (Value).MSW ;}\
                                        }
/*Value=Value>>Shift, where Value is U64 type*/
#define I64_ShiftRight(Shift,Value)     if (((unsigned)(Shift)) >= 64)  \
                                        {(Value).LSW = (Value).MSW = 0;}   \
                                        else  \
                                        {__optasm { \
                                        ldabc Shift, Value.LSW, Value.MSW ;\
                                        lshr    ;\
                                        stab Value.LSW, Value.MSW ;}\
                                        }
#endif /*#ifdef PROCESSOR_C2*/



#ifdef PROCESSOR_C1

/*Value=Value<<Shift, where Value is U64 type*/
#define I64_ShiftLeft(Shift,Value)      if (((unsigned)(Shift)) >= 64)  \
                                        {(Value).LSW = (Value).MSW = 0;}   \
                                        else  \
                                        { \
                                        	if (((unsigned)(Shift)) > 32)  \
                                        	{(Value).MSW = (Value).LSW ; (Value).LSW = 0; \
                                        	 (Value).MSW = ((Value).MSW)<<((Shift-32)<0 ? (-(Shift-32)) : (Shift-32));} \
                                        	 else if (((unsigned)(Shift)) == 32)  \
                                            {(Value).MSW =(Value).LSW; (Value).LSW = 0;} \
                                            else \
                                            { \
                                                register U32 tempLSW = (Value).LSW; \
                                                __optasm { \
                                            	ldab (Shift), (Value).LSW;\
                                            	shl;\
                                            	st (Value).LSW;} \
                                                tempLSW = tempLSW>>((32-Shift)<0 ? (-(32-Shift)) : (32-Shift)); \
                                                __optasm { \
                                            	ldab (Shift), (Value).MSW;\
                                            	shl;\
                                            	st (Value).MSW;} \
                                                (Value).MSW = (Value).MSW+tempLSW; \
                                            } \
                                         }
                                        
/*Value=Value>>Shift, where Value is U64 type*/
#define I64_ShiftRight(Shift,Value)   if (((unsigned)(Shift)) >= 64)  \
                                        {(Value).LSW = (Value).MSW = 0;}   \
                                        else  \
                                        { \
                                        	if (((unsigned)(Shift)) > 32)  \
                                        	{(Value).LSW = (Value).MSW ; (Value).MSW = 0; \
                                        	 (Value).LSW = ((Value).LSW)>>((Shift-32)<0 ? (-(Shift-32)) : (Shift-32));} \
                                        	 else if (((unsigned)(Shift)) == 32)  \
                                            {(Value).LSW =(Value).MSW; (Value).MSW = 0;} \
                                            else \
                                            { \
                                                register U32 tempMSW = (Value).MSW; \
                                                __optasm { \
                                            	ldab (Shift), (Value).MSW;\
                                            	shr;\
                                            	st (Value).MSW;} \
                                                tempMSW = tempMSW<<((32-Shift)<0 ? (-(32-Shift)) : (32-Shift)); \
                                                __optasm { \
                                            	ldab (Shift), (Value).LSW;\
                                            	shr;\
                                            	st (Value).LSW;} \
                                                (Value).LSW = (Value).LSW+tempMSW; \
                                            } \
                                         }

/*Value=A/B, where A is U64 type & B is 32-bit atmost*/
#define I64_DivLit(A,B,Value)          {U64 remnd, r5, thingtoadd, result, tempresult, d1, tempd2, d2; U8 loopctrl; \
                                        remnd.MSW = A.MSW; remnd.LSW = A.LSW;\
                                        d2.LSW = B; \
                                        d2.MSW = result.MSW = result.LSW = 0; \
                                        do{ \
                                          thingtoadd.LSW = tempresult.MSW = tempresult.LSW = r5.MSW = r5.LSW = 0; \
                                          thingtoadd.MSW = 0x00000001; \
                                          d1.MSW = remnd.MSW; d1.LSW = remnd.LSW; \
                                          tempd2.MSW = d2.MSW; tempd2.LSW = d2.LSW; \
                                          I64_ShiftLeft(32,tempd2); \
                                          for(loopctrl = 0; loopctrl<33; loopctrl++) \
                                          { \
                                            if(I64_IsGreaterOrEqual(d1,tempd2)) \
                                          	  { \
                                          		I64_Sub(d1,tempd2,r5); \
                                          		I64_Add(tempresult,thingtoadd,tempresult); \
                                          		d1.MSW = r5.MSW; d1.LSW = r5.LSW; \
                                          	  } \
                                          	I64_ShiftRight(1,thingtoadd); \
                                          	I64_ShiftRight(1,tempd2); \
                                          } \
                                          remnd.MSW = r5.MSW; remnd.LSW = r5.LSW; \
                                          I64_Add(result,tempresult,result); \
                                         } \
                                         while(I64_IsLessThan(d2,remnd)); \
                                         (Value).MSW = result.MSW; (Value).LSW = result.LSW; \
                                        }

/*Value=A%B, where A is U64 type & B is 32-bit atmost*/
#define I64_ModLit(A,B,Value)           {U64 remnd, r5, thingtoadd, result, tempresult, d1, tempd2, d2; U8 loopctrl; \
                                        remnd.MSW = (A).MSW; remnd.LSW = (A).LSW;\
                                        d2.LSW = B; \
                                        d2.MSW = result.MSW = result.LSW = 0; \
                                        do{ \
                                          thingtoadd.LSW = tempresult.MSW = tempresult.LSW = r5.MSW = r5.LSW = 0; \
                                          thingtoadd.MSW = 0x00000001; \
                                          d1.MSW = remnd.MSW; d1.LSW = remnd.LSW; \
                                          tempd2.MSW = d2.MSW; tempd2.LSW = d2.LSW; \
                                          I64_ShiftLeft(32,tempd2); \
                                          for(loopctrl = 0; loopctrl<33; loopctrl++) \
                                          { \
                                            if(I64_IsGreaterOrEqual(d1,tempd2)) \
                                          	  { \
                                          		I64_Sub(d1,tempd2,r5); \
                                          		I64_Add(tempresult,thingtoadd,tempresult); \
                                          		d1.MSW = r5.MSW; d1.LSW = r5.LSW; \
                                          	  } \
                                          	I64_ShiftRight(1,thingtoadd); \
                                          	I64_ShiftRight(1,tempd2); \
                                          } \
                                          remnd.MSW = r5.MSW; remnd.LSW = r5.LSW; \
                                          I64_Add(result,tempresult,result); \
                                        } \
                                        while(I64_IsLessThan(d2,remnd)); \
                                        (Value).MSW = remnd.MSW; (Value).LSW = remnd.LSW; \
                                       }

/*Value=A*B, where A & B are U64 type*/
#define I64_Mul(A,B,Value)              {U64 multiplier, multiplicand, result, Zero; \
                                         multiplier.MSW = B.MSW; multiplier.LSW = B.LSW; multiplicand.MSW = A.MSW; multiplicand.LSW = A.LSW; \
                                         result.MSW = result.LSW = Zero.MSW = Zero.LSW = 0; \
                                         do{ \
                                           if (((multiplier.LSW) & 0x00000001) == 1) \
                                             { \
                                               I64_Add(result,multiplicand,result); \
                                             } \
                                         I64_ShiftRight(1,multiplier); \
                                         I64_ShiftLeft(1,multiplicand); \
                                           } \
                                         while(I64_IsGreaterThan(multiplier,Zero)); \
                                         (Value).LSW = result.LSW; (Value).MSW = result.MSW; \
                                        }

/*Value=A*B, where A is U64 type & B is 32-bit atmost*/
#define I64_MulLit(A,B,Value)           {U64 multiplicand, result; U32 multiplier; \
                                         multiplier = B; multiplicand.MSW = (A).MSW; multiplicand.LSW = (A).LSW; \
                                         result.MSW = result.LSW = 0; \
                                         do{ \
                                           if((multiplier & 0x00000001) == 1) \
                                             { \
                                               I64_Add(result,multiplicand,result); \
                                             } \
                                           multiplier = multiplier>>1; \
                                           I64_ShiftLeft(1,multiplicand); \
                                           } \
                                         while(multiplier>0); \
                                         (Value).LSW = result.LSW; (Value).MSW = result.MSW; \
                                         }

#endif /*#ifdef PROCESSOR_C1*/
#endif /*#ifdef ARCHITECTURE_ST20*/

/*-----------------------------------------------*/

/* Boolean type (values should be among TRUE and FALSE constants only) */
#ifndef DEFINED_BOOL
#define DEFINED_BOOL
typedef int BOOL;
#endif

/* General purpose string type */
typedef char* ST_String_t;

/* Function return error code */
typedef U32 ST_ErrorCode_t;

/* Revision structure */
typedef const char * ST_Revision_t;

#define INCLUDE_GetRevision() ((ST_Revision_t) "INCLUDE-REL_2.1.16")

/* Device name type */
#define ST_MAX_DEVICE_NAME 16  /* 15 characters plus '\0' */
typedef char ST_DeviceName_t[ST_MAX_DEVICE_NAME];

/* Generic partition type */
#ifdef ST_OSLINUX
#ifndef PARTITION_T
#define PARTITION_T
typedef int partition_t;
#endif
#ifndef ST_PARTITION_T
#define ST_PARTITION_T
typedef partition_t ST_Partition_t;
#endif
#endif /*#ifdef ST_OSLINUX*/

#if defined(ST_OS20) || defined(ST_OS21) || defined(ST_OSWINCE)
typedef partition_t ST_Partition_t;
#endif


/*WinCE specific*/
#ifdef ST_OSWINCE
#ifndef CLOCK_T
#define CLOCK_T
typedef int	clock_t;
#endif
#ifndef OSCLOCK_T
#define OSCLOCK_T
typedef clock_t osclock_t;
#endif

#ifndef TASK_T
#define TASK_T
typedef struct	_wince_task 
{
	void			 *hHandle_opaque;				   /* the thread handle (HANDLE) */
	void			 (*pfnThread_opaque)(void* Param); /* the Real STAPI Thread Function */
	void			 *pThreadParameter;				   /* the Real STAPI parameter pointer */
	unsigned long     dwThreadId_opaque;			   /* the thread ID, not used */
	int				  dwOs21TaskPriority;			   /* the task priority from 0 to 255 */
	int				  dwCeTaskPriority;				   /* the task priority from 0 to 255 */
	char 			  tName[40];				       /* task name */
	void			  *dwdatap;						   /* user data */
	struct _wince_task *pThreadNext;				   /* task next linkend */
}task_t;
#endif /* TASK_T */

#ifndef PARTITION_T
#define PARTITION_T
typedef  void partition_t;
#endif

#ifndef WINCE_SEMAPHORE_T
#define WINCE_SEMAPHORE_T
/*#ifndef semaphore_t*/
typedef enum 
{
	SEMAPHORE,
	CRIT_SECT
} WINCE_SYNCH_OBJECT_TYPE;

typedef struct	_semaphore_t 
{
	void							*hHandle_opaque;   /* the Semaphore handle (HANDLE) */
	WINCE_SYNCH_OBJECT_TYPE			eSynchObjType;
	int								dbgcount;
}semaphore_t;
#endif /* WINCE_SEMAPHORE_T */

#ifndef WINCE_MUTEX_T
#define WINCE_MUTEX_T
typedef struct	_mutex_t 
{
	CRITICAL_SECTION hHandle_opaque;					/* the mutex handle (HANDLE) */
}mutex_t;
#endif /* WINCE_MUTEX_T */

#ifndef WINCE_MESSAGE_QUEUE_T
#define WINCE_MESSAGE_QUEUE_T
typedef struct
{
    void ** messages;    /* array of all the messages */
    unsigned int itemNumber;
    size_t       itemSize;
    HANDLE freeMessagesRead;   /* queue of free messages, read-side */
    HANDLE freeMessagesWrite;  /* queue of free messages, write-side */
    HANDLE sentMessagesRead ;  /* queue of sent messages, read-side */
    HANDLE sentMessagesWrite ; /* queue of sent messages, write-side */
    LONG waitingTasks; /* number of tasks waiting on this queue */

    /* for debug only */
    DWORD free, busy;
    DWORD sent, received, claimed, released;
    /* invariant: itemNumber == free + busy + (claimed - sent) + (received - released) */
} message_queue_t;
#endif /* WINCE_MESSAGE_QUEUE_T */

#ifndef task_flags_t
typedef int task_flags_t;       /* Unused */
#endif


typedef void * tdesc_t;
typedef int task_flags_t;
typedef int task_context_t;


typedef struct {
    size_t partition_status_size; /* Total number of bytes within partition */
    size_t partition_status_free; /* Total number of bytes free within partition */
    size_t partition_status_free_largest; /* Total number of bytes within the largest free block in partition */
    size_t partition_status_used; /* Total number of bytes which are allocated/in use within the partition */
} ST_partition_status_t;

extern ST_Partition_t* ST_partition_create(void* memory, size_t size);
extern void            ST_partition_delete(ST_Partition_t* partition);
void *				   ST_partition_malloc(ST_Partition_t* partition,unsigned int size);
void				   ST_partition_free(ST_Partition_t* partition,void *ptr);

extern ST_ErrorCode_t  ST_partition_status(ST_Partition_t* partition, ST_partition_status_t* status);

extern ST_ErrorCode_t  ST_SetOption(int code,...);


typedef enum 
{
ST_SET_WINCE_SHARABLEMEMORY,
ST_INIT_WINCE_SHARABLEMEMORY,
ST_ENUM_CE_TASKS_NAME,
ST_SET_CE_TASKS_PRIORITY,
ST_GET_CE_TASKS_PRIORITY,
ST_ENUM_CE_IST_NAME,		
ST_SET_CE_IST_PRIORITY,
ST_GET_CE_IST_PRIORITY
};

/* Interrupt name */
#ifndef  DEFINED_INTERRUPT_NAME_T
#define  DEFINED_INTERRUPT_NAME_T
typedef int		interrupt_name_t;
#endif


#endif /* ST_OSWINCE */




/* Exported Constants ------------------------------------------------------ */

/* BOOL type constant values */
#ifndef TRUE
    #define TRUE (1 == 1)
#endif
#ifndef FALSE
    #define FALSE (!TRUE)
#endif

/* Common driver error constants */
#define ST_DRIVER_ID   0
#define ST_DRIVER_BASE (ST_DRIVER_ID << 16)
enum
{
    ST_NO_ERROR = ST_DRIVER_BASE,
    ST_ERROR_BAD_PARAMETER,             /* Bad parameter passed       */
    ST_ERROR_NO_MEMORY,                 /* Memory allocation failed   */
    ST_ERROR_UNKNOWN_DEVICE,            /* Unknown device name        */
    ST_ERROR_ALREADY_INITIALIZED,       /* Device already initialized */
    ST_ERROR_NO_FREE_HANDLES,           /* Cannot open device again   */
    ST_ERROR_OPEN_HANDLE,               /* At least one open handle   */
    ST_ERROR_INVALID_HANDLE,            /* Handle is not valid        */
    ST_ERROR_FEATURE_NOT_SUPPORTED,     /* Feature unavailable        */
    ST_ERROR_INTERRUPT_INSTALL,         /* Interrupt install failed   */
    ST_ERROR_INTERRUPT_UNINSTALL,       /* Interrupt uninstall failed */
    ST_ERROR_TIMEOUT,                   /* Timeout occured            */
    ST_ERROR_DEVICE_BUSY                /* Device is currently busy   */
};

/* Exported Variables ------------------------------------------------------ */


/* Exported Macros --------------------------------------------------------- */
#define UNUSED_PARAMETER(x)    (void)(x)

/* Exported Functions ------------------------------------------------------ */

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __STDDEFS_H */

/* End of stddefs.h  ------------------------------------------------------- */


