/*******************************************************************************

File name   : vid_fgt.c

Description : Film grain technology source file

COPYRIGHT (C) STMicroelectronics 2006

1 TAB = 4 spaces

Date               Modification                                     Name
----               ------------                                     ----
12 Sep 2006        Created                                           MO
06 Jun 2007        Aspect Modifications                              JPB
                   Solve Issues - Value not initialized 
                   Move functions memory_* to STOS_Memory*                       
                   Move functions semaphore_* to STOS_Semaphore*
                   Move functions task_* to STOS_Task*
*******************************************************************************/

#if !defined(ST_OSLINUX)
#include <limits.h>
#include <stdio.h>
#include <string.h>
#endif

#define STTBX_REPORT
#include "sttbx.h"

#include <stdarg.h> /* va_start */
#include "stddefs.h"
#include "stvid.h"
#include "stos.h"

#include "vid_fgt.h"
#include "vid_fgt_data.h"

#ifdef ST_OSLINUX
    #include "compat.h"
#else
    #include "sttbx.h"
#endif /* ST_OSLINUX */

#include "stevt.h"
#include "stdevice.h"

#ifdef ST_OSLINUX
extern STAVMEM_PartitionHandle_t STAVMEM_GetPartitionHandle( U32 PartitionID );
#endif

/*#define STVID_FGT_LOAD_DATABASE*/

/* constantes ----------------------------------------------------------------*/
#ifdef STVID_VIDFGT_DEBUG
#define FGT_DUMP_FLEX_CMD_FILE_NAME  "../../FlexCmd.txt"
#endif /* STVID_VIDFGT_DEBUG */

#ifdef FGT_DATABASE_DUMP
#define FGT_DUMP_DATABASE_FILE_NAME  "../../Database.hex"
#endif /* FGT_DATABASE_DUMP */

/* global variables ----------------------------------------------------------*/
#ifdef STVID_VIDFGT_DEBUG
osclock_t       Local_ClockTicks;
static FILE*    FgtDumpFile_p = NULL;
static int      FgtDumpPictureCount;
#endif /* STVID_VIDFGT_DEBUG */

/* local prototypes declaration ----------------------------------------------*/
#ifdef STVID_FGT_LOAD_DATABASE
static ST_ErrorCode_t   LoadFilmGrainDatabase(      const VIDFGT_Handle_t FilmGrainHandle_p);
#else
static ST_ErrorCode_t   CreateFilmGrainDatabase(    const VIDFGT_Handle_t FilmGrainHandle_p);
static void             FgtRandomNumber(            U32 *seed);
static void             FgtTransformInverse(        S16 in[VIDFGT_TRANSFORM_SIZE][VIDFGT_TRANSFORM_SIZE],
                                                    S8 out[VIDFGT_TRANSFORM_SIZE][VIDFGT_TRANSFORM_SIZE]);
#endif /* STVID_FGT_LOAD_DATABASE */
#ifdef FGT_DATABASE_DUMP
static BOOL             DumpFgtDatabase(            const VIDFGT_Handle_t FilmGrainHandle_p);
#endif /* FGT_DATABASE_DUMP */
#ifdef STVID_VIDFGT_DEBUG
static ST_ErrorCode_t   FgtDebug_Init(              const VIDFGT_Handle_t FilmGrainHandle_p);
static ST_ErrorCode_t   FgtDebug_Term(              const VIDFGT_Handle_t FilmGrainHandle_p);
static void             FgtCopyFlexCmd(             const VIDFGT_Handle_t FilmGrainHandle_p);
static int              myfprintf(                  FILE *stream, const char *format, ...);
#endif /* STVID_VIDFGT_DEBUG */
static ST_ErrorCode_t   StartFilmGrainTask(         const VIDFGT_Handle_t FilmGrainHandle_p);
static ST_ErrorCode_t   StopFilmGrainTask(          const VIDFGT_Handle_t FilmGrainHandle_p);
static void             DeAllocateDatabaseBuffer(   const VIDFGT_Handle_t FilmGrainHandle_p);
static void             UpdateFilmGrainParams(      const VIDFGT_Handle_t FilmGrainHandle_p);
static void             ResetFilmGrainParams(       const VIDFGT_Handle_t FilmGrainHandle_p);
static void             FormatSEIMessage(           const VIDFGT_Handle_t FilmGrainHandle_p);

#ifdef STVID_VIDFGT_DEBUG
/*******************************************************************************
Name        : FgtDebug_Init
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/
  
static ST_ErrorCode_t FgtDebug_Init(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t           *VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    STEVT_OpenParams_t                      STEVT_OpenParams;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    ST_ErrorCode_t                          Error;
    int                                     cpt;
   
    FgtDumpPictureCount = 0;

    FgtDumpFile_p = fopen(FGT_DUMP_FLEX_CMD_FILE_NAME, "w");
    if (FgtDumpFile_p == NULL)
    {
        printf("fopen error on file '%s'\n", FGT_DUMP_FLEX_CMD_FILE_NAME);
        Error = ST_ERROR_OPEN_HANDLE;
    }

    AllocBlockParams.PartitionHandle          = VIDFGT_Data_p->AvmemPartitionHandle;
    AllocBlockParams.Alignment                = 8;
    AllocBlockParams.Size                     = 200 * sizeof(VIDFGT_FlexVpParamDebug_t);
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    STAVMEM_AllocBlock(&AllocBlockParams,&VIDFGT_Data_p->DebugHdl);
    STAVMEM_GetBlockAddress(VIDFGT_Data_p->DebugHdl, (void **)&VirtualAddress);
    /*TBD : check mem allocation*/
    VIDFGT_Data_p->FilmGrainParamsDebug_p = STAVMEM_VirtualToCPU(VirtualAddress,&VirtualMapping);
    VIDFGT_Data_p->FilmGrainParamsFirst_p = VIDFGT_Data_p->FilmGrainParamsDebug_p;
    
    for (cpt = 0; cpt < 199; cpt++)
    {
        VIDFGT_Data_p->FilmGrainParamsDebug_p->Next_p = (VIDFGT_FlexVpParamDebug_t *)((U32)VIDFGT_Data_p->FilmGrainParamsDebug_p + sizeof(VIDFGT_FlexVpParamDebug_t));
        VIDFGT_Data_p->FilmGrainParamsDebug_p = VIDFGT_Data_p->FilmGrainParamsDebug_p->Next_p;
    }
   
    VIDFGT_Data_p->FilmGrainParamsDebug_p->Next_p = NULL;
    VIDFGT_Data_p->FilmGrainParamsDebug_p = VIDFGT_Data_p->FilmGrainParamsFirst_p;
   
    return Error;
}

/*******************************************************************************
Name        : FgtDebug_Term
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static ST_ErrorCode_t FgtDebug_Term(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    STAVMEM_FreeBlockParams_t       AvmemFreeParams;
    int                             cpt;

    VIDFGT_Data_p->FilmGrainParamsDebug_p = NULL;
    while (VIDFGT_Data_p->FilmGrainParamsFirst_p != NULL)
    {
        myfprintf(  FgtDumpFile_p, "Picture :%d (%d/%d)\n",
                    VIDFGT_Data_p->FilmGrainParamsFirst_p->DOFID,
                    VIDFGT_Data_p->FilmGrainParamsFirst_p->IDExtension,
                    VIDFGT_Data_p->FilmGrainParamsFirst_p->ID);
        myfprintf(  FgtDumpFile_p, "Time :%10u us\n",
                    VIDFGT_Data_p->FilmGrainParamsFirst_p->Time*OSCLOCK_T_MILLION/Local_ClockTicks);
        myfprintf(  FgtDumpFile_p, "Accumulation buffer Luma : 0x%.8x  Chroma :0x%.8x\n",
                    VIDFGT_Data_p->FilmGrainParamsFirst_p->LumaAccBufferAddr,
                    VIDFGT_Data_p->FilmGrainParamsFirst_p->ChromaAccBufferAddr);
        myfprintf(  FgtDumpFile_p, "Flags = 0x%.8x\n",
                    VIDFGT_Data_p->FilmGrainParamsFirst_p->Flags);
        if (VIDFGT_Data_p->FilmGrainParamsFirst_p->Flags != VIDFGT_FLAGS_BYPASS_MASK)
        {
            myfprintf(  FgtDumpFile_p, "Grain_Pattern_Count:	 0x%.8x\n",
                        VIDFGT_Data_p->FilmGrainParamsFirst_p->GrainPatternCount);
            for (cpt = 0; cpt < MAX_GRAIN_PATTERN; cpt++)
            {
                myfprintf(  FgtDumpFile_p, "Grain_Pattern_Base_Address_%d:    0x%.8x\n", cpt,
                            (VIDFGT_Data_p->FilmGrainParamsFirst_p->GrainPatternAddrArray[i] != 0) ?
                            VIDFGT_Data_p->FilmGrainParamsFirst_p->GrainPatternAddrArray[cpt] - (U32)(VIDFGT_Data_p->Database->Database_BaseAdd_p) : 0);
            }                            
            myfprintf(  FgtDumpFile_p, "Y_Seed_Init:   0x%x\n",
                        VIDFGT_Data_p->FilmGrainParamsFirst_p->LumaSeed);
            myfprintf(  FgtDumpFile_p, "Cb_Seed_Init:  0x%x\n",
                        VIDFGT_Data_p->FilmGrainParamsFirst_p->CbSeed);
            myfprintf(  FgtDumpFile_p, "Cr_Seed_Init:  0x%x\n",
                        VIDFGT_Data_p->FilmGrainParamsFirst_p->CrSeed);
            myfprintf(  FgtDumpFile_p,"Log2_Scale_Factor:	 0x%x\n\n",
                        VIDFGT_Data_p->FilmGrainParamsFirst_p->Log2ScaleFactor);
        }
        else
        {
            myfprintf(FgtDumpFile_p, "Film grain cancled for this picture\n\n");
        }
        VIDFGT_Data_p->FilmGrainParamsFirst_p = VIDFGT_Data_p->FilmGrainParamsFirst_p->Next_p;
    }

    (void)fclose(FgtDumpFile_p);
    FgtDumpFile_p = NULL;

    AvmemFreeParams.PartitionHandle = VIDFGT_Data_p->AvmemPartitionHandle;
    STAVMEM_FreeBlock(&AvmemFreeParams, &(VIDFGT_Data_p->DebugHdl));
    VIDFGT_Data_p->FilmGrainParamsDebug_p = NULL;
    VIDFGT_Data_p->FilmGrainParamsFirst_p = NULL;

    return ST_NO_ERROR;
}


/*******************************************************************************
Name        : myfprintf
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/
static int myfprintf(FILE *stream, const char *format, ...)
{
    int ret = 0;

    if (stream != NULL)
    {
        va_list argptr;
        va_start(argptr, format);     /* Initialize variable arguments. */
        ret = vfprintf(stream, format, argptr);
        va_end(argptr);
        fflush(stream);
    }
    return ret;
}
#endif /* STVID_VIDFGT_DEBUG */


#ifdef VIDFGT_DATABASE_DUMP
/*******************************************************************************
Name        : DumpFgtDatabase
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/
static BOOL DumpFgtDatabase(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    int                             cpt_k, cpt_i, cpt_j;
    FILE                            *DbFile_p = NULL;
    U32                             Add = 0;
    
    DbFile_p = fopen(VIDFGT_DUMP_DATABASE_FILE_NAME, "w");
    if (DbFile_p == NULL)
    {
        printf("fopen error on file '%s' ***\n", VIDFGT_DUMP_DATABASE_FILE_NAME);
        return(FALSE);
    }

    for (cpt_i = VIDFGT_DB_MIN_FREQ_DB; cpt_i < VIDFGT_DB_MAX_FREQ_DB; cpt_i++)
    {
        for (cpt_j = VIDFGT_DB_MIN_FREQ_DB; cpt_j < VIDFGT_DB_MAX_FREQ_DB; cpt_j++)
        {
            cpt_k = 0;
            while ( cpt_k < (VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Width *
                    VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Height))
            {
                myfprintf(  DbFile_p, "0x%.8x  %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n", Add,
                            VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Pattern[cpt_k++] & 0xFF,
                            VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Pattern[cpt_k++] & 0xFF,
                            VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Pattern[cpt_k++] & 0xFF,
                            VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Pattern[cpt_k++] & 0xFF,
                            VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Pattern[cpt_k++] & 0xFF,
                            VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Pattern[cpt_k++] & 0xFF,
                            VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Pattern[cpt_k++] & 0xFF,
                            VIDFGT_Data_p->Database->databasepatterns[cpt_i][cpt_j]->Pattern[cpt_k++] & 0xFF);
                Add += 8; 
            }
        }
    }
}
#endif /* VIDFGT_DATABASE_DUMP */

#ifdef STVID_VIDFGT_DEBUG
/*******************************************************************************
Name        : FgtCopyFlexCmd
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static void FgtCopyFlexCmd(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    int                             cpt_i;
    VIDFGT_FlexVpParamDebug_t       *ParamDebug_p;
    VIDFGT_TransformParam_t         *FwParam_p;
    
    ParamDebug_p = VIDFGT_Data_p->FilmGrainParamsDebug_p;
    FwParam_p = VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam];
    
    ParamDebug_p->Flags                 = FwParam_p->Flags;
    ParamDebug_p->LumaAccBufferAddr     = FwParam_p->LumaAccBufferAddr;
    ParamDebug_p->ChromaAccBufferAddr   = FwParam_p->ChromaAccBufferAddr;
    if (ParamDebug_p->Flags != VIDFGT_FLAGS_BYPASS_MASK)
    {
        ParamDebug_p->GrainPatternCount = FwParam_p->GrainPatternCount;
        for (cpt_i = 0; cpt_i < MAX_GRAIN_PATTERN; cpt_i++)
        {
            ParamDebug_p->GrainPatternAddrArray[cpt_i] = FwParam_p->GrainPatternAddrArray[i];
        }    
        ParamDebug_p->Log2ScaleFactor   = FwParam_p->Log2ScaleFactor;
        ParamDebug_p->LumaSeed          = FwParam_p->LumaSeed;
        ParamDebug_p->CbSeed            = FwParam_p->CbSeed;
        ParamDebug_p->CrSeed            = FwParam_p->CrSeed;
    }
      
    if (ParamDebug_p->Next_p != NULL)
        ParamDebug_p = ParamDebug_p->Next_p;
    else
        FgtDebug_Term(FilmGrainHandle_p);
}
#endif /* STVID_VIDFGT_DEBUG */


#ifdef STVID_FGT_LOAD_DATABASE
/*******************************************************************************
Name        : LoadFilmGrainDatabase (internal function)
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static ST_ErrorCode_t LoadFilmGrainDatabase(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    VIDFGT_FilmGrainPrivateData_t           *VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void                                    *VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    FilmGrainDataBase_t                     *database;
    void                                    *databaseaddress_p;
    U8                                      val_i,val_j;
    FILE                                    *BinDataBase;
    size_t                                  Sizeread;
 
    database = VIDFGT_Data_p->Database;

#ifdef ST_OSLINUX    
    AllocBlockParams.PartitionHandle          = STAVMEM_GetPartitionHandle(LAYER_PARTITION_AVMEM);
#else    
    AllocBlockParams.PartitionHandle          = VIDFGT_Data_p->AvmemPartitionHandle;
#endif    
    AllocBlockParams.Alignment                = 8;
    AllocBlockParams.Size                     = VIDFGT_DB_NUM_SETS*(VIDFGT_DB_PATTERN_SIZE * VIDFGT_DB_PATTERN_SIZE);
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;
    
#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2(LAYER_PARTITION_AVMEM, &VirtualMapping );
#else    
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#endif    
    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams,&database->DatabaseHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    ErrorCode = STAVMEM_GetBlockAddress(database->DatabaseHdl, (void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    
    databaseaddress_p = STAVMEM_VirtualToCPU(VirtualAddress,&VirtualMapping);
    if(databaseaddress_p == NULL)
        return(ST_ERROR_NO_MEMORY);
      
    database->Database_BaseAdd_p = databaseaddress_p;

    BinDataBase = fopen(STVID_FGT_LOAD_DATABASE,"rb");

    if ( BinDataBase != NULL )
    {
        Sizeread = fread(databaseaddress_p, 1,AllocBlockParams.Size, BinDataBase);
        fclose (BinDataBase);
        if (Sizeread == AllocBlockParams.Size)
        {
            /* Initialize database */
            database->num_of_sets = VIDFGT_DB_NUM_SETS;
            database->scale_factor = VIDFGT_DB_LOG2_SCALE_FACTOR;
 
            for (val_i = 0; val_i < VIDFGT_DB_NUM_FREQ_HORZ; val_i++)
            {
                for (val_j = 0; val_j < VIDFGT_DB_NUM_FREQ_VERT; val_j++)
                {
                    if (    val_i >= VIDFGT_DB_MIN_FREQ_DB && val_i <= VIDFGT_DB_MAX_FREQ_DB && 
                            val_j >= VIDFGT_DB_MIN_FREQ_DB && val_j <= VIDFGT_DB_MAX_FREQ_DB)
                    {
                        database->databasepatterns[val_i][val_j] = STOS_MemoryAllocate(VIDFGT_Data_p->CPUPartition_p, sizeof(FilmGrainDataBasePattern_t));
                        if (database->databasepatterns[val_i][val_j] == NULL)
                            return(ST_ERROR_NO_MEMORY);
                        database->databasepatterns[val_i][val_j]->Pattern = (S8 *)databaseaddress_p;
                        databaseaddress_p = (void *)((U32)databaseaddress_p + (VIDFGT_DB_PATTERN_SIZE*VIDFGT_DB_PATTERN_SIZE));
                        database->databasepatterns[val_i][val_j]->Width = VIDFGT_DB_PATTERN_SIZE;
                        database->databasepatterns[val_i][val_j]->Height = VIDFGT_DB_PATTERN_SIZE;
                    }
                    else
                    {
                        database->databasepatterns[val_i][val_j] = NULL;
                    }
                } 
            }
        }
        return(ST_NO_ERROR);
    }

    STOS_memcpy(    databaseaddress_p,film_grain_pattern_database,
                    VIDFGT_DB_NUM_SETS*(VIDFGT_DB_PATTERN_SIZE * VIDFGT_DB_PATTERN_SIZE));
    database->num_of_sets = VIDFGT_DB_NUM_SETS;
    database->scale_factor = VIDFGT_DB_LOG2_SCALE_FACTOR;

    for (val_i = 0; val_i < VIDFGT_DB_NUM_FREQ_HORZ; val_i++)
    {
        for (val_j = 0; val_j < VIDFGT_DB_NUM_FREQ_VERT; val_j++)
        {
            if (    val_i >= VIDFGT_DB_MIN_FREQ_DB && val_i <= VIDFGT_DB_MAX_FREQ_DB && 
                    val_j >= VIDFGT_DB_MIN_FREQ_DB && val_j <= VIDFGT_DB_MAX_FREQ_DB)
            {
                database->databasepatterns[val_i][val_j] = STOS_MemoryAllocate(VIDFGT_Data_p->CPUPartition_p, sizeof(FilmGrainDataBasePattern_t));
                if (database->databasepatterns[val_i][val_j] == NULL)
                {
                    return(ST_ERROR_NO_MEMORY);
                }    
                database->databasepatterns[val_i][val_j]->Pattern = (S8 *)databaseaddress_p;
                databaseaddress_p = (void *)((U32)databaseaddress_p + (VIDFGT_DB_PATTERN_SIZE*VIDFGT_DB_PATTERN_SIZE));
                database->databasepatterns[val_i][val_j]->Width = VIDFGT_DB_PATTERN_SIZE;
                database->databasepatterns[val_i][val_j]->Height = VIDFGT_DB_PATTERN_SIZE;
            }
            else
            {
                database->databasepatterns[val_i][val_j] = NULL;
            }
        } 
    }
#ifdef VIDFGT_DATABASE_DUMP
    DumpFgtDatabase(FilmGrainHandle_p);
#endif    
    return(ST_NO_ERROR);
}
#else
/*******************************************************************************
Name        : FgtTransformInverse
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static void FgtTransformInverse (S16 in[VIDFGT_TRANSFORM_SIZE][VIDFGT_TRANSFORM_SIZE], S8 out[VIDFGT_TRANSFORM_SIZE][VIDFGT_TRANSFORM_SIZE])
{
    S8 i;
    S8 j;
    S8 k;
    S32 v;
    S16 m[VIDFGT_TRANSFORM_SIZE][VIDFGT_TRANSFORM_SIZE];

    /* Tt * in */
    for (i = 0; i < VIDFGT_TRANSFORM_SIZE; i++)
    {
        for (j = 0; j < VIDFGT_TRANSFORM_SIZE; j++)
        {
            v = 0;
            for (k = 0; k < VIDFGT_TRANSFORM_SIZE; k++)
            {
                v += integer_dct_transposed[k][j] * in[i][k];
            }
            m[i][j] = (S16) ((v + VIDFGT_TRANSFORM_ROUNDING) >> VIDFGT_TRANSFORM_LOG2SCALE);
        }
    }

    /* (Tt * in) * T */
    for (i = 0; i < VIDFGT_TRANSFORM_SIZE; i++)
    {
        for (j = 0; j < VIDFGT_TRANSFORM_SIZE; j++)
        {
            v = 0;
            for (k = 0; k < VIDFGT_TRANSFORM_SIZE; k++)
            {
                v += m[k][j] * integer_dct[i][k];
            }
            /* Scaling */
            v = (v + VIDFGT_TRANSFORM_ROUNDING) >> VIDFGT_TRANSFORM_LOG2SCALE;
            out[i][j] = (S8) (v > 127 ? 127 : v < -127 ? -127 : v);
        }
    }    
}

/*******************************************************************************
Name        : FgtRandomNumber
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static void FgtRandomNumber (U32 * seed)
{
    U32 newbit;

    /* XOR bit 31 with bit 3 */
    newbit = ((*seed >> 30) & 1) ^ ((*seed >> 2) & 1);

    /* Shift the seed one bit to the left and put the new bit as LSB */
    *seed = (*seed << 1) | newbit;
}

/*******************************************************************************
Name        : CreateFilmGrainDatabase
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static ST_ErrorCode_t CreateFilmGrainDatabase(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    STAVMEM_AllocBlockParams_t      AllocBlockParams;
    void *                          VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    FilmGrainDataBase_t             *database;
    void                            *databaseaddress_p;
    S8                              h, v;
    S8                              fh, fv;
    S8                              i, j;
    S16                             k;
    U32                             seed;
    S16                             coeff[VIDFGT_DB_PATTERN_SIZE][VIDFGT_DB_PATTERN_SIZE];
    S8                              grain[VIDFGT_DB_PATTERN_SIZE][VIDFGT_DB_PATTERN_SIZE];
    U8                              edge_scale_factor[VIDFGT_DB_NUM_FREQ_DB] =
                                    { 64, 71, 77, 84, 90, 96, 103, 109, 116, 122, 128, 128, 128 };
    
    database = VIDFGT_Data_p->Database;

#ifdef ST_OSLINUX    
    AllocBlockParams.PartitionHandle          = STAVMEM_GetPartitionHandle(LAYER_PARTITION_AVMEM);
#else    
    AllocBlockParams.PartitionHandle          = VIDFGT_Data_p->AvmemPartitionHandle;
#endif     
    AllocBlockParams.Alignment                = 8;
    AllocBlockParams.Size                     = 169*(VIDFGT_DB_PATTERN_SIZE * VIDFGT_DB_PATTERN_SIZE);
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2(LAYER_PARTITION_AVMEM, &VirtualMapping );
#else    
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#endif          
    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams,&database->DatabaseHdl);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    
    ErrorCode = STAVMEM_GetBlockAddress(database->DatabaseHdl, (void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    databaseaddress_p = STAVMEM_VirtualToCPU(VirtualAddress,&VirtualMapping);
    if(databaseaddress_p == NULL)
        return(ST_ERROR_NO_MEMORY);

    database->Database_BaseAdd_p = databaseaddress_p;

    /* Initialize database */
    database->num_of_sets = VIDFGT_DB_NUM_SETS;
    database->scale_factor = VIDFGT_DB_LOG2_SCALE_FACTOR;

    /* Create film grain patterns */
    for (v = 0; v < VIDFGT_DB_NUM_FREQ_VERT; v++)
    {
        for (h = 0; h < VIDFGT_DB_NUM_FREQ_HORZ; h++)
        {
            if (h >= VIDFGT_DB_MIN_FREQ_DB && h <= VIDFGT_DB_MAX_FREQ_DB && 
                v >= VIDFGT_DB_MIN_FREQ_DB && v <= VIDFGT_DB_MAX_FREQ_DB)
            {
                /* Initialize seed */
                seed = Seed_LUT[(h - 2) + VIDFGT_DB_NUM_FREQ_DB * (v - 2)];

                /* Scale cut frequencies from 16x16 to 64x64 */
                fh = (h+1) * 4 - 1;
                fv = (v+1) * 4 - 1;

                /* Create block of transformed coefficients */
                for (j = 0; j < VIDFGT_DB_PATTERN_SIZE; j++)
                {
                    for (i = 0; i < VIDFGT_DB_PATTERN_SIZE; i+=4)
                    {
                        /* Coefficients outside the band pass are set to 0 */
                        if (i > fh  || j > fv)
                        {
                            coeff[i][j]   = 0;
                            coeff[i+1][j] = 0;
                            coeff[i+2][j] = 0;
                            coeff[i+3][j] = 0;
                        }
                        else
                        {
                            /* Update seed */
                            FgtRandomNumber (&seed);

                            /* Set remaining coefficients to a gaussian value */
                            coeff[i][j]   = Gauss_LUT[seed % VIDFGT_GAUSS_LUT_LENGTH];
                            coeff[i+1][j] = Gauss_LUT[(seed+1) % VIDFGT_GAUSS_LUT_LENGTH];
                            coeff[i+2][j] = Gauss_LUT[(seed+2) % VIDFGT_GAUSS_LUT_LENGTH];
                            coeff[i+3][j] = Gauss_LUT[(seed+3) % VIDFGT_GAUSS_LUT_LENGTH];
                        }
                    }
                }

                /* Set DC coefficient to 0 */
                coeff[0][0] = 0;

                /* Inverse transform */
                FgtTransformInverse (coeff, grain);

                /* Scale top and bottom edges */
                for (j = 0; j < VIDFGT_DB_PATTERN_SIZE; j += 8)
                {
                    for (i = 0; i < VIDFGT_DB_PATTERN_SIZE; i++)
                    {
                        grain[i][j] = (grain[i][j] * edge_scale_factor[v-2]) >> 7;
                        grain[i][j+7] = (grain[i][j+7] * edge_scale_factor[v-2]) >> 7;
                    }
                }

                /* Add pattern to film grain database */
                database->databasepatterns[h][v] = STOS_MemoryAllocate(VIDFGT_Data_p->CPUPartition_p, sizeof(FilmGrainDataBasePattern_t));
                if(database->databasepatterns[h][v] == NULL)
                  return(ST_ERROR_NO_MEMORY);
                database->databasepatterns[h][v]->Pattern = (S8 *)databaseaddress_p;
                databaseaddress_p = (void *)((U32)databaseaddress_p + (VIDFGT_DB_PATTERN_SIZE*VIDFGT_DB_PATTERN_SIZE));
                database->databasepatterns[h][v]->Width = VIDFGT_DB_PATTERN_SIZE;
                database->databasepatterns[h][v]->Height = VIDFGT_DB_PATTERN_SIZE;
                k = 0;

                for (j = 0; j < VIDFGT_DB_PATTERN_SIZE; j++)
                {
                    for (i= 0; i < VIDFGT_DB_PATTERN_SIZE; i++)
                    {
                        database->databasepatterns[h][v]->Pattern[k++] = grain[i][j];
                    }
                }
            }
            else
            {
                /* Film grain pattern (h,v) is not stored in the database */
                database->databasepatterns[h][v] = NULL;
            }
        }
    }
#ifdef VIDFGT_DATABASE_DUMP
    DumpFgtDatabase(FilmGrainHandle_p);
#endif
return(ST_NO_ERROR);
}
#endif /* STVID_FGT_LOAD_DATABASE */

/*******************************************************************************
Name        : DeAllocateDatabaseBuffer
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static void DeAllocateDatabaseBuffer (const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    STAVMEM_FreeBlockParams_t       AvmemFreeParams;
    ST_ErrorCode_t                  Err = ST_NO_ERROR;
    int                             h, v;

    if (VIDFGT_Data_p->Database != NULL)
    {
        for (h = 0; h < VIDFGT_DB_NUM_FREQ_HORZ; h++)
        {
            for (v = 0; v < VIDFGT_DB_NUM_FREQ_VERT; v++)
            {
                if( VIDFGT_Data_p->Database->databasepatterns[h][v] != NULL)
                {
                    STOS_MemoryDeallocate(VIDFGT_Data_p->CPUPartition_p, VIDFGT_Data_p->Database->databasepatterns[h][v]);
                }    
            }
        }
        if (VIDFGT_Data_p->Database->Database_BaseAdd_p != NULL)
        {
            AvmemFreeParams.PartitionHandle = VIDFGT_Data_p->AvmemPartitionHandle;
            Err = STAVMEM_FreeBlock(&AvmemFreeParams, &(VIDFGT_Data_p->Database->DatabaseHdl));
        }
    }
}

/*******************************************************************************
Name        : ResetFilmGrainParams
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static void ResetFilmGrainParams(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    VIDFGT_TransformParam_t         *VIDFGT_FlexVpParam_p ;
    int                             cpt_h, cpt_v, cpt_i;
    VIDFGT_Lut_t                    *VIDFGT_Lut_p;

    VIDFGT_FlexVpParam_p  = (VIDFGT_TransformParam_t  *)VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam];
    VIDFGT_Lut_p = (VIDFGT_Lut_t *)VIDFGT_FlexVpParam_p->LutAddr;
  
    for (cpt_v = VIDFGT_DB_MIN_FREQ_DB; cpt_v <= VIDFGT_DB_MAX_FREQ_DB; cpt_v++)
    {
        for (cpt_h = VIDFGT_DB_MIN_FREQ_DB; cpt_h <= VIDFGT_DB_MAX_FREQ_DB; cpt_h++)
        {
            VIDFGT_Data_p->Database->databasepatterns[cpt_h][cpt_v]->IsActive = FALSE;
            VIDFGT_Data_p->Database->databasepatterns[cpt_h][cpt_v]->GrainPatternPosition = 0;
        }
    }
  
    VIDFGT_FlexVpParam_p->GrainPatternCount = 0;
    memset(VIDFGT_Lut_p, 0, sizeof(VIDFGT_Lut_t));
    for (cpt_i = 0; cpt_i < MAX_GRAIN_PATTERN; cpt_i++)
    {
        VIDFGT_FlexVpParam_p->GrainPatternAddrArray[cpt_i] = 0;
    }        
}

/*******************************************************************************
Name        : FormatSEIMessage
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static void FormatSEIMessage(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;
    VIDFGT_TransformParam_t         *VIDFGT_FlexVpParam_p ;
    int                             i, c, j;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    VIDFGT_FlexVpParam_p  = (VIDFGT_TransformParam_t  *)VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam];
  
    for (c = 0; c < MAX_COLOR_SPACE; c++)
    {
        for (i = 0; i <= VIDFGT_Data_p->CurrentFilmGrainData.num_intensity_intervals_minus1[c]; i++)
        {
            if(VIDFGT_Data_p->CurrentFilmGrainData.num_model_values_minus1[c] == 0)
            {
                VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][1] = 8;
                VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][2] = VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][1];
            }
            if(VIDFGT_Data_p->CurrentFilmGrainData.num_model_values_minus1[c] == 1)
            {
                VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][2] = VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][1]; 
            }
    
            for (j = 1; j < 3; j++)
            {
                if(c == Y_INDEX)
                    VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][j] = clip(2,14,VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][j]);
                else
                    VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][j] = clip(2,14,(VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][j] << 1));  
            }
        }
    }
}

/*******************************************************************************
Name        : UpdateFilmGrainParams
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

static void UpdateFilmGrainParams(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;
    VIDFGT_TransformParam_t         *VIDFGT_FlexVpParam_p ;
    int                             i, c, j;
    VIDFGT_Lut_t                    *VIDFGT_Lut_p;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    VIDFGT_FlexVpParam_p  = (VIDFGT_TransformParam_t  *)VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam];
    VIDFGT_Lut_p = (VIDFGT_Lut_t *)VIDFGT_FlexVpParam_p->LutAddr;
  
    ResetFilmGrainParams(FilmGrainHandle_p);
    FormatSEIMessage(FilmGrainHandle_p);
  
    VIDFGT_FlexVpParam_p->Flags = VIDFGT_FLAGS_DOWNLOAD_LUTS_MASK; 
  
    if(VIDFGT_Data_p->CurrentFilmGrainData.comp_model_present_flag[Y_INDEX] == TRUE)
    {
        VIDFGT_FlexVpParam_p->Flags |= VIDFGT_FLAGS_LUMA_GRAIN_MASK;
    }
    if( (VIDFGT_Data_p->CurrentFilmGrainData.comp_model_present_flag[CB_INDEX] == TRUE) ||
        (VIDFGT_Data_p->CurrentFilmGrainData.comp_model_present_flag[CR_INDEX] == TRUE))
    {
        VIDFGT_FlexVpParam_p->Flags |= VIDFGT_FLAGS_CHROMA_GRAIN_MASK;
    }

    VIDFGT_FlexVpParam_p->Log2ScaleFactor = VIDFGT_Data_p->CurrentFilmGrainData.log2_scale_factor;
    VIDFGT_FlexVpParam_p->LumaSeed  = Seed_LUT[(VIDFGT_Data_p->CurrentFilmGrainData.picture_offset + Y_SEED_OFFSET)&0xFF];
    VIDFGT_FlexVpParam_p->CbSeed    = Seed_LUT[(VIDFGT_Data_p->CurrentFilmGrainData.picture_offset + Cb_SEED_OFFSET)&0xFF];
    VIDFGT_FlexVpParam_p->CrSeed    = Seed_LUT[(VIDFGT_Data_p->CurrentFilmGrainData.picture_offset + Cr_SEED_OFFSET)&0xFF];
 
    for (c = 0; c < MAX_COLOR_SPACE; c++)
    {
        if (VIDFGT_Data_p->CurrentFilmGrainData.comp_model_present_flag[c] == TRUE)
        {
            for (i = 0; i <= VIDFGT_Data_p->CurrentFilmGrainData.num_intensity_intervals_minus1[c]; i++)
            {
                if (!VIDFGT_Data_p->Database->databasepatterns[VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][1]][VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][2]]->IsActive)
                {
                    if (VIDFGT_FlexVpParam_p->GrainPatternCount < MAX_GRAIN_PATTERN)
                    {
                        VIDFGT_FlexVpParam_p->GrainPatternAddrArray[VIDFGT_FlexVpParam_p->GrainPatternCount] = (U32)(VIDFGT_Data_p->Database->databasepatterns[VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][1]][VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][2]]->Pattern);
                        VIDFGT_Data_p->Database->databasepatterns[VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][1]][VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][2]]->IsActive = TRUE;
                        VIDFGT_Data_p->Database->databasepatterns[VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][1]][VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][2]]->GrainPatternPosition = VIDFGT_FlexVpParam_p->GrainPatternCount;
                        for (   j = VIDFGT_Data_p->CurrentFilmGrainData.intensity_interval_lower_bound[c][i];
                                j <= VIDFGT_Data_p->CurrentFilmGrainData.intensity_interval_upper_bound[c][i];
                                j++)
                        {
                            VIDFGT_Lut_p->GrainLut[c][j] = VIDFGT_FlexVpParam_p->GrainPatternCount * 4* 1024; 
                            if (c == Y_INDEX)
                                VIDFGT_Lut_p->ScalingLut[c][j] = VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][0];
                            else
                                VIDFGT_Lut_p->ScalingLut[c][j] = VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][0] >> 1;
                        }
                        VIDFGT_FlexVpParam_p->GrainPatternCount++;
                    }
                    else
                    {
                        STTBX_Print(( "Warning grain pattern count(%d) greater than allowed value(%d).",VIDFGT_FlexVpParam_p->GrainPatternCount+1,MAX_GRAIN_PATTERN)); 
                    }
                }
                else
                {
                    for (   j = VIDFGT_Data_p->CurrentFilmGrainData.intensity_interval_lower_bound[c][i];
                            j <= VIDFGT_Data_p->CurrentFilmGrainData.intensity_interval_upper_bound[c][i];
                            j++)
                    {
                        VIDFGT_Lut_p->GrainLut[c][j] = VIDFGT_Data_p->Database->databasepatterns[VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][1]][VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][2]]->GrainPatternPosition * 4* 1024;
                        if (c == Y_INDEX)
                            VIDFGT_Lut_p->ScalingLut[c][j] = VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][0];
                        else
                            VIDFGT_Lut_p->ScalingLut[c][j] = VIDFGT_Data_p->CurrentFilmGrainData.comp_model_value[c][i][0] >> 1;
                    }
                } 
            }
        }
    }
}

/*******************************************************************************
Name        : FilmGrainTaskFunc (internal function)
Description : Film Grain main task callback
Parameters  : 
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/

static void FilmGrainTaskFunc(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    
    do
    {
        STOS_SemaphoreWait(VIDFGT_Data_p->ProcessRequest);
        switch(VIDFGT_Data_p->ForTask.ToDo)
        {
        case VIDFGT_CREATE_DATABASE:
            VIDFGT_Data_p->ForTask.Time = time_now(); 
#ifndef STVID_FGT_LOAD_DATABASE        
            /* task_priority_set(NULL,STVID_TASK_PRIORITY_DATABASE_CREATION); */
            ErrorCode = CreateFilmGrainDatabase(FilmGrainHandle_p);
            /* task_priority_set(NULL,STVID_TASK_PRIORITY_FGT); */
#else
            ErrorCode = LoadFilmGrainDatabase(FilmGrainHandle_p);
#endif        
            VIDFGT_Data_p->ForTask.Time = time_minus(time_now(),VIDFGT_Data_p->ForTask.Time);
            VIDFGT_Data_p->ForTask.Time = VIDFGT_Data_p->ForTask.Time * OSCLOCK_T_MILLE /ST_GetClocksPerSecond();
            if(ErrorCode == ST_NO_ERROR)
            {
                VIDFGT_Data_p->Database->DatabaseStatus = VIDFGT_DATABASE_CREATED;
            }    
            else
            {
                DeAllocateDatabaseBuffer(FilmGrainHandle_p);
                VIDFGT_Data_p->Database->DatabaseStatus = VIDFGT_DATABASE_EMPTY;
            }
            VIDFGT_Data_p->ForTask.ToDo = VIDFGT_IDLE;
            VIDFGT_Data_p->IsFGTActivated = TRUE;
            break;
       
        case VIDFGT_FREE_DATABASE:
            DeAllocateDatabaseBuffer(FilmGrainHandle_p);
            VIDFGT_Data_p->Database->DatabaseStatus = VIDFGT_DATABASE_EMPTY;
            VIDFGT_Data_p->ForTask.ToDo = VIDFGT_IDLE;
            VIDFGT_Data_p->IsFGTActivated = FALSE;
            break;
            
        default:
            break;  
        }
    }
    while (!(VIDFGT_Data_p->VIDFGT_Task.ToBeDeleted));
}

/*******************************************************************************
Name        : VIDFGT_GetFlexVpParamsPointer
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/

void * VIDFGT_GetFlexVpParamsPointer(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;
    void                            *FlexVpParams_p;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    if( (VIDFGT_IsFilmGrainDatabaseReadytobeUsed(FilmGrainHandle_p)) &&
        !(VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam]->Flags & VIDFGT_FLAGS_BYPASS_MASK))
        FlexVpParams_p = (void *)(VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam]);
    else
        FlexVpParams_p = NULL;
      
    return(FlexVpParams_p);      
}

/*******************************************************************************
Name        : VIDFGT_Activate
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/

ST_ErrorCode_t VIDFGT_Activate(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    ST_ErrorCode_t                  ErrCode = ST_NO_ERROR;
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;
    
    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
        
    ErrCode = StartFilmGrainTask(FilmGrainHandle_p);
    if (ErrCode == ST_NO_ERROR)
    {
#ifdef STVID_VIDFGT_DEBUG
        FgtDebug_Init(FilmGrainHandle_p);
        Local_ClockTicks = (osclock_t) ST_GetClocksPerSecond();
#endif
    
        VIDFGT_Data_p->CurrentFilmGrainData.FgtMessageId = 0;
        if(VIDFGT_Data_p->Database->DatabaseStatus == VIDFGT_DATABASE_EMPTY)
        {
            VIDFGT_Data_p->ForTask.ToDo = VIDFGT_CREATE_DATABASE;
            STOS_SemaphoreSignal(VIDFGT_Data_p->ProcessRequest);
        }
    }        
    return ErrCode;
} 

/*******************************************************************************
Name        : VIDFGT_Deactivate
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/

ST_ErrorCode_t VIDFGT_Deactivate(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;
    
    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
            
    if(VIDFGT_Data_p->Database->DatabaseStatus == VIDFGT_DATABASE_CREATED)
    {
        VIDFGT_Data_p->ForTask.ToDo = VIDFGT_FREE_DATABASE;
        STOS_SemaphoreSignal(VIDFGT_Data_p->ProcessRequest);
    }

#ifdef STVID_VIDFGT_DEBUG
    FgtDebug_Term(FilmGrainHandle_p);
#endif
    
    if (VIDFGT_Data_p->Database->DatabaseStatus == VIDFGT_DATABASE_EMPTY)
    {
        StopFilmGrainTask(FilmGrainHandle_p);
        VIDFGT_Data_p->IsFGTActivated = FALSE;
    }
    return ST_NO_ERROR;
} 


/*******************************************************************************
Name        : VIDFGT_IsFilmGrainDatabaseReadytobeUsed
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : TRUE if database is ready to be used,
              FALSE
*******************************************************************************/

BOOL VIDFGT_IsFilmGrainDatabaseReadytobeUsed(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p; /* To do : check if film grain structure exist*/
    if(VIDFGT_Data_p->Database->DatabaseStatus == VIDFGT_DATABASE_CREATED)
        return(TRUE);
/* merged from thierry delahaye branch, but unused in this version, incompatible with VIDFGT_Activate
    else
    {    
        if(VIDFGT_Data_p->Database->DatabaseStatus != VIDFGT_DATABASE_CREATION_ONGOING)
            VIDFGT_CreateDatabase(FilmGrainHandle_p);
    }
*/        
    return(FALSE);
} 

/*******************************************************************************
Name        : VIDFGT_UpdateFilmGrainParams
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
              
*******************************************************************************/

void VIDFGT_UpdateFilmGrainParams(VIDBUFF_PictureBuffer_t * CurrentBuffer_p, const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    
    if (    (VIDFGT_Data_p->IsFGTActivated == TRUE) &&
            (VIDFGT_IsFilmGrainDatabaseReadytobeUsed(FilmGrainHandle_p)) &&
            (CurrentBuffer_p->ParsedPictureInformation.PictureFilmGrainSpecificData.IsFilmGrainEnabled == TRUE) &&
            (CurrentBuffer_p->FrameBuffer_p->ToBeKilledAsSoonAsPossible == 0))    
    {
        if (    CurrentBuffer_p->ParsedPictureInformation.PictureFilmGrainSpecificData.FgtMessageId !=
                VIDFGT_Data_p->CurrentFilmGrainData.FgtMessageId)
        {
            VIDFGT_Data_p->ActiveFlexParam = (VIDFGT_Data_p->ActiveFlexParam == 0 ? 1 : 0);
            VIDFGT_Data_p->CurrentFilmGrainData = CurrentBuffer_p->ParsedPictureInformation.PictureFilmGrainSpecificData;
            VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam]->LumaAccBufferAddr   = (U32)(CurrentBuffer_p->FrameBuffer_p->Allocation.FGTLumaAccumBuffer_p);
            VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam]->ChromaAccBufferAddr = (U32)(CurrentBuffer_p->FrameBuffer_p->Allocation.FGTChromaAccumBuffer_p);
#ifdef STVID_VIDFGT_DEBUG
            if (VIDFGT_Data_p->FilmGrainParamsDebug_p != NULL)
                VIDFGT_Data_p->FilmGrainParamsDebug_p->Time = time_now();
#endif
            UpdateFilmGrainParams(FilmGrainHandle_p);
#ifdef STVID_VIDFGT_DEBUG
            if(VIDFGT_Data_p->FilmGrainParamsDebug_p != NULL)
            {
                VIDFGT_Data_p->FilmGrainParamsDebug_p->Time = time_minus(time_now(),VIDFGT_Data_p->FilmGrainParamsDebug_p->Time);
                VIDFGT_Data_p->FilmGrainParamsDebug_p->IDExtension = CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension;
                VIDFGT_Data_p->FilmGrainParamsDebug_p->ID = CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;
                VIDFGT_Data_p->FilmGrainParamsDebug_p->DOFID = CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID;
                FgtCopyFlexCmd(FilmGrainHandle_p);
            }
#endif
        }
        else
        {
            VIDFGT_Data_p->ActiveFlexParam = (VIDFGT_Data_p->ActiveFlexParam == 0 ? 1 : 0);
            STOS_memcpy( VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam],
                    VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam == 0 ? 1 : 0],
                    sizeof(VIDFGT_TransformParam_t));
            VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam]->Flags &= ~VIDFGT_FLAGS_DOWNLOAD_LUTS_MASK;
#ifdef STVID_VIDFGT_DEBUG
            if(VIDFGT_Data_p->FilmGrainParamsDebug_p != NULL)
            {
                VIDFGT_Data_p->FilmGrainParamsDebug_p->Time = 0;
                VIDFGT_Data_p->FilmGrainParamsDebug_p->IDExtension = CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension;
                VIDFGT_Data_p->FilmGrainParamsDebug_p->ID = CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;
                VIDFGT_Data_p->FilmGrainParamsDebug_p->DOFID = CurrentBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID;
                FgtCopyFlexCmd(FilmGrainHandle_p);
            }
#endif
        }
    }
    else
    {
        VIDFGT_Data_p->FlexFwParam[VIDFGT_Data_p->ActiveFlexParam]->Flags = VIDFGT_FLAGS_BYPASS_MASK;
    }
}

/*******************************************************************************
Name        : StartFilmGrainTask
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/

static ST_ErrorCode_t StartFilmGrainTask(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;
    VIDCOM_Task_t                   *Task_p;
    ST_ErrorCode_t                  ErrCode = ST_NO_ERROR;
    
    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    Task_p = &(VIDFGT_Data_p->VIDFGT_Task);

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    Task_p->ToBeDeleted = FALSE; 

    ErrCode = STOS_TaskCreate(  (void(*)(void*))FilmGrainTaskFunc,
                                (void *) FilmGrainHandle_p,
                                VIDFGT_Data_p->CPUPartition_p,
                                STVID_TASK_STACK_SIZE_FGT,
                                &(Task_p->TaskStack),
                                VIDFGT_Data_p->CPUPartition_p,
                                &(Task_p->Task_p),
                                &(Task_p->TaskDesc),
                                STVID_TASK_PRIORITY_FGT,
                                "STVID.FGTTask",
                                task_flags_no_min_stack_size);
    if (ErrCode != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);   
    }                                     

    Task_p->IsRunning  = TRUE;
    
    return(ST_NO_ERROR);
} /* End of StartFilmGrainTask() function */


/*******************************************************************************
Name        : StopFilmGrainTask
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopFilmGrainTask(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;
    VIDCOM_Task_t                   *Task_p;
    task_t                          *TaskList_p ;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    Task_p = &(VIDFGT_Data_p->VIDFGT_Task);

    TaskList_p = Task_p->Task_p;
    if (Task_p->IsRunning)
    {
        /* End the function of the task here... */
        Task_p->ToBeDeleted = TRUE;
        
        /* deblock task main loop */
        VIDFGT_Data_p->ForTask.ToDo = VIDFGT_IDLE;
        STOS_SemaphoreSignal(VIDFGT_Data_p->ProcessRequest);
                
        STOS_TaskWait( &TaskList_p, TIMEOUT_INFINITY);       
        STOS_TaskDelete(Task_p->Task_p, NULL, NULL, NULL);

        Task_p->IsRunning  = FALSE;
    }

    return(ST_NO_ERROR);
} /* End of StopFilmGrainTask() function */


/*******************************************************************************
Name        : VIDFGT_Init
Description : Init film grain module
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t VIDFGT_Init(const VIDFGT_InitParams_t * const InitParams_p, VIDFGT_Handle_t * const FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t           *VIDFGT_Data_p;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void *                                  VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    VIDCOM_Task_t                           *Task_p;
    ST_ErrorCode_t                          Error;
    
    /* Allocate a film grain structure */
    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VIDFGT_FilmGrainPrivateData_t));
    if (VIDFGT_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    *FilmGrainHandle_p = (VIDFGT_Handle_t *) VIDFGT_Data_p;
   
    /* Store parameters */
    VIDFGT_Data_p->CPUPartition_p       = InitParams_p->CPUPartition_p;
    VIDFGT_Data_p->DeviceType           = InitParams_p->DeviceType;
    VIDFGT_Data_p->DecoderNumber        = InitParams_p->DecoderNumber;
    strcpy(VIDFGT_Data_p->VideoName, InitParams_p->VideoName);
    VIDFGT_Data_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
    VIDFGT_Data_p->IsFGTActivated       = FALSE;
    strcpy(VIDFGT_Data_p->EventsHandlerName,InitParams_p->EventsHandlerName);
    
    /*Allocate database structure*/
    VIDFGT_Data_p->Database = (FilmGrainDataBase_t *) STOS_MemoryAllocate(VIDFGT_Data_p->CPUPartition_p, sizeof(FilmGrainDataBase_t));
    /*TBD : check mem allocation*/
    VIDFGT_Data_p->Database->DatabaseStatus = VIDFGT_DATABASE_EMPTY;
    VIDFGT_Data_p->Database->Database_BaseAdd_p = NULL;
    
    /*Allocate the first film grain params structure*/
    VIDFGT_Data_p->FlexFwParam[0] = (VIDFGT_TransformParam_t  *) STOS_MemoryAllocate(VIDFGT_Data_p->CPUPartition_p, sizeof(VIDFGT_TransformParam_t ));
    VIDFGT_Data_p->FlexFwParam[1] = (VIDFGT_TransformParam_t  *) STOS_MemoryAllocate(VIDFGT_Data_p->CPUPartition_p, sizeof(VIDFGT_TransformParam_t ));
    VIDFGT_Data_p->ActiveFlexParam = 0;
    /*TBD : check mem allocation*/
   
#ifdef ST_OSLINUX    
    AllocBlockParams.PartitionHandle          = STAVMEM_GetPartitionHandle(LAYER_PARTITION_AVMEM);
#else    
    AllocBlockParams.PartitionHandle          = VIDFGT_Data_p->AvmemPartitionHandle;
#endif    
    AllocBlockParams.Alignment                = 8;
    AllocBlockParams.Size                     = 2 * sizeof(VIDFGT_Lut_t);
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;
#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2(LAYER_PARTITION_AVMEM, &VirtualMapping );
#else    
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#endif    
    Error = STAVMEM_AllocBlock(&AllocBlockParams,&VIDFGT_Data_p->FGTLutHdl);
    if (Error != ST_NO_ERROR)
    {
        return (Error);
    }
    Error = STAVMEM_GetBlockAddress(VIDFGT_Data_p->FGTLutHdl, (void **)&VirtualAddress);
    if (Error != ST_NO_ERROR)
    {
        return (Error);
    }    
    VIDFGT_Data_p->FlexFwParam[0]->LutAddr = (U32 )STAVMEM_VirtualToCPU(VirtualAddress,&VirtualMapping);
    VIDFGT_Data_p->FlexFwParam[1]->LutAddr = VIDFGT_Data_p->FlexFwParam[0]->LutAddr + sizeof(VIDFGT_Lut_t);
    
    /* task is not running */
    Task_p = &(VIDFGT_Data_p->VIDFGT_Task);
    Task_p->IsRunning = FALSE;
    VIDFGT_Data_p->ProcessRequest = STOS_SemaphoreCreateFifo(NULL, 0);
    if (VIDFGT_Data_p->ProcessRequest == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
     
    return(ST_NO_ERROR);             
}

/*******************************************************************************
Name        : VIDFGT_Term
Description : film grain module termination
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/

ST_ErrorCode_t VIDFGT_Term(VIDFGT_Handle_t * const FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t           *VIDFGT_Data_p;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t*)FilmGrainHandle_p;
    if (VIDFGT_Data_p)
    {
        if (VIDFGT_Data_p->IsFGTActivated == TRUE)
            VIDFGT_Deactivate(FilmGrainHandle_p);
        STOS_SemaphoreDelete(NULL, VIDFGT_Data_p->ProcessRequest);    
        STOS_MemoryDeallocate(VIDFGT_Data_p->CPUPartition_p,VIDFGT_Data_p->FlexFwParam[0]);
        STOS_MemoryDeallocate(VIDFGT_Data_p->CPUPartition_p,VIDFGT_Data_p->FlexFwParam[1]);
        STOS_MemoryDeallocate(VIDFGT_Data_p->CPUPartition_p, VIDFGT_Data_p);            
    }
           
    return ST_NO_ERROR;
}    
/*******************************************************************************
Name        : VIDFGT_IsActivated
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/

BOOL VIDFGT_IsActivated(const VIDFGT_Handle_t FilmGrainHandle_p)
{
    VIDFGT_FilmGrainPrivateData_t   *VIDFGT_Data_p;

    VIDFGT_Data_p = (VIDFGT_FilmGrainPrivateData_t *)FilmGrainHandle_p;
    if (VIDFGT_Data_p)
    {
        return VIDFGT_Data_p->IsFGTActivated;      
    }
    return FALSE;     
}    

