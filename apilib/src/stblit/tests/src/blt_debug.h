/*******************************************************************************

File name   : blt_debug.h

Description : STBLIT commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2006.

Date                     Modification                                 Name
----                     ------------                                 ----
03 Nov 2006              Created                                      HE
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLIT_DEBUG_H
#define __BLIT_DEBUG_H

/* Includes ----------------------------------------------------------------- */


/* --- Expoted variables --- */
semaphore_t* BlitCompletionSemaphore_p;
semaphore_t* JobCompletionSemaphore_p;
semaphore_t* Task1CompletionSemaphore_p;
semaphore_t* Task2CompletionSemaphore_p;
semaphore_t* Task3CompletionSemaphore_p;
semaphore_t* Task4CompletionSemaphore_p;

/* Exported Functions ------------------------------------------------------- */
#ifdef ST_OSLINUX
ST_ErrorCode_t AllocBitmapBuffer (STGXOBJ_Bitmap_t* Bitmap_p, BOOL SecuredData, void* SharedMemoryBaseAddress_p,
                                  STGXOBJ_BitmapAllocParams_t* Params1_p,
                                  STGXOBJ_BitmapAllocParams_t* Params2_p);
#else /* !ST_OSLINUX */
ST_ErrorCode_t AllocBitmapBuffer (STGXOBJ_Bitmap_t* Bitmap_p , void* SharedMemoryBaseAddress_p, STAVMEM_PartitionHandle_t AVMEMPartition,
                                  STAVMEM_BlockHandle_t*  BlockHandle1_p,
                                  STGXOBJ_BitmapAllocParams_t* Params1_p, STAVMEM_BlockHandle_t*  BlockHandle2_p,
                                  STGXOBJ_BitmapAllocParams_t* Params2_p);
#endif /* ST_OSLINUX */
ST_ErrorCode_t Blit_LoadReferenceFile(char* filename, U32* CRCValuesTab);
void TaskCompletedHandler (STEVT_CallReason_t, const ST_DeviceName_t, STEVT_EventConstant_t, const void*, const void* );
void BlitCompletedHandler (STEVT_CallReason_t, const ST_DeviceName_t, STEVT_EventConstant_t, const void*, const void* );
void BlitDemoCompletedHandler (STEVT_CallReason_t, const ST_DeviceName_t, STEVT_EventConstant_t, const void*, const void* );
void JobCompletedHandler (STEVT_CallReason_t, const ST_DeviceName_t, STEVT_EventConstant_t, const void*, const void* );
U32 GetBitmapCRC(STGXOBJ_Bitmap_t*        Bitmap_p);
ST_ErrorCode_t ConvertBitmapToGamma ( char*, STGXOBJ_Bitmap_t*, STGXOBJ_Palette_t* );
ST_ErrorCode_t ConvertGammaToBitmap ( char*, U8 PartitionInfo, STGXOBJ_Bitmap_t*, STGXOBJ_Palette_t* );
void Blit_InitCrc32table(void);
ST_ErrorCode_t GUTIL_Free(void *ptr);
#endif /* #ifndef ____BLIT_DEBUG_H */
/* End of blit_debug.h */
