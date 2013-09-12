/*******************************************************************************

File name   : halv_vin.c

Description : HAL video input source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Jul 2000        Created                                        Jerome Audu
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif /* ST_OSLINUX */

#include "sttbx.h"
#include "stddefs.h"

#include "stvin.h"
#include "vin_init.h"
#include "halv_vin.h"

#include "omega2/hv_sd_in.h"
#include "omega2/hv_hd_in.h"
#include "omega2/hv_dvp.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define HALVIN_VALID_HANDLE 0x0A01A01f

#define STVIN_MAX_ANCILLARY_DATA_SIZE           (21*944)/* Standard case ! 21 lines, 944 pixels/line PAL-SQ Standard */


/* Private Variables (static)------------------------------------------------ */

/*
  => the 7015/7020 Chip
 -----------------------

                       |   External Sync  |   CCIR    |   Interlace / Progressiv  |
             ==========+==================+===========+===========================+  \
                 RGB   |        X         |           | toggle bit in "Digital1"  |   |
             ----------+------------------+-----------+---------------------------+   |
               YCbCr   |                  |           |                           |   |
               24 bits |        X         |           | toggle bit in "Digital1"  |   |
               4:4:4   |                  |           |                           |   |   HD Mode
             ----------+------------------+-----------+---------------------------+   |
               YCbCr   |                  |           |                           |   |
               16 bits |                  |     X     | toggle bit in "Digital1"  |   |
               Chroma/ |                  |           |                           |   |
               Luma    |                  |           |                           |   |
             ----------+------------------+-----------+---------------------------+  /
               YCbCr   |        X         |     X     |     TVS bit in SDPP       |  \
               8 bits  |                  |           |        register           |   |   SD Mode
             ----------+------------------+-----------+---------------------------+  /

                    +--------+
           ---------|  SD 0  |------
                    +--------+
                    +--------+
                +---|  SD 1  |------
                |   +--------+      the Input for SD1 & HD are the same
                |   +--------+
           -----+---|   HD   |------
                    +--------+

*/


static const STVIN_DefInputMode_t CheckInputParam[7][7][2] = {
    {
        /* DeviceType: STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT */

        /*   External Sync        Embedded Sync   */
        {STVIN_INPUT_MODE_SD, STVIN_INPUT_MODE_SD},           /* YCbCr 8bit             */
        {STVIN_INPUT_INVALID, STVIN_INPUT_MODE_HD},           /* 16bit Chroma/Luma      */
        {STVIN_INPUT_MODE_HD, STVIN_INPUT_INVALID},           /* YCbCr 24bit            */
        {STVIN_INPUT_MODE_HD, STVIN_INPUT_INVALID},           /* RGB 24bit => YCbCr 24bit */
        {STVIN_INPUT_MODE_HD, STVIN_INPUT_INVALID},           /* RGB 24bit              */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* CD_SERIAL_MULT (?)     */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID}            /* CD_MULT (?)            */
    },
    {
        /* DeviceType: STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT */

        /*   External Sync        Embedded Sync   */
        {STVIN_INPUT_MODE_SD, STVIN_INPUT_MODE_SD},           /* YCbCr 8bit             */
        {STVIN_INPUT_INVALID, STVIN_INPUT_MODE_HD},           /* 16bit Chroma/Luma      */
        {STVIN_INPUT_MODE_HD, STVIN_INPUT_INVALID},           /* YCbCr 24bit            */
        {STVIN_INPUT_MODE_HD, STVIN_INPUT_INVALID},           /* RGB 24bit => YCbCr 24bit */
        {STVIN_INPUT_MODE_HD, STVIN_INPUT_INVALID},           /* RGB 24bit              */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* CD_SERIAL_MULT (?)     */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID}            /* CD_MULT (?)            */
    },
    {
        /* DeviceType: STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT */

        /*   External Sync        Embedded Sync   */
        {STVIN_INPUT_MODE_SD, STVIN_INPUT_MODE_SD},           /* YCbCr 8bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* 16bit Chroma/Luma */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit => YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* CD_SERIAL_MULT (?) */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID}            /* CD_MULT (?) */
    },
    {
        /* DeviceType: STVIN_DEVICE_TYPE_ST5528_DVP_INPUT */
        {STVIN_INPUT_MODE_SD, STVIN_INPUT_MODE_SD},           /* YCbCr 8bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* 16bit Chroma/Luma */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit => YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* CD_SERIAL_MULT (?) */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID}            /* CD_MULT (?) */
    },
    {
        /* DeviceType: STVIN_DEVICE_TYPE_ST7710_DVP_INPUT */
        {STVIN_INPUT_MODE_SD, STVIN_INPUT_MODE_SD},           /* YCbCr 8bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* 16bit Chroma/Luma */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit => YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* CD_SERIAL_MULT (?) */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID}            /* CD_MULT (?) */
    },
    {
        /* DeviceType: STVIN_DEVICE_TYPE_ST7100_DVP_INPUT */
        {STVIN_INPUT_MODE_SD, STVIN_INPUT_MODE_SD},           /* YCbCr 8bit */
        {STVIN_INPUT_MODE_HD, STVIN_INPUT_MODE_HD},           /* 16bit Chroma/Luma */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit => YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* CD_SERIAL_MULT (?) */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID}            /* CD_MULT (?) */
    },
    {
        /* DeviceType: STVIN_DEVICE_TYPE_ST7109_DVP_INPUT */
        {STVIN_INPUT_MODE_SD, STVIN_INPUT_MODE_SD},           /* YCbCr 8bit */
        {STVIN_INPUT_MODE_HD, STVIN_INPUT_MODE_HD},           /* 16bit Chroma/Luma */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit => YCbCr 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* RGB 24bit */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID},           /* CD_SERIAL_MULT (?) */
        {STVIN_INPUT_INVALID, STVIN_INPUT_INVALID}            /* CD_MULT (?) */
    }

};


/* Warning!: See STVIN_InputMode_t typedef befor any changes !!! */
static const STVIN_VideoParams_t CheckInitVideoParams[] = {
    /*---------------------*/
    /* Standard Definition */
    /*---------------------*/

    /* STVIN_SD_NTSC_720_480_I_CCIR */
    {STVIN_INPUT_DIGITAL_YCbCr422_8_MULT,
     STGXOBJ_INTERLACED_SCAN,
     STVIN_SYNC_TYPE_CCIR,
     STVIN_InputPath_PAD,
     720, 480,
     138, 45,              /* 138, 45 in External Sync */
     0, 528,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 0,
     {{STVIN_STANDARD_NTSC, STVIN_ANC_DATA_RAW_CAPTURE, 0, STVIN_FIRST_PIXEL_IS_COMPLETE}}
    },
    /* STVIN_SD_NTSC_640_480_I_CCIR */
    {STVIN_INPUT_DIGITAL_YCbCr422_8_MULT,
     STGXOBJ_INTERLACED_SCAN,
     STVIN_SYNC_TYPE_CCIR,
     STVIN_InputPath_PAD,
     640, 480,
     140, 45,           /* 140, 45 in External Sync */
     0, 528,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 0,
     {{STVIN_STANDARD_NTSC_SQ, STVIN_ANC_DATA_RAW_CAPTURE, 0, STVIN_FIRST_PIXEL_IS_COMPLETE}}
    },
    /* STVIN_SD_PAL_720_576_I_CCIR */
    {STVIN_INPUT_DIGITAL_YCbCr422_8_MULT,
     STGXOBJ_INTERLACED_SCAN,
     STVIN_SYNC_TYPE_CCIR,
     STVIN_InputPath_PAD,
     720, 576,
     144, 49,           /* 144, 49 in External Sync */
     0, 528,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 0,
     {{STVIN_STANDARD_PAL, STVIN_ANC_DATA_RAW_CAPTURE, 0, STVIN_FIRST_PIXEL_IS_COMPLETE}}
    },
    /* STVIN_SD_PAL_768_576_I_CCIR */
    {STVIN_INPUT_DIGITAL_YCbCr422_8_MULT,
     STGXOBJ_INTERLACED_SCAN,
     STVIN_SYNC_TYPE_CCIR,
     STVIN_InputPath_PAD,
     768, 576,
     176, 49,           /* 176, 49 in External Sync */
     0, 528,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 0,
     {{STVIN_STANDARD_PAL_SQ, STVIN_ANC_DATA_RAW_CAPTURE, 0, STVIN_FIRST_PIXEL_IS_COMPLETE}}
    },
    /*-----------------*/
    /* High Definition */
    /*-----------------*/

    /* STVIN_HD_YCbCr_1920_1080_I_CCIR */
    {STVIN_INPUT_DIGITAL_YCbCr422_16_CHROMA_MULT,
     STGXOBJ_INTERLACED_SCAN,
     STVIN_SYNC_TYPE_CCIR,
     STVIN_InputPath_PAD,
     1920, 1080,
     130, 20,
     1440, 2400,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 20,
     {{STVIN_RISING_EDGE, STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES, STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH, 0}}
    },
    /* STVIN_HD_YCbCr_1280_720_P_CCIR */
    {STVIN_INPUT_DIGITAL_YCbCr422_16_CHROMA_MULT,
     STGXOBJ_PROGRESSIVE_SCAN,
     STVIN_SYNC_TYPE_CCIR,
     STVIN_InputPath_PAD,
     1280, 720,
     220, 16,
     411, 1240,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 16,
     {{STVIN_RISING_EDGE, STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES, STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH, 0}}
    },
    /* STVIN_HD_RGB_1024_768_P_EXT */
    /* TODO: Update sync timming !*/
    {STVIN_INPUT_DIGITAL_RGB888_24_COMP_TO_YCbCr422,
     STGXOBJ_PROGRESSIVE_SCAN,
     STVIN_SYNC_TYPE_EXTERNAL,
     STVIN_InputPath_PAD,
     1024, 768,
     0, 0,
     0, 0,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 0,
     {{STVIN_RISING_EDGE, STVIN_FIELD_DETECTION_METHOD_LINE_COUNTING, STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH, 0}}
    },
    /* STVIN_HD_RGB_800_600_P_EXT */
    /* TODO: Update sync timming !*/
    {STVIN_INPUT_DIGITAL_RGB888_24_COMP_TO_YCbCr422,
     STGXOBJ_PROGRESSIVE_SCAN,
     STVIN_SYNC_TYPE_EXTERNAL,
     STVIN_InputPath_PAD,
     800, 600,
     0, 0,
     0, 0,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 0,
     {{STVIN_RISING_EDGE, STVIN_FIELD_DETECTION_METHOD_LINE_COUNTING, STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH, 0}}
    },
    /* STVIN_HD_RGB_640_480_P_EXT */
    /* TODO: Update sync timming !*/
    {STVIN_INPUT_DIGITAL_RGB888_24_COMP_TO_YCbCr422,
     STGXOBJ_PROGRESSIVE_SCAN,
     STVIN_SYNC_TYPE_EXTERNAL,
     STVIN_InputPath_PAD,
     640, 480,
     0, 0,
     480, 800,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 16,
     {{STVIN_RISING_EDGE, STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES, STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH, 0}}
    },
    /* STVIN_HD_YCbCr_720_480_P_CCIR */
    {STVIN_INPUT_DIGITAL_YCbCr422_16_CHROMA_MULT,
     STGXOBJ_PROGRESSIVE_SCAN,
     STVIN_SYNC_TYPE_CCIR,
     STVIN_InputPath_PAD,
     720, 480,
     220, 16,
     411, 1240,
     STVIN_RISING_EDGE, STVIN_RISING_EDGE,
     0, 0, 16,
     {{STVIN_RISING_EDGE, STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES, STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH, 0}}
    },
};


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t GetInputNumber(const STVIN_DeviceType_t DeviceType,
        const void * RegistersBaseAddress_p,
        STVIN_InputMode_t InputMode,
        U8 * const InputNumber_p,
        STVIN_DefInputMode_t * const DefInputMode_p);


/* Functions ---------------------------------------------------------------- */


/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : HALVIN_AllocateMemoryForAncillaryData
Description : Allocate the table for ancillary data capture.
Parameters  : IN : HAL input handle
              IN : Number of table to allocate
              OUT: Ancillary Data Table parameters to set
Assumptions :
Limitations : Every table's size will default one, i.e. STVIN_MAX_ANCILLARY_DATA_SIZE.
Returns     : ST_NO_ERROR
              ST_ERROR_BAD_PARAMETER
              ST_ERROR_NO_MEMORY
*******************************************************************************/
ST_ErrorCode_t HALVIN_AllocateMemoryForAncillaryData(
        const HALVIN_Handle_t InputHandle,
        U32 NumberOfTable,
        U32 TableSize,
        void *AncillaryDataTable_p)
{
    U32                             Loop;
    STAVMEM_AllocBlockParams_t      AvmemAllocParams;
    HALVIN_Properties_t *           HALVIN_Data_p;
    void                          * TableVirtualAddress;
    void                          * TableAddress;
    stvin_AncillaryBufferParams_t * AncillaryDataElementTable_p;
    STAVMEM_BlockHandle_t           TableHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    char                            ResetPattern = 0x00;

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if ( (InputHandle == NULL) || (AncillaryDataTable_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Test basic buffer occupancy. */
    if ((TableSize == 0)     || (TableSize > STVIN_MAX_ANCILLARY_DATA_SIZE) ||
        (NumberOfTable == 0) || (NumberOfTable > STVIN_MAX_NB_BUFFER_FOR_ANCILLARY_DATA))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Find structure.                                                        */
    HALVIN_Data_p = (HALVIN_Properties_t *) InputHandle;
    AncillaryDataElementTable_p = (stvin_AncillaryBufferParams_t *)AncillaryDataTable_p;

    AvmemAllocParams.Alignment       = 256;
    AvmemAllocParams.PartitionHandle = HALVIN_Data_p->AVMEMPartitionHandle;
    AvmemAllocParams.Size            = TableSize + STVIN_MAX_ANCILLARY_DATA_SIZE;
    AvmemAllocParams.AllocMode       = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AvmemAllocParams.NumberOfForbiddenRanges    = 0;
    AvmemAllocParams.ForbiddenRangeArray_p      = NULL;
    AvmemAllocParams.NumberOfForbiddenBorders   = 0;
    AvmemAllocParams.ForbiddenBorderArray_p     = NULL;

    for (Loop = 0; Loop < NumberOfTable; Loop ++)
    {
        ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &TableHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            if ( (ErrorCode == STAVMEM_ERROR_PARTITION_FULL) ||
                 (ErrorCode == STAVMEM_ERROR_MAX_BLOCKS) )
            {
                ErrorCode = ST_ERROR_NO_MEMORY;
            }
            else
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;
        }
        else
        {
            ErrorCode = STAVMEM_GetBlockAddress(TableHandle, &TableVirtualAddress);

            TableAddress = STAVMEM_VirtualToDevice(TableVirtualAddress,
                &HALVIN_Data_p->VirtualMapping);
            if (ErrorCode != ST_NO_ERROR)
            {
                ErrorCode = ST_ERROR_NO_MEMORY;
                break;
            }
            else
            {
                AncillaryDataElementTable_p->TotalSize        = TableSize + STVIN_MAX_ANCILLARY_DATA_SIZE;
                AncillaryDataElementTable_p->Address_p        = TableAddress;
                AncillaryDataElementTable_p->VirtualAddress_p = TableVirtualAddress;
                AncillaryDataElementTable_p->AvmemBlockHandle = TableHandle;

                /* And reset its content.       */
                ErrorCode = STAVMEM_FillBlock1D((void *)&ResetPattern, sizeof (char),
                        TableVirtualAddress, (TableSize + STVIN_MAX_ANCILLARY_DATA_SIZE));
            }
        }
        AncillaryDataElementTable_p ++;
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* An Error occured during allocation. De-allocate all buffer (if possible).    */
        HALVIN_DeAllocateMemoryForAncillaryData(InputHandle, NumberOfTable,
                AncillaryDataTable_p);
    }

    return(ErrorCode);

} /* End of HALVIN_AllocateMemoryForAncillaryData() function. */

/*******************************************************************************
Name        : HALVIN_DeAllocateMemoryForAncillaryData
Description : DeAllocate the table for ancillary data capture.
Parameters  : IN : HAL input handle
              IN : Number of table to allocate
              OUT: Ancillary Data Table parameters to set
Assumptions : .
Limitations : .
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t HALVIN_DeAllocateMemoryForAncillaryData(
        const HALVIN_Handle_t InputHandle,
        U32 NumberOfTable,
        void *AncillaryDataTable_p)
{
    U32                             Loop;
    STAVMEM_FreeBlockParams_t       AvmemFreeParams;
    stvin_AncillaryBufferParams_t * AncillaryDataElementTable_p;
    HALVIN_Properties_t *           HALVIN_Data_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    if ( (InputHandle == NULL) || (AncillaryDataTable_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Find structure.                                                        */
    HALVIN_Data_p = (HALVIN_Properties_t *) InputHandle;
    AncillaryDataElementTable_p = (stvin_AncillaryBufferParams_t *)AncillaryDataTable_p;

    AvmemFreeParams.PartitionHandle = HALVIN_Data_p->AVMEMPartitionHandle;

    for (Loop = 0; Loop < NumberOfTable; Loop ++)
    {
        if ( (AncillaryDataElementTable_p->AvmemBlockHandle != 0) &&
             (AncillaryDataElementTable_p->AvmemBlockHandle != STAVMEM_INVALID_BLOCK_HANDLE))
        {
            STAVMEM_FreeBlock(&AvmemFreeParams, &(AncillaryDataElementTable_p->AvmemBlockHandle));
            AncillaryDataElementTable_p->AvmemBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
        }
        AncillaryDataElementTable_p ++;
    }

    return(ErrorCode);

} /* End of HALVIN_DeAllocateMemoryForAncillaryData() function. */

/*******************************************************************************
Name        : HALVIN_Init
Description : Init Input HAL
Parameters  : Input type, Input registers address
Assumptions : Init params, pointer to handle to return, pointer to video input number to return
Limitations :
Returns     : in params: HAL input handle
*******************************************************************************/
ST_ErrorCode_t HALVIN_Init(const HALVIN_InitParams_t * const InputInitParams_p, HALVIN_Handle_t * const InputHandle_p)
{
    ST_ErrorCode_t Err;
    HALVIN_Properties_t * HALVIN_Data_p;
    STVIN_DefInputMode_t CheckDefInputMode;
    U8 InputNumber;
    STVIN_DefInputMode_t DefInputMode;
    STVIN_InputType_t CheckInputType;
    STVIN_SyncType_t CheckSyncType;

#define CHECK_ERROR_AND_EXIT_IF_FAILED(_StatusToCheck) {        \
        if (_StatusToCheck != ST_NO_ERROR)                      \
        {                                                       \
            /* Error: Failed!, Undo initialisations done */     \
            HALVIN_Term(*InputHandle_p);                        \
            return(_StatusToCheck);                             \
        }                                                       \
    }

    /* Test input parameter validity, not to dereference NULL pointer. */
    if (InputInitParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate a HALVIN structure */
    HALVIN_Data_p = (HALVIN_Properties_t *) memory_allocate(InputInitParams_p->CPUPartition_p,
                                                            sizeof(HALVIN_Properties_t));
    if (HALVIN_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *InputHandle_p = (HALVIN_Handle_t *) HALVIN_Data_p;
    HALVIN_Data_p->ValidityCheck        = HALVIN_VALID_HANDLE;
    HALVIN_Data_p->CPUPartition_p       = InputInitParams_p->CPUPartition_p;
    HALVIN_Data_p->VideoParams_p        = InputInitParams_p->VideoParams_p;
    HALVIN_Data_p->DeviceType           = InputInitParams_p->DeviceType;
    HALVIN_Data_p->InputMode            = InputInitParams_p->InputMode;
    HALVIN_Data_p->FunctionsTable_p     = (HALVIN_FunctionsTable_t *)NULL;
    HALVIN_Data_p->AVMEMPartitionHandle = InputInitParams_p->AVMEMPartionHandle;

    STAVMEM_GetSharedMemoryVirtualMapping(&(HALVIN_Data_p->VirtualMapping));

    /* Check Init parameter */
    CheckInputType = CheckInitVideoParams[InputInitParams_p->InputMode].InputType;
    CheckSyncType =  CheckInitVideoParams[InputInitParams_p->InputMode].SyncType;
    CheckDefInputMode = CheckInputParam[InputInitParams_p->DeviceType][CheckInputType][CheckSyncType];

    /* Get the SD Interface Number */
    Err = GetInputNumber(InputInitParams_p->DeviceType, InputInitParams_p->RegistersBaseAddress_p, InputInitParams_p->InputMode, &InputNumber, &DefInputMode);
    CHECK_ERROR_AND_EXIT_IF_FAILED(Err);
    HALVIN_Data_p->InputNumber  = InputNumber;
    HALVIN_Data_p->DefInputMode = DefInputMode;

    if (CheckDefInputMode != DefInputMode)
    {
        /* Input mode impossible. Deallocate memory and return. */
        HALVIN_Term(*InputHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((InputNumber != 0) &&
        ((InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT) ||
         (InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT))) {
        switch (CheckDefInputMode)
        {
            case STVIN_INPUT_MODE_SD:
                /* Perform initialisation and configuration of the HD interface */
                /*--------------------------------------------------------------*/

                /* On the 7015/7020/ST40GFX1, we must perform some init in HD mode... */
                /* First, we have to desactivate the HD interface & switch to SD input mode (HD block) */
                HALVIN_Data_p->FunctionsTable_p = &HALVIN_HDOmega2Functions;
                HALVIN_Data_p->RegistersBaseAddress_p = (void *)((U32)(InputInitParams_p->DeviceBaseAddress_p) +
                                                                 (U32)(InputInitParams_p->RegistersBaseAddress_p) +
                                                                 (U32)(0x100));

                Err = (HALVIN_Data_p->FunctionsTable_p->Init)(*InputHandle_p);
                CHECK_ERROR_AND_EXIT_IF_FAILED(Err);

                /* Select Standard definition Pixel Interface */
                (HALVIN_Data_p->FunctionsTable_p->SelectInterfaceMode)(*InputHandle_p, HALVIN_SD_PIXEL_INTERFACE);

                /* Deactivate the HD interface (To Check!) */
                (HALVIN_Data_p->FunctionsTable_p->DisableInterface)(*InputHandle_p);

                /* Then switch to SD function table & base adresse */
                /*-------------------------------------------------*/
                HALVIN_Data_p->FunctionsTable_p = &HALVIN_SDOmega2Functions;
                HALVIN_Data_p->RegistersBaseAddress_p = (void *)((U32)(InputInitParams_p->DeviceBaseAddress_p) +
                                                                 (U32)(InputInitParams_p->RegistersBaseAddress_p));

                break;
            case STVIN_INPUT_MODE_HD:
                /* First, we have to desactivate the SD interface */
                HALVIN_Data_p->FunctionsTable_p = &HALVIN_SDOmega2Functions;
                HALVIN_Data_p->RegistersBaseAddress_p = (void *)((U32)(InputInitParams_p->DeviceBaseAddress_p) +
                                                                 (U32)(InputInitParams_p->RegistersBaseAddress_p) -
                                                                 (U32)(0x100));

                Err = (HALVIN_Data_p->FunctionsTable_p->Init)(*InputHandle_p);
                CHECK_ERROR_AND_EXIT_IF_FAILED(Err);

                /* Deactivate the SD interface (To Check!) */
                (HALVIN_Data_p->FunctionsTable_p->DisableInterface)(*InputHandle_p);

                /* Then switch to HD function table & base adresse */
                /*-------------------------------------------------*/
                HALVIN_Data_p->FunctionsTable_p = &HALVIN_HDOmega2Functions;
                HALVIN_Data_p->RegistersBaseAddress_p = (void *)((U32)(InputInitParams_p->DeviceBaseAddress_p) +
                                                                 (U32)(InputInitParams_p->RegistersBaseAddress_p));

                break;
            case STVIN_INPUT_INVALID:
                HALVIN_Term(*InputHandle_p);
                return(ST_ERROR_BAD_PARAMETER);
            default:
                HALVIN_Term(*InputHandle_p);
                return(ST_ERROR_BAD_PARAMETER);
        }
    }
    else
    {
        switch (CheckDefInputMode)
        {
            case STVIN_INPUT_MODE_SD:
                /* SD function table & base adresse */
                /*----------------------------------*/
                if ( (InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT) ||
                     (InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_ST5528_DVP_INPUT)  ||
                     (InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_ST7710_DVP_INPUT)  ||
                     (InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_ST7100_DVP_INPUT)  ||
                     (InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_ST7109_DVP_INPUT) )
                {
                    HALVIN_Data_p->FunctionsTable_p = &HALVIN_DVPOmega2Functions;
                }
                else
                {
                    HALVIN_Data_p->FunctionsTable_p = &HALVIN_SDOmega2Functions;
                }

                HALVIN_Data_p->RegistersBaseAddress_p = (void *)((U32)(InputInitParams_p->DeviceBaseAddress_p) +
                                                                 (U32)(InputInitParams_p->RegistersBaseAddress_p));
                /* Select Standard definition Pixel Interface */
                (HALVIN_Data_p->FunctionsTable_p->SelectInterfaceMode)(*InputHandle_p, HALVIN_SD_PIXEL_INTERFACE);

                break;
            case STVIN_INPUT_MODE_HD:
                /* HD function table & base adresse */
                /*----------------------------------*/
                if ( (InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_ST7100_DVP_INPUT)  ||
                     (InputInitParams_p->DeviceType == STVIN_DEVICE_TYPE_ST7109_DVP_INPUT) )
                {
                    HALVIN_Data_p->FunctionsTable_p = &HALVIN_DVPOmega2Functions;
                }
                else
                {
                    HALVIN_Data_p->FunctionsTable_p = &HALVIN_HDOmega2Functions;
                }

                HALVIN_Data_p->RegistersBaseAddress_p = (void *)((U32)(InputInitParams_p->DeviceBaseAddress_p) +
                                                                 (U32)(InputInitParams_p->RegistersBaseAddress_p));
                /* Select High definition Pixel Interface */
                (HALVIN_Data_p->FunctionsTable_p->SelectInterfaceMode)(*InputHandle_p, HALVIN_EMBEDDED_SYNC_MODE);

                break;
            default:
                HALVIN_Term(*InputHandle_p);
                return(ST_ERROR_BAD_PARAMETER);
        }
    }

    /* Initialise input */
    Err = (HALVIN_Data_p->FunctionsTable_p->Init)(*InputHandle_p);
    CHECK_ERROR_AND_EXIT_IF_FAILED(Err);

    (HALVIN_Data_p->FunctionsTable_p->SetInputPath)(*InputHandle_p, HALVIN_Data_p->VideoParams_p->InputPath);

    /* Update "VideoParams_p" struct */
    memcpy(HALVIN_Data_p->VideoParams_p,
           &(CheckInitVideoParams[InputInitParams_p->InputMode]),
           sizeof(STVIN_VideoParams_t));

    /* Set default value for Input/output Window - no Zoom */
    HALVIN_Data_p->InputWindow.InputWinX = 0;
    HALVIN_Data_p->InputWindow.InputWinY = 0;
    HALVIN_Data_p->InputWindow.InputWinWidth = HALVIN_Data_p->VideoParams_p->FrameWidth;
    HALVIN_Data_p->InputWindow.InputWinHeight = HALVIN_Data_p->VideoParams_p->FrameHeight;
    HALVIN_Data_p->InputWindow.OutputWinWidth = HALVIN_Data_p->VideoParams_p->FrameWidth;
    HALVIN_Data_p->InputWindow.OutputWinHeight = HALVIN_Data_p->VideoParams_p->FrameHeight;

    /* Then call all the HAL_xxx fonctions! */

    /* Size of the Frame */
    Err = HALVIN_SetSizeOfTheFrame(*InputHandle_p,
                                   HALVIN_Data_p->VideoParams_p->FrameWidth,
                                   HALVIN_Data_p->VideoParams_p->FrameHeight);
    CHECK_ERROR_AND_EXIT_IF_FAILED(Err);

    /* Scan Type */
    Err = HALVIN_SetScanType(*InputHandle_p, HALVIN_Data_p->VideoParams_p->ScanType);
    CHECK_ERROR_AND_EXIT_IF_FAILED(Err);

    /* Sync Type */
    Err = HALVIN_SetSyncType(*InputHandle_p, HALVIN_Data_p->VideoParams_p->SyncType);
    CHECK_ERROR_AND_EXIT_IF_FAILED(Err);

    /* Blancking Offset */
    HALVIN_SetBlankingOffset(*InputHandle_p,
                             HALVIN_Data_p->VideoParams_p->VerticalBlankingOffset,
                             HALVIN_Data_p->VideoParams_p->HorizontalBlankingOffset);

    /* Set a window to dertermine field type */
    switch (CheckDefInputMode)
    {
        case STVIN_INPUT_MODE_SD:
            Err = HALVIN_SetFieldDetectionMethod(*InputHandle_p,
                                                 STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES,
                                                 HALVIN_Data_p->VideoParams_p->HorizontalLowerLimit,
                                                 HALVIN_Data_p->VideoParams_p->HorizontalUpperLimit);
            break;
        case STVIN_INPUT_MODE_HD:
            Err = HALVIN_SetFieldDetectionMethod(*InputHandle_p,
                                                HALVIN_Data_p->VideoParams_p->ExtraParams.HighDefinitionParams.DetectionMethod,
                                                HALVIN_Data_p->VideoParams_p->HorizontalLowerLimit,
                                                HALVIN_Data_p->VideoParams_p->HorizontalUpperLimit);
            break;
        default:
            HALVIN_Term(*InputHandle_p);
            return(ST_ERROR_BAD_PARAMETER);
    }
    CHECK_ERROR_AND_EXIT_IF_FAILED(Err);

    /* Polarity of HSync & VSync  */
    HALVIN_SetSyncActiveEdge(*InputHandle_p,
                             HALVIN_Data_p->VideoParams_p->HorizontalSyncActiveEdge,
                             HALVIN_Data_p->VideoParams_p->VerticalSyncActiveEdge);

    /* Constants parameters - need to be hard fixed ! */
    HALVIN_SetVSyncOutLineOffset(*InputHandle_p,
                                 HALVIN_Data_p->VideoParams_p->HorizontalVSyncOutLineOffset,
                                 HALVIN_Data_p->VideoParams_p->VerticalVSyncOutLineOffset);
    HALVIN_SetHSyncOutHorizontalOffset(*InputHandle_p,
                                       HALVIN_Data_p->VideoParams_p->HSyncOutHorizontalOffset);

    /* Activate interface - *Must* be perform in Video driver */
/*    (HALVIN_Data_p->FunctionsTable_p->EnabledInterface)(*InputHandle_p); */

    return(Err);

#undef CHECK_ERROR_AND_EXIT_IF_FAILED
}

/*******************************************************************************
Name        : HALVIN_Term
Description : Terminate Input HAL
Parameters  : Input handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALVIN_Term(const HALVIN_Handle_t InputHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALVIN_Properties_t * HALVIN_Data_p;

    /* Find structure */
    HALVIN_Data_p = (HALVIN_Properties_t *) InputHandle;

    if (HALVIN_Data_p->ValidityCheck != HALVIN_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Desactivate interface */
    if (HALVIN_Data_p->FunctionsTable_p)
    {
        (HALVIN_Data_p->FunctionsTable_p->DisableInterface)(InputHandle);
    }

    /* De-validate structure */
    HALVIN_Data_p->ValidityCheck = 0; /* not HALVIN_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    memory_deallocate(HALVIN_Data_p->CPUPartition_p, (void *) HALVIN_Data_p);

    return(ErrorCode);
}


/*******************************************************************************
Name        : HALVIN_SetSizeOfTheFrame
Description :
Parameters  : Input handle, FrameWidth, FrameHeight
Assumptions :

Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALVIN_SetSizeOfTheFrame(const HALVIN_Handle_t InputHandle, const U16 FrameWidth, const U16 FrameHeight)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALVIN_Properties_t * HALVIN_Data_p;
    U16 FrameHeightTop, FrameHeightBottom;


    /* Find structure */
    HALVIN_Data_p = (HALVIN_Properties_t *) InputHandle;

    if (HALVIN_Data_p->ValidityCheck != HALVIN_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check the Scan Type */
    switch (HALVIN_Data_p->VideoParams_p->ScanType)
    {
        case STGXOBJ_PROGRESSIVE_SCAN:
            FrameHeightTop = FrameHeight;
            FrameHeightBottom = 0;
            break;
        case STGXOBJ_INTERLACED_SCAN:
            FrameHeightTop = FrameHeight; /* /2; */
            FrameHeightBottom = FrameHeight; /* /2; */
            break;
        default:
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    /* Check Input Window to match to new size of the Frame */
    if ( (HALVIN_Data_p->InputWindow.InputWinX + HALVIN_Data_p->InputWindow.InputWinWidth) > FrameWidth )
    {
        HALVIN_Data_p->InputWindow.InputWinWidth = FrameWidth - HALVIN_Data_p->InputWindow.InputWinX;
    }

    if ( (HALVIN_Data_p->InputWindow.InputWinY + HALVIN_Data_p->InputWindow.InputWinHeight) > FrameHeight )
    {
        HALVIN_Data_p->InputWindow.InputWinHeight = FrameHeight - HALVIN_Data_p->InputWindow.InputWinY;
    }

    ErrorCode = (HALVIN_Data_p->FunctionsTable_p->SetSizeOfTheFrame)(InputHandle, FrameWidth, FrameHeightTop, FrameHeightBottom);
    return(ErrorCode);
}

/*******************************************************************************
Name        : HALVIN_SetSyncType
Description :
Parameters  : Input handle, SyncType
Assumptions :

Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALVIN_SetSyncType(const HALVIN_Handle_t InputHandle, const STVIN_SyncType_t SyncType)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALVIN_Properties_t * HALVIN_Data_p;

    /* Find structure */
    HALVIN_Data_p = (HALVIN_Properties_t *) InputHandle;

    if (HALVIN_Data_p->ValidityCheck != HALVIN_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    ErrorCode = (HALVIN_Data_p->FunctionsTable_p->SetSyncType)(InputHandle, SyncType);

    if (ErrorCode == ST_NO_ERROR)
    {
        HALVIN_Data_p->VideoParams_p->SyncType = SyncType;
        if (SyncType == STVIN_SYNC_TYPE_EXTERNAL)
        {
            /* Size of the Frame */
            HALVIN_SetSizeOfTheFrame(InputHandle,
                                     HALVIN_Data_p->VideoParams_p->FrameWidth,
                                     HALVIN_Data_p->VideoParams_p->FrameHeight);

            (HALVIN_Data_p->FunctionsTable_p->SetBlankingOffset)(
                InputHandle,
                HALVIN_Data_p->VideoParams_p->VerticalBlankingOffset,
                HALVIN_Data_p->VideoParams_p->HorizontalBlankingOffset);
        }
        else if (SyncType == STVIN_SYNC_TYPE_CCIR)
        {
            /* Size of the Frame */
            HALVIN_SetSizeOfTheFrame(InputHandle,
                                     HALVIN_Data_p->VideoParams_p->FrameWidth,
                                     HALVIN_Data_p->VideoParams_p->FrameHeight);

            (HALVIN_Data_p->FunctionsTable_p->SetBlankingOffset)(InputHandle, 0, 0);
        }
    }

    return(ErrorCode);

}

/*******************************************************************************
Name        : HALVIN_InputPath
Description :
Parameters  : Input handle, SyncType
Assumptions :

Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALVIN_SetInputPath(const HALVIN_Handle_t InputHandle, const STVIN_InputPath_t InputPath)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALVIN_Properties_t * HALVIN_Data_p;

    /* Find structure */
    HALVIN_Data_p = (HALVIN_Properties_t *) InputHandle;

    if (HALVIN_Data_p->ValidityCheck != HALVIN_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    ErrorCode = (HALVIN_Data_p->FunctionsTable_p->SetInputPath)(InputHandle, InputPath);

    if (ErrorCode == ST_NO_ERROR)
    {
        HALVIN_Data_p->VideoParams_p->InputPath = InputPath;
    }

    return(ErrorCode);

} /* EOF HALVIN_InputPath */

/*******************************************************************************
Name        : HALVIN_SetBlankingOffset
Description :
Parameters  : Input handle, SyncType
Assumptions :

Limitations :
Returns     :
*******************************************************************************/
void HALVIN_SetBlankingOffset(const HALVIN_Handle_t InputHandle,  const U16 Vertical, const U16 Horizontal)
{
    HALVIN_Properties_t * HALVIN_Data_p;

    /* Find structure */
    HALVIN_Data_p = (HALVIN_Properties_t *) InputHandle;

    if (HALVIN_Data_p->ValidityCheck != HALVIN_VALID_HANDLE)
    {
        return;
    }

    HALVIN_Data_p->VideoParams_p->HorizontalBlankingOffset = Horizontal;
    HALVIN_Data_p->VideoParams_p->VerticalBlankingOffset = Vertical;

    if (HALVIN_Data_p->VideoParams_p->SyncType == STVIN_SYNC_TYPE_EXTERNAL)
    {
        /* Size of the Frame */
        HALVIN_SetSizeOfTheFrame(InputHandle,
                                 HALVIN_Data_p->VideoParams_p->FrameWidth,
                                 HALVIN_Data_p->VideoParams_p->FrameHeight);
        (HALVIN_Data_p->FunctionsTable_p->SetBlankingOffset)(InputHandle, Vertical, Horizontal);
    }
    else if (HALVIN_Data_p->VideoParams_p->SyncType == STVIN_SYNC_TYPE_CCIR)
    {
        /* Size of the Frame */
        HALVIN_SetSizeOfTheFrame(InputHandle,
                                 HALVIN_Data_p->VideoParams_p->FrameWidth,
                                 HALVIN_Data_p->VideoParams_p->FrameHeight);
        (HALVIN_Data_p->FunctionsTable_p->SetBlankingOffset)(InputHandle, 0, 0);
    }
}

/*******************************************************************************
Name        : HALVIN_CheckInputParam
Description :
Parameters  : DeviceType, InputType, SyncType
Assumptions : all params are corrects!!

Limitations :
Returns     : STVIN_DefInputMode_t
*******************************************************************************/
STVIN_DefInputMode_t HALVIN_CheckInputParam(STVIN_DeviceType_t DeviceType, STVIN_InputType_t InputType, STVIN_SyncType_t SyncType)
{
    return CheckInputParam[DeviceType][InputType][SyncType];
}



/*******************************************************************************
Name        : HALVIN_SetIOWindow
Description :
Parameters  : Input handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALVIN_SetIOWindow(const HALVIN_Handle_t InputHandle, S32 InputWinX, S32 InputWinY, U32 InputWinWidth, U32 InputWinHeight, U32 OutputWinWidth, U32 OutputWinHeight)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALVIN_Properties_t * HALVIN_Data_p;

    /* Find structure */
    HALVIN_Data_p = (HALVIN_Properties_t *) InputHandle;

    if (HALVIN_Data_p->ValidityCheck != HALVIN_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ( ( (InputWinX + InputWinWidth) > HALVIN_Data_p->VideoParams_p->FrameWidth) ||
         ( (InputWinY + InputWinHeight) > HALVIN_Data_p->VideoParams_p->FrameHeight) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    HALVIN_Data_p->InputWindow.InputWinX = InputWinX;
    HALVIN_Data_p->InputWindow.InputWinY = InputWinY;
    HALVIN_Data_p->InputWindow.InputWinWidth = InputWinWidth;
    HALVIN_Data_p->InputWindow.InputWinHeight = InputWinHeight;
    HALVIN_Data_p->InputWindow.OutputWinWidth = OutputWinWidth;
    HALVIN_Data_p->InputWindow.OutputWinHeight = OutputWinHeight;

    return(ErrorCode);
}

/* Private functions -------------------------------------------------------- */


/*******************************************************************************
Name        : GetInputNumber
Description : Get input number
Parameters  : Device type, registers base address, pointer to return input number
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if can't get number
*******************************************************************************/
static ST_ErrorCode_t GetInputNumber(const STVIN_DeviceType_t DeviceType, const void * RegistersBaseAddress_p, STVIN_InputMode_t InputMode, U8 * const InputNumber_p, STVIN_DefInputMode_t * const DefInputMode_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    *DefInputMode_p = STVIN_INPUT_INVALID;

    /* Input number returned is as follows:
        Omega1: always 0
        Omega2:
          * 7015: - Digital input SD 1 : 0
                  - Digital input SD 2 : 1
                  - Digital input HD 1 : 2

          * 7020: - Digital input SD 1 : 0
                  - Digital input SD 2 : 1
                  - Digital input HD 1 : 2

          * ST40GX1: - Digital input SD 1 : 0
                     - Digital input SD 2 : 1
    */

    if ((RegistersBaseAddress_p == NULL) &&
        (DeviceType != STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (DeviceType)
    {
        case STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT :
        case STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT :
            /* Base addresses 0x000, 0x100, 0x200 correspond to input number 0, 1, 2 */
            switch ((((U32) RegistersBaseAddress_p) & 0xFFF))
            {
                case 0x000:
                    *InputNumber_p = 0;
                    *DefInputMode_p = STVIN_INPUT_MODE_SD;
                    break;

                case 0x100 :
                    *InputNumber_p = 1;
                    *DefInputMode_p = STVIN_INPUT_MODE_SD;
                    break;

                case 0x200 :
                    *InputNumber_p = 2;
                    *DefInputMode_p = STVIN_INPUT_MODE_HD;
                    break;

                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;
        case STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT :
            /* Base addresses 0x000, 0x100 correspond to input number 0, 1 */
            switch ((((U32) RegistersBaseAddress_p) & 0xFFF))
            {
                case 0x000:
                    *InputNumber_p = 0;
                    *DefInputMode_p = STVIN_INPUT_MODE_SD;
                    break;

                case 0x100 :
                    *InputNumber_p = 1;
                    *DefInputMode_p = STVIN_INPUT_MODE_SD;
                    break;

                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;
        case STVIN_DEVICE_TYPE_ST5528_DVP_INPUT :
        case STVIN_DEVICE_TYPE_ST7710_DVP_INPUT :
        case STVIN_DEVICE_TYPE_ST7100_DVP_INPUT :
        case STVIN_DEVICE_TYPE_ST7109_DVP_INPUT :
            /* Consider the only one digital video port.    */
            *InputNumber_p = 0;
            switch (InputMode) {
                case STVIN_SD_NTSC_720_480_I_CCIR :
                case STVIN_SD_NTSC_640_480_I_CCIR :
                case STVIN_SD_PAL_720_576_I_CCIR :
                case STVIN_SD_PAL_768_576_I_CCIR :
                    *DefInputMode_p = STVIN_INPUT_MODE_SD;
                    break;

                case STVIN_HD_YCbCr_1280_720_P_CCIR :
                case STVIN_HD_YCbCr_720_480_P_CCIR :
                case STVIN_HD_RGB_1024_768_P_EXT :
                case STVIN_HD_RGB_800_600_P_EXT :
                case STVIN_HD_RGB_640_480_P_EXT :
                case STVIN_HD_YCbCr_1920_1080_I_CCIR :
                default :
                    *DefInputMode_p = STVIN_INPUT_MODE_HD;
                    break;
            }
            break;
        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    return(ErrorCode);
} /* End of GetInputNumber() function */


/* End of halv_vin.c */


