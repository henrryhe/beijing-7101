/*******************************************************************************

File name   : vid_fgt.h

Description : Film grain technology private header file

COPYRIGHT (C) STMicroelectronics 2006.

Date               Modification                                     Name
----               ------------                                     ----
12 Sept 2006       Created                                           MO
*******************************************************************************/
/* Define to prevent recursive inclusion */

#ifndef __STVID_VIDFGT_H
#define __STVID_VIDFGT_H

/* Includes ----------------------------------------------------------------- */
#ifdef ST_OSLINUX
#include "compat.h"
#else
#include "stlite.h"
#endif
#include "stddefs.h"

#include "stevt.h"
#include "stavmem.h"

#include "stgxobj.h"
#include "stlayer.h"

#include "stvid.h"

#include "vid_com.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef ST_OS21
    #define FORMAT_SPEC_OSCLOCK "ll"
    #define OSCLOCK_T_MILLE     1000LL
    #define OSCLOCK_T_MILLION   1000000LL
#else
    #define FORMAT_SPEC_OSCLOCK ""
    #define OSCLOCK_T_MILLE     1000
    #define OSCLOCK_T_MILLION   1000000
#endif



#define STVID_TASK_STACK_SIZE_FGT                 32768
#define STVID_TASK_PRIORITY_FGT                   5
#define STVID_TASK_PRIORITY_DATABASE_CREATION     1



#define VIDFGT_SEED_LUT_LENGTH         256
#define VIDFGT_TRANSFORM_LOG2SCALE     8
#define VIDFGT_TRANSFORM_ROUNDING      128
#define VIDFGT_TRANSFORM_SIZE          64
#define VIDFGT_GAUSS_LUT_LENGTH        2048
#define VIDFGT_DB_MIN_FREQ_DB          2  /**< Minimum allowed cut frequency to access the film grain database. */
#define VIDFGT_DB_MAX_FREQ_DB          14  /**< Maximum allowed cut frequency to access the film grain database. */
#define VIDFGT_DB_NUM_FREQ_DB          13  /**< Number of different cut frequencies stored in the film grain database. */
#define VIDFGT_DB_NUM_FREQ_HORZ        16  /**< Number of different horizontal cut frequencies */
#define VIDFGT_DB_NUM_FREQ_VERT        16  /**< Number of different vertical cut frequencies */
#define VIDFGT_DB_NUM_SETS             169  /**< Number of film grain sets stored in the film grain pattern database. */
#define VIDFGT_DB_LOG2_SCALE_FACTOR    6  /**< Log2 scale factor for the film grain samples stored in the film grain pattern database. */
#define VIDFGT_DB_PATTERN_SIZE         64  /**< Horizontal and vertical size (pixels) of each pattern stored in the film grain pattern database. */



/* If set, this bit indicates that the film grain process is bypassed =>
   - no LUTs or patterns are transfered into DMEM
   - nothing is sent to the external memory/VDP */
#define VIDFGT_FLAGS_BYPASS_BIT 0
/* If set, this bit indicates that the firmware has to download the LUT tables (i.e the
   VIDFGT_Lut_t record pointed by LutAddr).
   Otherwise, it means that the LUTs are the same as the ones used during the
   previous decode. */
#define VIDFGT_FLAGS_DOWNLOAD_LUTS_BIT 2
/* If set, this bit indicates that firmware has to compute luma grain.
   Otherwise, it means that it has to send instead 0 to the external memory/VDP. */
#define VIDFGT_FLAGS_LUMA_GRAIN_BIT 3
/* If set, this bit indicates that firmware has to compute Cb and Cr grain.
   Otherwise, it means that it has to send instead 0 to the external memory/VDP. */
#define VIDFGT_FLAGS_CHROMA_GRAIN_BIT 4
/* If set, this bit indicates that firmware has to send the grain to the external memory. */
#define VIDFGT_FLAGS_EXT_MEM_BIT 6



#define VIDFGT_FLAGS_BYPASS_MASK               (1 << VIDFGT_FLAGS_BYPASS_BIT)   
#define VIDFGT_FLAGS_DOWNLOAD_LUTS_MASK        (1 << VIDFGT_FLAGS_DOWNLOAD_LUTS_BIT)
#define VIDFGT_FLAGS_LUMA_GRAIN_MASK           (1 << VIDFGT_FLAGS_LUMA_GRAIN_BIT)
#define VIDFGT_FLAGS_CHROMA_GRAIN_MASK         (1 << VIDFGT_FLAGS_CHROMA_GRAIN_BIT)
#define VIDFGT_FLAGS_EXT_MEM_MASK              (1 << VIDFGT_FLAGS_EXT_MEM_BIT)


/* color seed offset */
#define Y_SEED_OFFSET 0
#define Cb_SEED_OFFSET 85
#define Cr_SEED_OFFSET 170

/* Maximum number of grain patterns */
#define MAX_GRAIN_PATTERN 10
/* Size of a grain pattern (in bytes) */
#define GRAIN_PATTERN_SIZE (4 * 1024)
/* Number of color space components */
#define MAX_COLOR_SPACE 3
/* Constants to access arrays indexed by MAX_COLOR_SPACE */
#define Y_INDEX 0
#define CB_INDEX 1
#define CR_INDEX 2
/* Maximum number of elements in a LUT (grain LUT or scaling LUT) */
#define MAX_LUT 256


/* Structure containing all the LUTs
*/
typedef struct
{
    /* GrainLut[c][i] gives an offset to access to the 4K grain pattern for a 8x8 average equal to <i> */
    /* and for the color component <c>. */
    U16 GrainLut[MAX_COLOR_SPACE][MAX_LUT];
    /* ScalingLut[c][i] gives the scaling value which shall be applied to the grain pattern for */
    /* a 8x8 average equal to i and for the color component <c>. */
    U8 ScalingLut[MAX_COLOR_SPACE][MAX_LUT];
} VIDFGT_Lut_t;

/* Structure containing all the decode parameters
*/
typedef struct
{
    /* Bitfield to pass different information to the firmware. */
    /* Note that at least FGT_FLAGS_VDP_BIT bit or FGT_FLAGS_EXT_MEM_BIT bit shall be set. */
    U32 Flags;
    /* Begin address (in external memory) of the luma accumulations for the whole */
    /* picture (it doesn't take into account the viewport coordinates). */
    /* These accumulations are stored in omega2 MB format. */
    /* The address shall be aligned on a 512 bytes boundary (9 less significant bits equal to 0). */
    U32 LumaAccBufferAddr;
    /* Begin address (in external memory) of the chroma accumulations for the whole */
    /* picture (it doesn't take into account the viewport coordinates). */
    /* These accumulations are stored in omega2 MB format. */
    /* The address shall be aligned on a 512 bytes boundary (9 less significant bits equal to 0). */
    U32 ChromaAccBufferAddr;
    /* Number of elements in GrainPatternArray */
    /* From 0 to MAX_GRAIN_PATTERN */
    U32 GrainPatternCount;
    /* GrainPatternAddrArray[i] is the address in external memory of a 4K section of memory */
    /* containing a grain pattern. */
    U32 GrainPatternAddrArray[MAX_GRAIN_PATTERN];
    /* Address in external memory of a VIDFGT_Lut_t record. */
    U32 LutAddr;
    /* Initial seed values for the whole picture. */
    U32 LumaSeed;
    U32 CbSeed;
    U32 CrSeed;
    /* Begin addresses (in external memory) of buffers where the firmware will store */
    /* the grain it has computed. Storage in external memory will be done only if bit */
    /* FGT_FLAGS_EXT_MEM_BIT is set (otherwise grain will be sent to the VDP). */
    /* Grain is stored in 4:2:0 raster 3 buffers format. Only pixels belonging to */
    /* the viewport are written in memory. */
    U32 LumaGrainBufferAddr;
    U32 CbGrainBufferAddr;
    U32 CrGrainBufferAddr;
    /* Logarithmic scale factor. */
    /* Shall be in [2..7]. */
    U32 Log2ScaleFactor;
    
    /* added for simplify code */
    U32 PictureSize;
    U32 FieldType;
    U32 UserZoomOut;
} VIDFGT_TransformParam_t;


#ifdef STVID_VIDFGT_DEBUG
typedef struct VIDFGT_FlexVpParamDebug_s
{
    U32 ID;
    U32 IDExtension;
    U32 DOFID;
    U32 SEIMessageId;
    U32 Flags;
    U32 LumaAccBufferAddr;
    U32 ChromaAccBufferAddr;
    U32 GrainPatternCount;
    U32 GrainPatternAddrArray[MAX_GRAIN_PATTERN];
    U32 LutAddr;
    U32 LumaSeed;
    U32 CbSeed;
    U32 CrSeed;
    U32 Log2ScaleFactor;
    osclock_t Time;
    struct VIDFGT_FlexVpParamDebug_s * Next_p;
} VIDFGT_FlexVpParamDebug_t;
#endif

typedef struct FilmGrainDataBasePattern_s
{
    S8   *Pattern;
    U32  Width;
    U32  Height;
    BOOL IsActive;
    U32  GrainPatternPosition;
} FilmGrainDataBasePattern_t;

typedef enum
{
    VIDFGT_DATABASE_EMPTY,
    VIDFGT_DATABASE_CREATION_ONGOING,
    VIDFGT_DATABASE_CREATED
} VIDFGT_DatabaseStatus_t;


typedef struct FilmGrainDataBase_s
{
    STAVMEM_BlockHandle_t       DatabaseHdl;
    VIDFGT_DatabaseStatus_t     DatabaseStatus;  
    void                        *Database_BaseAdd_p;
    FilmGrainDataBasePattern_t  *databasepatterns[VIDFGT_DB_NUM_FREQ_HORZ][VIDFGT_DB_NUM_FREQ_VERT];
    U8                          scale_factor;
    U8                          num_of_sets;
} FilmGrainDataBase_t;


typedef void * VIDFGT_Handle_t;

typedef struct VIDFGT_InitParams_s
{
    ST_Partition_t *            CPUPartition_p;
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
    ST_DeviceName_t             EventsHandlerName;
    ST_DeviceName_t             VideoName;              /* Name of the video instance */
    STVID_DeviceType_t          DeviceType;
    U8                          DecoderNumber;  /* As defined in vid_com.c. See definition there. */
} VIDFGT_InitParams_t;


typedef enum
{
    VIDFGT_IDLE,
    VIDFGT_CREATE_DATABASE,
    VIDFGT_FREE_DATABASE,
    VIDFGT_UPDATE_VIDFGT_PARAMS
} VIDFGT_Command_t;

typedef struct
{
    ST_Partition_t *                CPUPartition_p;
   	STAVMEM_PartitionHandle_t       AvmemPartitionHandle;
    ST_DeviceName_t                 EventsHandlerName;
    ST_DeviceName_t                 VideoName;              /* Name of the video instance */
    STVID_DeviceType_t              DeviceType;
    U8                              DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    STEVT_Handle_t                  EventsHandle;
    FilmGrainDataBase_t             *Database;
    VIDCOM_FilmGrainSpecificData_t  CurrentFilmGrainData;
    VIDFGT_TransformParam_t         *FlexFwParam[2];
    U32                             ActiveFlexParam;
    STAVMEM_BlockHandle_t           FGTLutHdl;
#ifdef STVID_VIDFGT_DEBUG
    VIDFGT_FlexVpParamDebug_t       *FilmGrainParamsDebug_p;
    VIDFGT_FlexVpParamDebug_t       *FilmGrainParamsFirst_p;
    STAVMEM_BlockHandle_t           DebugHdl;
#endif    
    VIDCOM_Task_t                   VIDFGT_Task;
    semaphore_t                     *ProcessRequest;
    struct
    {
        osclock_t               Time;
        VIDFGT_Command_t        ToDo;
    } ForTask;
    BOOL                            IsFGTActivated;
} VIDFGT_FilmGrainPrivateData_t;

#define clip(x,y,z)  (z>y ? y : ( z<x ? x : z))

/* function prototypes */
ST_ErrorCode_t  VIDFGT_Init(const VIDFGT_InitParams_t * const InitParams_p, VIDFGT_Handle_t * const FilmGrainHandle_p);
ST_ErrorCode_t  VIDFGT_Term(VIDFGT_Handle_t * const FilmGrainHandle_p);
ST_ErrorCode_t  VIDFGT_Activate(const VIDFGT_Handle_t FilmGrainHandle_p);
ST_ErrorCode_t  VIDFGT_Deactivate(const VIDFGT_Handle_t FilmGrainHandle_p);
BOOL            VIDFGT_IsActivated(const VIDFGT_Handle_t FilmGrainHandle_p);
void            VIDFGT_UpdateFilmGrainParams(VIDBUFF_PictureBuffer_t * CurrentBuffer_p,const VIDFGT_Handle_t FilmGrainHandle_p);
BOOL            VIDFGT_IsFilmGrainDatabaseReadytobeUsed(const VIDFGT_Handle_t FilmGrainHandle_p);
void*           VIDFGT_GetFlexVpParamsPointer(const VIDFGT_Handle_t FilmGrainHandle_p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVID_VIDFGT_H */

/* End of vid_fgt.h */
