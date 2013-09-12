/*******************************************************************************

File name   : stvmix_wrap_disp.c

Description : video mixer module source file : wrapper for STDISP

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
20 11 2003                                                           TM
18 02 2005                                                           NM
07 03 2005                                                           SBEBA
******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>

#include "sttbx.h"

#include "stvmix.h"
#include "stvtg.h"
#include "stos.h"

#include "stvmix_wrap_disp.h"


#ifdef ST_5100
#include "sti5100.h"
#endif

#ifdef ST_5301
#include "sti5301.h"
#endif

#ifdef ST_5525
#include "sti5525.h"
#endif


/* Private Constants -------------------------------------------------------- */

#define STVMIX_MAX_DEVICE  2     /* Max number of Init() allowed */
#define STVMIX_MAX_OPEN    2     /* Max number of Open() allowed per Init() */
#define STVMIX_MAX_UNIT    (STVMIX_MAX_OPEN * STVMIX_MAX_DEVICE)

#define INVALID_DEVICE_INDEX (-1)

/* Private Variables (static)------------------------------------------------ */

static stvmix_Device_t DeviceArray[STVMIX_MAX_DEVICE];
static stvmix_Unit_t UnitArray[STVMIX_MAX_UNIT];
static semaphore_t * InstancesAccessControl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;

/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
     extern U32  DynamicDataSize ;
    #define AddDynamicDataSize(x)       DynamicDataSize += (x)
#else
    #define AddDynamicDataSize(x)
#endif

/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Passing a (STVMIX_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((stvmix_Unit_t *) (Handle)) >= UnitArray) &&                    \
                               (((stvmix_Unit_t *) (Handle)) < (UnitArray + STVMIX_MAX_UNIT)) &&  \
                               (((stvmix_Unit_t *) (Handle))->UnitValidity == STVMIX_VALID_UNIT))

/* Private Function prototypes ---------------------------------------------- */
 static void GetDisplayLatencyAndFrameBufferHoldTime(stvmix_Device_t* Device_p, U8* DisplayLatency_p, U8* FrameBufferHoldTime_p);


/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{
    static BOOL InstancesAccessControlInitialized = FALSE;

    STOS_TaskLock();
    if (!InstancesAccessControlInitialized)
    {
        InstancesAccessControlInitialized = TRUE;
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p = STOS_SemaphoreCreateFifo(NULL,1);
    }
    STOS_TaskUnlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InstancesAccessControl_p);
} /* End of EnterCriticalSection() function */


/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreSignal(InstancesAccessControl_p);
} /* End of LeaveCriticalSection() function */



/*******************************************************************************
Name        : ConvertRGBToYCbCr
Description : Convert RGB to Ycbcr4:4:4 Unsigned ITU601
Parameters  :
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void ConvertRGBToYCbCr(STGXOBJ_ColorRGB_t* RGB_p, STGXOBJ_ColorUnsignedYCbCr_t* YCbCr_p)
{
    U8 R, G, B;

    R = RGB_p->R;
    G = RGB_p->G;
    B = RGB_p->B;

    YCbCr_p->Y  = ( 66*R + 129*G +  25*B)/256 + 16;
    YCbCr_p->Cb = (112*B -  38*R -  74*G)/256 + 128;
    YCbCr_p->Cr = (112*R -  94*G -  18*B)/256 + 128;
}

/*******************************************************************************
Name        : IsLayerStillConnected
Description : Check if layer to connect is already connected
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL IsLayerStillConnected(const ST_DeviceName_t WantedName,
                                  const STVMIX_LayerDisplayParams_t * const LayerDisplayParams[],
                                  const U16 NbLayerParams)
{
    int i = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(LayerDisplayParams[i]->DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            return(TRUE);
        }
        i++;
    } while (i < NbLayerParams);

    return(FALSE);
}

/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 WantedIndex = INVALID_DEVICE_INDEX, Index = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(DeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STVMIX_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */


/*******************************************************************************
Name        : GetHandleOfLayerName
Description : Get the handle of an already open layer in device
Parameters  : the name to look for in the given device
Assumptions : Device_p & WantedName not Null. Layer.NbConnect initialised (Init func)
Returns     : NULL if no handle found, or handle
*******************************************************************************/
static STLAYER_Handle_t GetHandleOfLayerName(const stvmix_Device_t* Device_p, const ST_DeviceName_t WantedName)
{
    U16    ConnectNb;

    ConnectNb = Device_p->Layers.NbConnect;

    /* Until no layer found */
    while(ConnectNb-- != 0)
    {
        if(strcmp(Device_p->Layers.ConnectArray_p[ConnectNb].DeviceName, WantedName) == 0)
        {
            /* Return handle */
            return(Device_p->Layers.ConnectArray_p[ConnectNb].Handle);
        }
    }
    return(0);
}

/*******************************************************************************
Name        : CheckLayerOnOtherDevice
Description : Check if layer already connected on other devices. No pb if already connected to the current device
Parameters  : Layer Name
Assumptions : DeviceName not NULL, Layers.NbConnect initialised (init)
Returns     : ST_NO_ERROR, STVMIX_ERROR_LAYER_ALREADY_CONNECTED
*******************************************************************************/
static ST_ErrorCode_t CheckLayerOnOtherDevice(stvmix_Device_t* CurrentDevice_p, const ST_DeviceName_t DeviceName)
{

    U16  LayerCnt, DeviceCnt = 0;

    /* Access to global structure of device */
    while(DeviceCnt < STVMIX_MAX_DEVICE)
    {
        /* Check if device initialised */
        if((DeviceArray[DeviceCnt].DeviceName[0] != '\0') && (&DeviceArray[DeviceCnt] != CurrentDevice_p))
        {
            /* Layers connected to this device ? */
            if(DeviceArray[DeviceCnt].Layers.NbConnect != 0)
            {
                /* Check each connected layers */
                for(LayerCnt=0; LayerCnt<DeviceArray[DeviceCnt].Layers.NbConnect ;LayerCnt++)
                {
                    /* Connect layer matches with devicename ? */
                    if(strcmp(DeviceArray[DeviceCnt].Layers.ConnectArray_p[LayerCnt].DeviceName, DeviceName) == 0)
                        /* Layer is found */
                        return(STVMIX_ERROR_LAYER_ALREADY_CONNECTED);
                }
            }
        }
        DeviceCnt++;
    }
    /* No layer found with same name */
    return(ST_NO_ERROR);
}



/*******************************************************************************
Name        : Init
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(stvmix_Device_t * const Device_p, const STVMIX_InitParams_t * const InitParams_p,const ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t                  Err = ST_NO_ERROR;
    int                             i;
    STGXOBJ_BitmapAllocParams_t     BitmapParams1, BitmapParams2;
    STAVMEM_AllocBlockParams_t      AllocParams;
    STGXOBJ_Bitmap_t                Bitmap;
    STAVMEM_MemoryRange_t           RangeArea[2];
    U8                              NbForbiddenRange;
    STDISP_InitParams_t             InitParams;
    STGXOBJ_Color_t                 BackgroundColor;

    /* Check Init params */
    if (InitParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set default Black Background color : Store RGB888 Value  */
    Device_p->BackgroundColor.Type = STGXOBJ_COLOR_TYPE_RGB888;
    Device_p->BackgroundColor.Value.RGB888.R  = 0;
    Device_p->BackgroundColor.Value.RGB888.G  = 0;
    Device_p->BackgroundColor.Value.RGB888.B  = 0;
    Device_p->IsBackgroundEnable              = FALSE;

    /* Convert color to YcbCr888 4:4:4 ITU691 unsigned for STDISP configuration */
    ConvertRGBToYCbCr(&(Device_p->BackgroundColor.Value.RGB888),&(Device_p->DefaultYCbCr));

    /* Record some info in Device structure */
    Device_p->ScreenParams.AspectRatio  = STGXOBJ_ASPECT_RATIO_4TO3;
    Device_p->ScreenParams.ScanType     = STGXOBJ_INTERLACED_SCAN;
    Device_p->ScreenParams.FrameRate    = 50000;                        /*         ---                      */
    Device_p->ScreenParams.Width        = 720;                          /*            | __ Default is PAL   */
#ifdef SERVICE_DIRECTV
    Device_p->ScreenParams.Height       = 480;
#else
    Device_p->ScreenParams.Height       = 576;                          /*            |                     */
#endif
    Device_p->ScreenParams.XStart       = 132;                          /*            |                     */
    Device_p->ScreenParams.YStart       = 23;                           /*         ---                      */
    Device_p->XOffset                   = 0;
    Device_p->YOffset                   = 0;
    Device_p->MaxLayer                  = InitParams_p->MaxLayer;
    Device_p->CPUPartition_p            = InitParams_p->CPUPartition_p;
    Device_p->Layers.NbConnect          = 0;
    Device_p->AVMEM_Partition           = InitParams_p->AVMEM_Partition2;
    Device_p->DeviceType                = InitParams_p->DeviceType;

    strcpy(Device_p->VTGName, InitParams_p->VTGName);
    STAVMEM_GetSharedMemoryVirtualMapping(&Device_p->VirtualMapping);

#if defined(ST_5100) || defined(ST_5301)  || defined(ST_5525)

        if (( InitParams_p->DeviceType != STVMIX_COMPOSITOR ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_422 ))
            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif

#if defined(ST_5105) && !defined(SELECT_DEVICE_STB5118)
        if (( InitParams_p->DeviceType != STVMIX_COMPOSITOR ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_422 ))

            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif


#if defined(ST_5105) && defined(SELECT_DEVICE_STB5118)
        if (( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))
            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif

#if defined(ST_5107) && !defined(SELECT_DEVICE_STB5118)
        if (( InitParams_p->DeviceType != STVMIX_COMPOSITOR ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))

            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif

#if defined(ST_5107) && defined(SELECT_DEVICE_STB5118)
        if (( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))
            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif

#if defined(ST_5162) && !defined(SELECT_DEVICE_STB5118)
        if (( InitParams_p->DeviceType != STVMIX_COMPOSITOR ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))

            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif

#if defined(ST_5162) && defined(SELECT_DEVICE_STB5118)
        if (( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))
            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif


#if defined(ST_5188)
        if (( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM ) &&
            ( InitParams_p->DeviceType != STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))
            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
#endif



    /* Get Info about bitmaps to allocate */
#ifdef ST_5100
        if(InitParams_p->RegisterInfo.GdmaBaseAddress_p ==  ((void *)ST5100_GDMA1_BASE_ADDR)) /* GDMA1 (Main) */
        {
            /* Get Info about output bitmap color type */
         if( ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_SDRAM_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))
                 {
                Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
            }
            else
            {
                Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
            }
        }
        if(InitParams_p->RegisterInfo.GdmaBaseAddress_p ==  ((void *)ST5100_GDMA2_BASE_ADDR)) /* GDMA2 (Aux) */
        {
            Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
        }
#endif

#ifdef ST_5301
        if(InitParams_p->RegisterInfo.GdmaBaseAddress_p ==  ((void *)ST5301_GDMA1_BASE_ADDR)) /* GDMA1 (Main) */
        {
            /* Get Info about output bitmap color type */
            if( ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_SDRAM_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))
           {
                Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
            }
            else
            {
                Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
            }
        }
        if(InitParams_p->RegisterInfo.GdmaBaseAddress_p ==  ((void *)ST5301_GDMA2_BASE_ADDR)) /* GDMA2 (Aux) */
        {
            Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
        }
#endif

#ifdef ST_5525
        if(InitParams_p->RegisterInfo.GdmaBaseAddress_p ==  ((void *)ST5525_GDMA1_BASE_ADDR)) /* GDMA1 (Main) */
        {
            /* Get Info about output bitmap color type */
            if( ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_SDRAM_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) ||
                ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))
           {
                Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
            }
            else
            {
                Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
            }
        }
        if(InitParams_p->RegisterInfo.GdmaBaseAddress_p ==  ((void *)ST5525_GDMA2_BASE_ADDR)) /* GDMA2 (Aux) */
        {
            Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
        }
#endif



#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107) || defined(ST_5162)
        /* Get Info about output bitmap color type */
        /* GDMA1 (Main) */
        if( ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_422 ) ||
            ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_422 ) ||
            ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_SDRAM_422 ) ||
            ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_COMBINED_422 ) ||
            ( InitParams_p->DeviceType  == STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 ))

        {
            Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
        }
        else
        {
            Bitmap.ColorType            = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
        }
#endif

    Bitmap.BitmapType           = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    Bitmap.PreMultipliedColor   = FALSE;
    Bitmap.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
    Bitmap.AspectRatio          = STGXOBJ_ASPECT_RATIO_4TO3;
    Bitmap.Width                = Device_p->ScreenParams.Width;
    Bitmap.Height               = Device_p->ScreenParams.Height;
    STGXOBJ_GetBitmapAllocParams(&Bitmap, STGXOBJ_GAMMA_BLITTER,&BitmapParams1, &BitmapParams2);
    Bitmap.Pitch                = BitmapParams1.Pitch;
    Bitmap.Offset               = BitmapParams1.Offset;
    Bitmap.Size1                = BitmapParams1.AllocBlockParams.Size;

    /* Forbidden range : All Virtual memory range but Virtual window
     *  VirtualWindowOffset may be 0
     *  Virtual Size is always > 0
     *  Assumption : Physical base Address from device = 0 !!!! */
    NbForbiddenRange = 1;
    if (Device_p->VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) Device_p->VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) Device_p->VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((Device_p->VirtualMapping.VirtualWindowOffset + Device_p->VirtualMapping.VirtualWindowSize) !=
         Device_p->VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowOffset) +
                                             (U32)(Device_p->VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(Device_p->VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(Device_p->VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange = 2;
    }

    /* Output Bitmap data allocation */
    AllocParams.PartitionHandle          = InitParams_p->AVMEM_Partition2;
    AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p   = NULL;
    AllocParams.Size                     = BitmapParams1.AllocBlockParams.Size;
    AllocParams.Alignment                = BitmapParams1.AllocBlockParams.Alignment;

     if ( (InitParams_p->DeviceType == STVMIX_COMPOSITOR) ||
         (InitParams_p->DeviceType == STVMIX_COMPOSITOR_422) )
       {
        /* In frame mode, 2 bitmap buffer are needed to perform display and composition */
        Device_p->NbBitmapBuffer = 2;
    }
    else /* field acess */
    {
        /* In field mode, display and composition are performed in the same bitmap buffer*/
        Device_p->NbBitmapBuffer = 1;
    }

    for (i=0; i<Device_p->NbBitmapBuffer ; i++ )
    {
        Device_p->OutputBitmap[i] = Bitmap;

        Err = STAVMEM_AllocBlock (&AllocParams, &Device_p->OutputBitmapAVMEMHandle[i]);
        if ( Err != ST_NO_ERROR )
        {
            STTBX_Print (("Error allocating Output Bitmap %d Data1_p: %s\n",i,Err));
            return(Err);
        }
        Err = STAVMEM_GetBlockAddress (Device_p->OutputBitmapAVMEMHandle[i], (void **)&(Device_p->OutputBitmap[i].Data1_p));
        if ( Err != ST_NO_ERROR )
        {
            STTBX_Print (("Error getting block address Output Bitmap %d Data1_p : %s\n",i,Err));
            return(Err);
        }
    }

    /* Init Display */
    InitParams.CPUPartition_p                               = Device_p->CPUPartition_p;
    InitParams.RegisterInfo.CompoBaseAddress_p              = InitParams_p->RegisterInfo.CompoBaseAddress_p;
    InitParams.RegisterInfo.GdmaBaseAddress_p               = InitParams_p->RegisterInfo.GdmaBaseAddress_p;
    InitParams.RegisterInfo.VoutBaseAddress_p               = InitParams_p->RegisterInfo.VoutBaseAddress_p;
    InitParams.CPUBigEndian                                 = FALSE;
    switch ( InitParams_p->DeviceType )
    {
        case STVMIX_COMPOSITOR :
        case STVMIX_COMPOSITOR_422 :
#ifdef ST_5100
            InitParams.DeviceType                                   = STDISP_5100_COMPOSITOR;
#endif
#ifdef ST_5105
            InitParams.DeviceType                                   = STDISP_5105_COMPOSITOR;
#endif
#ifdef ST_5301
            InitParams.DeviceType                                   = STDISP_5301_COMPOSITOR;
#endif

#ifdef ST_5525
            InitParams.DeviceType                                   = STDISP_5525_COMPOSITOR;
#endif
#ifdef ST_5107
            InitParams.DeviceType                                   = STDISP_5107_COMPOSITOR;
#endif
#ifdef ST_5162
            InitParams.DeviceType                                   = STDISP_5162_COMPOSITOR;
#endif


            InitParams.MaxViewPorts                                 = STVMIX_FRAME_MODE_MAX_VIEWPORTS;
            InitParams.CompoNodeNumber                              = STVMIX_FRAME_MODE_NODE_NUMBER;
            break;

        case STVMIX_COMPOSITOR_FIELD :
        case STVMIX_COMPOSITOR_FIELD_422 :
            InitParams.DeviceType                                   = STDISP_COMPOSITOR_FIELD;
            InitParams.MaxViewPorts                                 = STVMIX_FIELD_MODE_MAX_VIEWPORTS;
            InitParams.CompoNodeNumber                              = STVMIX_FIELD_MODE_NODE_NUMBER;
            break;

        case STVMIX_COMPOSITOR_FIELD_SDRAM :
        case STVMIX_COMPOSITOR_FIELD_SDRAM_422 :
            InitParams.DeviceType                                   = STDISP_COMPOSITOR_FIELD_SDRAM;
            InitParams.MaxViewPorts                                 = STVMIX_FIELD_SDRAM_MODE_MAX_VIEWPORTS;
            InitParams.CompoNodeNumber                              = STVMIX_FIELD_SDRAM_MODE_NODE_NUMBER;
            break;

        case STVMIX_COMPOSITOR_FIELD_COMBINED :
        case STVMIX_COMPOSITOR_FIELD_COMBINED_422 :
            InitParams.DeviceType                                   = STDISP_COMPOSITOR_FIELD_COMBINED;
            InitParams.MaxViewPorts                                 = STVMIX_FIELD_MODE_MAX_VIEWPORTS;
            InitParams.CompoNodeNumber                              = STVMIX_FIELD_MODE_NODE_NUMBER;
            break;

        case STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM :
        case STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 :
            InitParams.DeviceType                                   = STDISP_COMPOSITOR_FIELD_COMBINED_SDRAM;
            InitParams.MaxViewPorts                                 = STVMIX_FIELD_SDRAM_MODE_MAX_VIEWPORTS;
            InitParams.CompoNodeNumber                              = STVMIX_FIELD_SDRAM_MODE_NODE_NUMBER;
            break;

        default:
        	return ST_ERROR_BAD_PARAMETER;
        	break;
    }

#ifdef ST_5100
    if(InitParams.RegisterInfo.GdmaBaseAddress_p == ((void *)ST5100_GDMA1_BASE_ADDR))
    {
        InitParams.CompoQueueType                           = STDISP_QUEUE_TYPE_COMPOSITION;
    }

    if(InitParams.RegisterInfo.GdmaBaseAddress_p == ((void *)ST5100_GDMA2_BASE_ADDR))
    {
        InitParams.CompoQueueType                           = STDISP_QUEUE_TYPE_APPLICATION;
    }
#endif
#ifdef ST_5301
    if(InitParams.RegisterInfo.GdmaBaseAddress_p == ((void *)ST5301_GDMA1_BASE_ADDR))
    {
        InitParams.CompoQueueType                           = STDISP_QUEUE_TYPE_COMPOSITION;
    }

    if(InitParams.RegisterInfo.GdmaBaseAddress_p == ((void *)ST5301_GDMA2_BASE_ADDR))
    {
        InitParams.CompoQueueType                           = STDISP_QUEUE_TYPE_APPLICATION;
    }
#endif

#ifdef ST_5525
    if(InitParams.RegisterInfo.GdmaBaseAddress_p == ((void *)ST5525_GDMA1_BASE_ADDR))
    {
        InitParams.CompoQueueType                           = STDISP_QUEUE_TYPE_COMPOSITION;
    }

    if(InitParams.RegisterInfo.GdmaBaseAddress_p == ((void *)ST5525_GDMA2_BASE_ADDR))
    {
        InitParams.CompoQueueType                           = STDISP_QUEUE_TYPE_APPLICATION;
    }
#endif


#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107) || defined(ST_5162)
    InitParams.CompoQueueType                           = STDISP_QUEUE_TYPE_COMPOSITION;
#endif

    strcpy(InitParams.EventHandlerName,InitParams_p->EvtHandlerName);
    InitParams.AVMEM_Partition                              = InitParams_p->AVMEM_Partition;
    InitParams.MaxHandles                                   = InitParams_p->MaxOpen;
    /*InitParams.ViewPortBuffer_p             =; */
    InitParams.ViewPortBufferUserAllocated                  = FALSE;
    BackgroundColor.Type                                    = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    BackgroundColor.Value.UnsignedYCbCr888_444              = Device_p->DefaultYCbCr;
    InitParams.BackgroundColor_p                            = &BackgroundColor;
    InitParams.ScanType                                     = Device_p->ScreenParams.ScanType;
    InitParams.ScreenWidth                                  = Device_p->ScreenParams.Width;
    InitParams.ScreenHeight                                 = Device_p->ScreenParams.Height;
    InitParams.XStart                                       = Device_p->ScreenParams.XStart;
    InitParams.YStart                                       = Device_p->ScreenParams.YStart;
    InitParams.VTGParams.VTGFrameRate                       = Device_p->ScreenParams.FrameRate ;
    strcpy(InitParams.VTGParams.VTGName, Device_p->VTGName);
    InitParams.OutputArray_p                                = InitParams_p->OutputArray_p ;
    InitParams.CompoNodeBufferUserAllocated                 = FALSE;
    /*InitParams.CompoNodeBuffer_p            = ; */
    InitParams.CompoCacheBitmap_p                           = InitParams_p->CacheBitmap_p;

    InitParams.GdmaNodeBufferUserAllocated                  = FALSE;
    InitParams.GdmaNodeNumber                               = STVMIX_GDMA_NODE_NUMBER;
    /*InitParams.GdmaNodeBuffer_p             =; */
       if ( InitParams_p->DeviceType == STVMIX_COMPOSITOR ||
         InitParams_p->DeviceType == STVMIX_COMPOSITOR_422 )
    {
        /* In frame mode, 2 bitmap buffer are needed to perform display and composition */
        InitParams.OutputBitmap1_p                              = &Device_p->OutputBitmap[0];
        InitParams.OutputBitmap2_p                              = &Device_p->OutputBitmap[1];
    }
    else /* field acess */
    {
        /* In field mode, display and composition are performed in the same bitmap buffer*/
        InitParams.OutputBitmap1_p                              = &Device_p->OutputBitmap[0];
    }

    Err = STDISP_Init(DeviceName,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_Init : %d\n",Err));

        if (Err == STDISP_ERROR_VOUT_UNKNOWN)
        {
            return(STVMIX_ERROR_VOUT_UNKNOWN);
        }
        else if (Err == STDISP_ERROR_VOUT_ALREADY_CONNECTED)
        {
            return(STVMIX_ERROR_VOUT_ALREADY_CONNECTED);
        }
        else /* As First implementation, all other Errors remain as it is */
        {
            return(Err);
        }
    }

    return(Err);
}

/*******************************************************************************
Name        : Open
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :

*******************************************************************************/
static ST_ErrorCode_t Open(stvmix_Unit_t * const Unit_p, const STVMIX_OpenParams_t * const OpenParams_p)
{
    ST_ErrorCode_t      Err = ST_NO_ERROR;
    STDISP_OpenParams_t OpenParams;           /* dummy OpenParams */

    UNUSED_PARAMETER(OpenParams_p);
    Err = STDISP_Open(Unit_p->Device_p->DeviceName,&OpenParams,&(Unit_p->Handle));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_Open : %d\n",Err));
        return(Err);
    }

    return(Err);
}

/*******************************************************************************
Name        : Close
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Close(stvmix_Unit_t * const Unit_p)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    Err = STDISP_Close(Unit_p->Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_Close : %d\n",Err));
        return(Err);
    }

    return(Err);
}

/*******************************************************************************
Name        : Term
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Term(stvmix_Device_t * const Device_p, const STVMIX_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t              Err = ST_NO_ERROR;
    STDISP_TermParams_t         TermParams;
    STAVMEM_FreeBlockParams_t   FreeParams;
    int                         i;

    /* Check Init params */
    if (TermParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    TermParams.ForceTerminate = TermParams_p->ForceTerminate;

    Err = STDISP_Term(Device_p->DeviceName, &TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_Term : %d\n",Err));
        return(Err);
    }

    /* deallocate Both output bitmaps */
    for (i=0; i<Device_p->NbBitmapBuffer; i++ )
    {
        FreeParams.PartitionHandle = Device_p->AVMEM_Partition;
        Err = STAVMEM_FreeBlock(&FreeParams,&(Device_p->OutputBitmapAVMEMHandle[i]));
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Data1_p OutputBitmap %d : %d\n",i,Err));
            /* return (Err); */
        }
    }

    return(Err);
}


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : STVMIX_GetRevision
Description : Give the Revision number of the STVMIX driver
Parameters  : None
Assumptions : None
Limitations :
Returns     : Revision number
*******************************************************************************/
ST_Revision_t STVMIX_GetRevision(void)
{
    static const char Revision[] = "STVMIX-REL_2.1.0";

    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
       Example: STVMIX-REL_1.2.3 */

    return((ST_Revision_t) Revision);
} /* End of STVMIX_GetRevision() function */
/*******************************************************************************
Name        : STVMIX_GetCapability
Description : Get the capability of a driver
Parameters  : Handle on a device, pointer to capabilities structure
Assumptions : None
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetCapability(const ST_DeviceName_t DeviceName, STVMIX_Capability_t* const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t    ErrCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                        /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')             /* Device Name should not be NULL */
        )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* Init done */
    if (!FirstInitDone)
    {
        ErrCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill structure with current info */
            Capability_p->ScreenOffsetHorizontalMin =  STVMIX_COMPO_MIN_HORIZONTAL_OFFSET;
            Capability_p->ScreenOffsetHorizontalMax =  STVMIX_COMPO_MAX_HORIZONTAL_OFFSET;
            Capability_p->ScreenOffsetVerticalMin   = -STVMIX_COMPO_VERTICAL_OFFSET ;
            Capability_p->ScreenOffsetVerticalMax   =  STVMIX_COMPO_VERTICAL_OFFSET;
        }
    }

    LeaveCriticalSection();

    return(ErrCode);
}

/*******************************************************************************
Name        : STVMIX_Enable
Description : Enable output of a mixer
Parameters  : Handle on a device
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, STVMIX_ERROR_LAYER_ACCESS,
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t STVMIX_Enable(const STVMIX_Handle_t Handle)
{
    ST_ErrorCode_t          Err     = ST_NO_ERROR;
    stvmix_Unit_t*          Unit_p  = (stvmix_Unit_t*) Handle ;


    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Err = STDISP_EnableDisplay(Unit_p->Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_EnableDisplay : %d\n",Err));
        return(Err);
    }

    return(Err);
}
#ifdef WA_GNBvd31390 /*WA for Chroma Luma delay bug on STi5100*/
/*******************************************************************************
Name        : STVMIX_CLDelayWA
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_CLDelayWA(STVMIX_Handle_t Handle,STVTG_TimingMode_t Mode)
{
    ST_ErrorCode_t          Err     = ST_NO_ERROR;
    stvmix_Unit_t*          Unit_p  = (stvmix_Unit_t*) Handle ;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    /*Call STDISP function : application can only access to STDISP through VMIX*/
    Err = STDISP_CLDelayWA(Unit_p->Handle, Mode );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_EnableDisplay : %d\n",Err));
        return(Err);
    }

    return(Err);
}
#endif /*WA_GNBvd31390*/

/*******************************************************************************
Name        : STVMIX_Disable
Description : Disable output of mixer
Parameters  : Handle on a device
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVMIX_Disable(const STVMIX_Handle_t Handle)
{
    ST_ErrorCode_t          Err     = ST_NO_ERROR;
    stvmix_Unit_t*          Unit_p  = (stvmix_Unit_t*) Handle ;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Err = STDISP_DisableDisplay(Unit_p->Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_DisableDisplay : %d\n",Err));
        return(Err);
    }

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_SetBackgroundColor
Description : Set the background color of a driver
Parameters  : Handle on a device, color structure and enable
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetBackgroundColor(const STVMIX_Handle_t Handle, \
                                         STGXOBJ_ColorRGB_t const RGB888, BOOL const Enable)
{
    ST_ErrorCode_t                  Err     = ST_NO_ERROR;
    stvmix_Unit_t*                  Unit_p  = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*                Device_p;
    STGXOBJ_ColorUnsignedYCbCr_t    YCbCr;


    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Set STDISP Background color */
    if (Enable == TRUE)
    {
        /* Convert color to YcbCr888 4:4:4 ITU691 unsigned */
        ConvertRGBToYCbCr((STGXOBJ_ColorRGB_t*)&RGB888,&YCbCr);
    }
    else
    {
        YCbCr = Device_p->DefaultYCbCr;
    }

    Err = STDISP_SetDisplayBackgroundColor(Unit_p->Handle,YCbCr);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_SetDisplayBackgroundColor : %d\n",Err));

        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(Err);
    }

    /* Record RGB Value + Enable */
    Device_p->BackgroundColor.Value.RGB888.R    = RGB888.R;
    Device_p->BackgroundColor.Value.RGB888.G    = RGB888.G;
    Device_p->BackgroundColor.Value.RGB888.B    = RGB888.B;
    Device_p->IsBackgroundEnable                = Enable;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_GetBackgroundColor
Description : Get the Background color of a driver
Parameters  : Handle on a device, pointer to color structure and enable
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetBackgroundColor(const STVMIX_Handle_t     Handle, \
                                         STGXOBJ_ColorRGB_t* const RGB888_p, BOOL* const Enable_p)
{
    ST_ErrorCode_t                  Err = ST_NO_ERROR;
    stvmix_Unit_t*                  Unit_p  = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*                Device_p;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;


    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Parameters */
    if ((RGB888_p == NULL) || (Enable_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    RGB888_p->R    = Device_p->BackgroundColor.Value.RGB888.R;
    RGB888_p->G    = Device_p->BackgroundColor.Value.RGB888.G;
    RGB888_p->B    = Device_p->BackgroundColor.Value.RGB888.B;
    *Enable_p      = Device_p->IsBackgroundEnable;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_SetScreenOffset
Description : Set the screen offset
Parameters  : Handle on a device, Horizontal & Vertical Offset
Assumptions : Parameters tested in STDISP
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetScreenOffset(const STVMIX_Handle_t Handle, const S8 Horizontal, const S8 Vertical)
{
    ST_ErrorCode_t          Err     = ST_NO_ERROR;
    stvmix_Unit_t*          Unit_p  = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*        Device_p;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ((Horizontal > STVMIX_COMPO_MAX_HORIZONTAL_OFFSET)  ||
        (Horizontal < STVMIX_COMPO_MIN_HORIZONTAL_OFFSET) ||
        (Vertical > STVMIX_COMPO_VERTICAL_OFFSET)      ||
        (Vertical < -STVMIX_COMPO_VERTICAL_OFFSET))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Err = STDISP_SetDisplayScreenOffset(Unit_p->Handle,Horizontal, Vertical);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_SetDisplayScreenOffset : %d\n",Err));
        return(Err);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Device structure */
    Device_p->XOffset = Horizontal;
    Device_p->YOffset = Vertical;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_GetScreenOffset
Description : Get the screen offset
Parameters  : Handle on a device, pointer to horizontal and vertical offset
Assumptions : Parameters tested in STDISP
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetScreenOffset(const STVMIX_Handle_t Handle, S8* const Horizontal_p, S8* const Vertical_p)
{
    ST_ErrorCode_t          Err     = ST_NO_ERROR;
    stvmix_Unit_t*          Unit_p  = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*        Device_p;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;


    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ((Horizontal_p == NULL) ||
        (Vertical_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Update Device structure */
    *Horizontal_p   = Device_p->XOffset;
    *Vertical_p     = Device_p->YOffset;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_SetScreenParams
Description : Set the screen parameters
Parameters  : Handle on a device, pointer to screen parameter structure
Assumptions : None
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetScreenParams(const STVMIX_Handle_t Handle, const STVMIX_ScreenParams_t* const ScreenParams_p)
{
    ST_ErrorCode_t                  Err             = ST_NO_ERROR;
    stvmix_Unit_t*                  Unit_p          = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*                Device_p;
    STDISP_ScreenParams_t           ScreenParams;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ScreenParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check parameters validity */
    if((ScreenParams_p->AspectRatio > STGXOBJ_ASPECT_RATIO_SQUARE) ||
       (ScreenParams_p->ScanType > STGXOBJ_INTERLACED_SCAN)||
       (ScreenParams_p->Height == 0) ||
       (ScreenParams_p->Width == 0))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    /* Set STDISP */
    ScreenParams.AspectRatio    = ScreenParams_p->AspectRatio;
    ScreenParams.ScanType       = ScreenParams_p->ScanType;
    ScreenParams.FrameRate      = ScreenParams_p->FrameRate;
    ScreenParams.Width          = ScreenParams_p->Width;
    ScreenParams.Height         = ScreenParams_p->Height;
    ScreenParams.XStart         = ScreenParams_p->XStart;
    ScreenParams.YStart         = ScreenParams_p->YStart;

    Err = STDISP_SetDisplayScreenParams(Unit_p->Handle,&ScreenParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_SetDisplayScreenParams : %d\n",Err));
        return(Err);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Set Device structure */
    Device_p->ScreenParams.AspectRatio    = ScreenParams_p->AspectRatio;
    Device_p->ScreenParams.ScanType       = ScreenParams_p->ScanType;
    Device_p->ScreenParams.FrameRate      = ScreenParams_p->FrameRate;
    Device_p->ScreenParams.Width          = ScreenParams_p->Width;
    Device_p->ScreenParams.Height         = ScreenParams_p->Height;
    Device_p->ScreenParams.XStart         = ScreenParams_p->XStart;
    Device_p->ScreenParams.YStart         = ScreenParams_p->YStart;

    /* Call update from mixer if layer connected */
    if (Device_p->Layers.NbConnect != 0)
    {
        STLAYER_OutputParams_t  UpdateParams;
        unsigned int i;

        memset(&UpdateParams, 0, sizeof(UpdateParams));
        UpdateParams.UpdateReason   = STLAYER_SCREEN_PARAMS_REASON | STLAYER_VTG_REASON;
        UpdateParams.AspectRatio    = Device_p->ScreenParams.AspectRatio;
        UpdateParams.ScanType       = Device_p->ScreenParams.ScanType;
        UpdateParams.FrameRate      = Device_p->ScreenParams.FrameRate;
        UpdateParams.Width          = Device_p->ScreenParams.Width;
        UpdateParams.Height         = Device_p->ScreenParams.Height;
        UpdateParams.XStart         = Device_p->ScreenParams.XStart;
        UpdateParams.YStart         = Device_p->ScreenParams.YStart;
        UpdateParams.DisplayHandle  = Unit_p->Handle;
        strcpy(UpdateParams.VTGName , Device_p->VTGName);

        /* Check UpdateFromMixer parameters */

        for (i=0; i<Device_p->Layers.NbConnect ;i++ )
        {
            ST_ErrorCode_t  RetErr;

            /* Update from Mixer */
            RetErr = STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[i].Handle,&UpdateParams);
            if (RetErr != ST_NO_ERROR)
            {
                /* First implementation without any UNDO for previous loop calls : TBD!!!*/

                /* Free access */
                STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STLAYER_UpdateFromMixer : %d\n",RetErr));
                if (RetErr == ST_ERROR_INVALID_HANDLE)
                {
                    /* Layer may have been destroyed */
                    return(STVMIX_ERROR_LAYER_ACCESS);
                }
                else
                {
                    return(STVMIX_ERROR_LAYER_UPDATE_PARAMETERS);
                }
            }
        }
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_GetScreenParams
Description : Get the screen paramameters
Parameters  : Handle on a device, pointer to screen parameters
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetScreenParams(const STVMIX_Handle_t Handle, STVMIX_ScreenParams_t* const ScreenParams_p)
{
    ST_ErrorCode_t                  Err             = ST_NO_ERROR;
    stvmix_Unit_t*                  Unit_p          = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*                Device_p;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;


    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ScreenParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    ScreenParams_p->AspectRatio = Device_p->ScreenParams.AspectRatio;
    ScreenParams_p->ScanType    = Device_p->ScreenParams.ScanType;
    ScreenParams_p->FrameRate   = Device_p->ScreenParams.FrameRate;
    ScreenParams_p->Width       = Device_p->ScreenParams.Width;
    ScreenParams_p->Height      = Device_p->ScreenParams.Height;
    ScreenParams_p->XStart      = Device_p->ScreenParams.XStart;
    ScreenParams_p->YStart      = Device_p->ScreenParams.YStart;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_SetTimeBase
Description : Attach a VTG driver to a mixer, then to the layers connected
Parameters  : Handle on a device, Name of a VTG driver
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STVMIX_ERROR_LAYER_ACCESS,
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetTimeBase(const STVMIX_Handle_t Handle, const ST_DeviceName_t VTGDriver)
{
    ST_ErrorCode_t                  Err         = ST_NO_ERROR;
    stvmix_Unit_t*                  Unit_p      = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*                Device_p;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;



    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Params */
    if(VTGDriver == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if(strlen(VTGDriver) > ST_MAX_DEVICE_NAME - 1)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if(VTGDriver[0] == '\0')
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    Err = STDISP_SetDisplayTimeBase(Unit_p->Handle,VTGDriver);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_SetDisplayTimeBase : %d\n",Err));
        return(Err);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Set Device structure */
    strcpy(Device_p->VTGName, VTGDriver);

    /* Call update from mixer if layer connected */
    if (Device_p->Layers.NbConnect != 0)
    {
        STLAYER_OutputParams_t  UpdateParams;
        unsigned int i;

        memset(&UpdateParams, 0, sizeof(UpdateParams));
        UpdateParams.UpdateReason   = STLAYER_VTG_REASON;
        strcpy(UpdateParams.VTGName , Device_p->VTGName);
        UpdateParams.DisplayHandle  = Unit_p->Handle;

        /* Check UpdateFromMixer parameters */

        for (i=0; i<Device_p->Layers.NbConnect ;i++ )
        {
            ST_ErrorCode_t  RetErr;

            /* Update from Mixer */
            RetErr = STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[i].Handle,&UpdateParams);
            if (RetErr != ST_NO_ERROR)
            {
                /* First implementation without any UNDO for previous loop calls : TBD!!!*/

                /* Free access */
                STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STLAYER_UpdateFromMixer : %d\n",RetErr));
                if (RetErr == ST_ERROR_INVALID_HANDLE)
                {
                    /* Layer may have been destroyed */
                    return(STVMIX_ERROR_LAYER_ACCESS);
                }
                else
                {
                    return(STVMIX_ERROR_LAYER_UPDATE_PARAMETERS);
                }
            }
        }
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_GetTimeBase
Description : Give back VTG driver name attached to the mixer device
Parameters  : Handle on a device, Pointer to allocated device name
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetTimeBase(const STVMIX_Handle_t Handle, ST_DeviceName_t* const VTGDriver_p)
{
    ST_ErrorCode_t                  Err = ST_NO_ERROR;
    stvmix_Unit_t*                  Unit_p      = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*                Device_p;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (VTGDriver_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    strcpy(*VTGDriver_p,Device_p->VTGName);

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*
--------------------------------------------------------------------------------
Initialise STVMIX_Init driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVMIX_Init(const ST_DeviceName_t DeviceName, const STVMIX_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (InitParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((InitParams_p->MaxOpen > STVMIX_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (InitParams_p->MaxOpen == 0) ||
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) ||    /* Device name length should be respected */
        (strcmp(&DeviceName[0],"\0") ==0 )                /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index = 0; Index < STVMIX_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index = 0; Index < STVMIX_MAX_UNIT; Index++)
        {
            UnitArray[Index].UnitValidity = 0;
        }
        /* Process this only once */
        FirstInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        Err = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available */
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STVMIX_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STVMIX_MAX_DEVICE)
        {
            /* All devices initialised */
            Err = ST_ERROR_NO_MEMORY;
        }
        else
        {
            stvmix_LayerConnect_t*  LayerConnectedArray_p;

            /* Allocated for layer order */
            LayerConnectedArray_p = (stvmix_LayerConnect_t*) memory_allocate (InitParams_p->CPUPartition_p, \
                                                                              InitParams_p->MaxLayer * \
                                                                              sizeof(stvmix_LayerConnect_t));
            AddDynamicDataSize(InitParams_p->MaxLayer * sizeof(stvmix_LayerConnect_t));

            if (LayerConnectedArray_p == NULL)
            {
                memory_deallocate(InitParams_p->CPUPartition_p, LayerConnectedArray_p);
                Err = ST_ERROR_NO_MEMORY;  /* Not enough memory to initialize mixer */
            }

            if (Err == ST_NO_ERROR)
            {
                /* Save allocated structure */
                DeviceArray[Index].Layers.ConnectArray_p  = LayerConnectedArray_p;

                /* Semaphore mixer device access creation */
                DeviceArray[Index].CtrlAccess_p = STOS_SemaphoreCreateFifo(NULL,1);

                /* API specific initialisations */
                Err = Init(&DeviceArray[Index], InitParams_p, DeviceName);

                if (Err == ST_NO_ERROR)
                {
                    /* Device available and successfully initialised: register device name */
                    strcpy(DeviceArray[Index].DeviceName, DeviceName);
                    DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                    DeviceArray[Index].MaxHandles = InitParams_p->MaxOpen;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised", DeviceArray[Index].DeviceName));
                }
                else   /* Free allocated memory. Not NULL checked upper */
                {
                    memory_deallocate(InitParams_p->CPUPartition_p, LayerConnectedArray_p);
                }
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();

    return(Err);
} /* End of STVMIX_Init() function */


/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVMIX_Open(const ST_DeviceName_t DeviceName, const STVMIX_OpenParams_t * const OpenParams_p, STVMIX_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        (strcmp(&DeviceName[0],"\0") ==0 )
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            Err = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Look for a free unit and check all opened units to check if MaxHandles is not reached */
            UnitIndex = STVMIX_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STVMIX_MAX_UNIT; Index++)
            {
                if ((UnitArray[Index].UnitValidity == STVMIX_VALID_UNIT) && (UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (UnitArray[Index].UnitValidity != STVMIX_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxHandles) || (UnitIndex >= STVMIX_MAX_UNIT))
            {
                /* None of the units is free or MaxHandles reached */
                Err = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STVMIX_Handle_t) &UnitArray[UnitIndex];
                UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                /* API specific actions after opening */
                Err = Open(&UnitArray[UnitIndex], OpenParams_p);

                if (Err == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    UnitArray[UnitIndex].UnitValidity = STVMIX_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End fo STVMIX_Open() function */


/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVMIX_Close(STVMIX_Handle_t Handle)
{
    stvmix_Unit_t *Unit_p;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    EnterCriticalSection();

    if (!(FirstInitDone))
    {
        Err = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidHandle(Handle))
        {
            Err = ST_ERROR_INVALID_HANDLE;
        }
        else
        {
            Unit_p = (stvmix_Unit_t *) Handle;

            /* API specific actions before closing */
            Err = Close(Unit_p);

            /* Close only if no errors */
            if (Err == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
        } /* End handle valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End of STVMIX_Close() function */


/*
--------------------------------------------------------------------------------
Terminate xxxxx driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVMIX_Term(const ST_DeviceName_t DeviceName, const STVMIX_TermParams_t *const TermParams_p)
{
    stvmix_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        (strcmp(&DeviceName[0],"\0") ==0 )            /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            Err = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if there is still 'open' on this device */
/*            Found = FALSE;*/
            UnitIndex = 0;
            Unit_p = UnitArray;
            while ((UnitIndex < STVMIX_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVMIX_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = UnitArray;
                    while (UnitIndex < STVMIX_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVMIX_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            Err = Close(Unit_p);
                            if (Err == ST_NO_ERROR)
                            {
                                /* Un-register opened handle */
                                Unit_p->UnitValidity = 0;

                                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                            }
                            else
                            {
                                /* Problem: this should not happen
                                Fail to terminate ? No because of force */
                                Err = ST_NO_ERROR;
                            }
                        }
                        Unit_p++;
                        UnitIndex++;
                    }
                } /* End ForceTerminate: closed all opened handles */
                else
                {
                    /* Can't term if there are handles still opened, and ForceTerminate not set */
                    Err = ST_ERROR_OPEN_HANDLE;
                }
            } /* End found handle not closed */

            /* Terminate if OK */
            if (Err == ST_NO_ERROR)
            {
                /* API specific terminations */
                Err = Term(&DeviceArray[DeviceIndex], TermParams_p);

                if (Err == ST_NO_ERROR)
                {
                    /* Free allocated memory */
                    memory_deallocate(DeviceArray[DeviceIndex].CPUPartition_p, \
                                      DeviceArray[DeviceIndex].Layers.ConnectArray_p);

                    /* Get semaphore before deleting it */
                    STOS_SemaphoreWait(DeviceArray[DeviceIndex].CtrlAccess_p);

                    /* Free device semaphore */
                    STOS_SemaphoreDelete(NULL,DeviceArray[DeviceIndex].CtrlAccess_p);

                    /* Reset value */
                    DeviceArray[DeviceIndex].Layers.ConnectArray_p = NULL;

                    /* Device found: desallocate memory, free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
                }
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End of STVMIX_Term() function */



/*******************************************************************************
Name        : STVMIX_ConnectLayers
Description : Connect layers to a mixer

              Previous Configuration : Device_p->Layers.ConnectArray_p[]
                                     _____ _____ _____ _____
                                    |     |     |     |     |
                                    | L1  | L2  | L3  | L4  |   size = Device_p->Layers.NbConnect
                                    |_____|_____|_____|_____|
                                      |     |      |     |
                                      |     |      |     |_____DeviceName + Handle
                                      |     |      |_____DeviceName + Handle
                                      |     |_____DeviceName + Handle
                                      |_____DeviceName + Handle

              New Configuration :  LayerDisplayParams[]
                                     _____ _____ _____
                                    |     |     |     |
                                    | L3  | L5  | L1  |         size = NbLayerParams
                                    |_____|_____|_____|
                                       |     |      |
                                       |     |      |_____DeviceName
                                       |     |_____DeviceName
                                       |_____DeviceName

              Temporary Layer Array : LayerTmpArray_p[]
                                     _____ _____ _____
                                    |     |     |     |
                                    | L3  | L5  | L1  |         size = NbLayerParams
                                    |_____|_____|_____|
                                       |     |      |
                                       |     |      |_____Handle
                                       |     |_____Handle
                                       |_____Handle

Parameters  : Handle, pointer to LayerDisplayParams structure, size of this structure
Assumptions : None
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_ConnectLayers(const STVMIX_Handle_t Handle,
                                    const STVMIX_LayerDisplayParams_t * const LayerDisplayParams[],
                                    const U16 NbLayerParams)
{
    ST_ErrorCode_t            Err = ST_NO_ERROR;
    ST_ErrorCode_t            RetErr = ST_NO_ERROR;
    STLAYER_Handle_t          LayerHandle;
    stvmix_Unit_t*            Unit_p      = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*          Device_p;
    stvmix_LayerTmp_t*        LayerTmpArray_p;
    int                       i;
    STLAYER_OutputParams_t    UpdateParams;

    memset(&UpdateParams, 0, sizeof(UpdateParams));
    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

        /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Return success if nothing to connect */
    if (NbLayerParams == 0)
    {
        return(ST_NO_ERROR);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Check if max layers reached or no layer connected*/
    if ((NbLayerParams > Device_p->MaxLayer) || (NbLayerParams == 0))
    {
        /* Report */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Too many layers to connect"));

        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Allocated for Temporary layer data */
    LayerTmpArray_p = (stvmix_LayerTmp_t*) memory_allocate (Device_p->CPUPartition_p, NbLayerParams * sizeof(stvmix_LayerTmp_t));
    AddDynamicDataSize(NbLayerParams * sizeof(stvmix_LayerTmp_t));
    if (LayerTmpArray_p == NULL)
    {
        /* Report */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Not enough memory"));

        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(ST_ERROR_NO_MEMORY);
    }

    /* Initialize Temporay layer data */
    for (i=0;i<NbLayerParams; i++ )
    {
        LayerTmpArray_p[i].Handle = (STLAYER_Handle_t)NULL;
    }

    /* Check layer  already connected on other device */
    for (i=0; i<NbLayerParams; i++ )
    {
        Err = CheckLayerOnOtherDevice(Device_p, LayerDisplayParams[i]->DeviceName);
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Layer '%s' already connected on other device", LayerDisplayParams[i]->DeviceName));

            /* Free Temporary layer data */
            memory_deallocate (Device_p->CPUPartition_p, LayerTmpArray_p);

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(Err);
        }
     }

    /* Get Layer Handles */
    for (i=0; i<NbLayerParams; i++ )
    {
        /* Check layer already connected on same device*/
        LayerHandle = GetHandleOfLayerName(Device_p, LayerDisplayParams[i]->DeviceName);
        if (LayerHandle == 0)
        {
            STLAYER_OpenParams_t   LayerOpenParam;

            /* Open Layer connection */
            Err = STLAYER_Open(((const STVMIX_LayerDisplayParams_t *) (LayerDisplayParams[i]))->DeviceName,
                            &LayerOpenParam, &LayerHandle);
            if (Err != ST_NO_ERROR)
            {
                int j;

                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to open device layer '%s'", LayerDisplayParams[i]->DeviceName));

                /* Close all temporary Layer connections */
                for (j=0; j<i; j++ )
                {
                    if (LayerTmpArray_p[j].IsTmp == TRUE)
                    {
                        Err = STLAYER_Close(LayerTmpArray_p[j].Handle);
                    }
                }

                /* Free Temporary layer data */
                memory_deallocate (Device_p->CPUPartition_p, LayerTmpArray_p);

                /* Free access */
                STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

                return (STVMIX_ERROR_LAYER_UNKNOWN);
            }
            LayerTmpArray_p[i].IsTmp = TRUE;
        }
        else
        {
            LayerTmpArray_p[i].IsTmp = FALSE;
        }
        LayerTmpArray_p[i].Handle = LayerHandle;
    }

    if (Device_p->Layers.NbConnect != 0)
    {
        stvmix_LayerConnect_t* DisconnectLayerArray_p;
        U32                    NbToDisconnect;

        /* Allocate layer list to be disconnected */
        DisconnectLayerArray_p = (stvmix_LayerConnect_t*) memory_allocate (Device_p->CPUPartition_p,
                                                                       Device_p->Layers.NbConnect * sizeof(stvmix_LayerConnect_t));
        AddDynamicDataSize(Device_p->Layers.NbConnect * sizeof(stvmix_LayerConnect_t));
        if (DisconnectLayerArray_p == NULL)
        {
            /* Report */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Not enough memory"));

            /* Free Temporary layer data */
            memory_deallocate (Device_p->CPUPartition_p, LayerTmpArray_p);

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            return(ST_ERROR_NO_MEMORY);
        }

        /* Look if layer need to be Disconnected => If yes, add it in the list  */
        NbToDisconnect = 0;
        for (i=0; i<Device_p->Layers.NbConnect; i++)
        {
            if (IsLayerStillConnected(Device_p->Layers.ConnectArray_p[i].DeviceName,LayerDisplayParams,NbLayerParams) == FALSE)
            {
                DisconnectLayerArray_p[NbToDisconnect].Handle = Device_p->Layers.ConnectArray_p[i].Handle;
                NbToDisconnect++;
            }
            /* else the layer is still connected but may have a different Z-position!! */
        }

        /* Check Parameter for layer which need to be disconnected : TBD!!! */

        /* Disconnect layers which have to be disconnected */
        UpdateParams.UpdateReason   = STLAYER_DISCONNECT_REASON;
        UpdateParams.DisplayHandle  = Unit_p->Handle;
        for (i=0; i<NbToDisconnect; i++)
        {
            /* Return no error. Should have been checked previously */
            STLAYER_UpdateFromMixer(DisconnectLayerArray_p[i].Handle,&UpdateParams);

            STLAYER_Close(DisconnectLayerArray_p[i].Handle);
        }
        /* Free layer list to be disconnected */
        memory_deallocate (Device_p->CPUPartition_p, DisconnectLayerArray_p);
    }

    /* Check Parameter for layer which need to be connected or updated : TBD!!!!  */

    /* Update from mixer : Start to connect from Back layer because each STLAYER_UpdateFromMixer connection put the layer at the Front !!!!*/
    for (i=0; i<NbLayerParams; i++ )
    {
        UpdateParams.UpdateReason       = STLAYER_SCREEN_PARAMS_REASON | STLAYER_VTG_REASON | STLAYER_CONNECT_REASON;
        UpdateParams.AspectRatio        = Device_p->ScreenParams.AspectRatio;
        UpdateParams.ScanType           = Device_p->ScreenParams.ScanType;
        UpdateParams.FrameRate          = Device_p->ScreenParams.FrameRate;
        UpdateParams.Width              = Device_p->ScreenParams.Width;
        UpdateParams.Height             = Device_p->ScreenParams.Height;
        UpdateParams.XStart             = Device_p->ScreenParams.XStart;
        UpdateParams.YStart             = Device_p->ScreenParams.YStart;
        GetDisplayLatencyAndFrameBufferHoldTime(Device_p, &UpdateParams.FrameBufferDisplayLatency, &UpdateParams.FrameBufferHoldTime);
        if (i == 0)
        {
            UpdateParams.BackLayerHandle = (STLAYER_Handle_t)NULL;
        }
        else
        {
            UpdateParams.BackLayerHandle = LayerTmpArray_p[i-1].Handle;
        }
        if (i == (NbLayerParams - 1))
        {
            UpdateParams.FrontLayerHandle = (STLAYER_Handle_t)NULL;
        }
        else
        {
            UpdateParams.FrontLayerHandle = LayerTmpArray_p[i+1].Handle;
        }
        UpdateParams.DisplayHandle      = Unit_p->Handle;
        strcpy(UpdateParams.VTGName , Device_p->VTGName);

        /* Update from Mixer */
        RetErr = STLAYER_UpdateFromMixer(LayerTmpArray_p[i].Handle,&UpdateParams);
        if (RetErr != ST_NO_ERROR)
        {
            /* First implementation without any UNDO for previous loop calls  : TBD!!!*/

            /* Free Temporary layer data */
            memory_deallocate (Device_p->CPUPartition_p, LayerTmpArray_p);

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STLAYER_UpdateFromMixer : %d\n",RetErr));
            if (RetErr == ST_ERROR_INVALID_HANDLE)
            {
                /* Layer may have been destroyed */
                return(STVMIX_ERROR_LAYER_ACCESS);
            }
            else
            {
                return(STVMIX_ERROR_LAYER_UPDATE_PARAMETERS);
            }
        }

        /* Update list of connected layers */
        Device_p->Layers.ConnectArray_p[i].Handle = LayerTmpArray_p[i].Handle ;
        strcpy(Device_p->Layers.ConnectArray_p[i].DeviceName, LayerDisplayParams[i]->DeviceName);
    }
    Device_p->Layers.NbConnect = NbLayerParams ;

    /* Free Temporary layer data */
    memory_deallocate (Device_p->CPUPartition_p, LayerTmpArray_p);

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_DisconnectLayers
Description : Disconnect layers from a mixer
Parameters  : Handle on a device
Assumptions : None
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, STVMIX_ERROR_LAYER_ACCESS
*******************************************************************************/
ST_ErrorCode_t STVMIX_DisconnectLayers(const STVMIX_Handle_t Handle)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    ST_ErrorCode_t          RetErr = ST_NO_ERROR;
    stvmix_Unit_t*          Unit_p      = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*        Device_p;
    unsigned int                     i;
    STLAYER_OutputParams_t  UpdateParams;

    memset(&UpdateParams, 0, sizeof(UpdateParams));
    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

            /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    if (Device_p->Layers.NbConnect == 0)
    {
        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return(ST_NO_ERROR);
    }

    /* Check UpdateFromMixer parameters */

    /* disconnect layers */
    UpdateParams.UpdateReason   = STLAYER_DISCONNECT_REASON;
    UpdateParams.DisplayHandle  = Unit_p->Handle;
    for (i=0; i< Device_p->Layers.NbConnect; i++)
    {
        /* Update from Mixer */
        RetErr = STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[i].Handle,&UpdateParams);
        if (RetErr != ST_NO_ERROR)
        {
            /* First implementation without any UNDO for previous loop calls  : TBD!!!*/

            /* Free access */
            STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STLAYER_UpdateFromMixer : %d\n",RetErr));
            if (RetErr == ST_ERROR_INVALID_HANDLE)
            {
                /* Layer may have been destroyed */
                return(STVMIX_ERROR_LAYER_ACCESS);
            }
            else
            {
                return(STVMIX_ERROR_LAYER_UPDATE_PARAMETERS);
            }
        }

        /* Close connection to Layer : don't care if it fails */
        STLAYER_Close(Device_p->Layers.ConnectArray_p[i].Handle);
    }

    Device_p->Layers.NbConnect = 0;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_GetConnectedLayers
Description : Ability to retrieve connected layers
Parameters  : Handle on a device,
              Layer Number starting from 1 and the farthest from the eyes
              Pointer to ST_DeviceName_t
Assumptions : None
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER,
              STVMIX_ERROR_LAYER_UNKNOWN
*******************************************************************************/
ST_ErrorCode_t STVMIX_GetConnectedLayers(const STVMIX_Handle_t Handle,
                                        const U16 LayerPosition,
                                        STVMIX_LayerDisplayParams_t* const LayerParams_p)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    stvmix_Unit_t*          Unit_p      = (stvmix_Unit_t*) Handle ;
    stvmix_Device_t*        Device_p;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

                /* Check Handle Validity */
    if (!IsValidHandle(Handle))
        return ST_ERROR_INVALID_HANDLE;



    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    /* Protect access */
    STOS_SemaphoreWait(Device_p->CtrlAccess_p);

    /* Check Params */
    if ((LayerPosition == 0) ||
        (LayerParams_p == NULL) ||
        (Device_p->MaxLayer < LayerPosition))
    {
        /* Free access */
        STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Layers are connected & layer number exists */
    if((Device_p->Layers.NbConnect !=0 ) &&
       (Device_p->Layers.NbConnect >= LayerPosition))
    {
        strcpy(LayerParams_p->DeviceName, Device_p->Layers.ConnectArray_p[LayerPosition-1].DeviceName);
        LayerParams_p->ActiveSignal = FALSE; /* ignored TBD!!! */
    }
    else
    {
        Err = STVMIX_ERROR_LAYER_UNKNOWN;
    }

    /* Free access */
    STOS_SemaphoreSignal(Device_p->CtrlAccess_p);

    return(Err);
}



/*******************************************************************************
Name        : STVMIX_OpenVBIViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_OpenVBIViewPort(STVMIX_Handle_t     Handle,
                                      STVMIX_VBIViewPortType_t    VBIType,
                                      STVMIX_VBIViewPortHandle_t*   VPHandle_p)
{

 ST_ErrorCode_t              Err                 = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107) || defined(ST_5162)

 STDISP_VBIViewPortType_t    DispVBIType;
 /*STDISP_VBIViewPortHandle_t* DISPVPHandle_p;*/
 stvmix_Unit_t*          Unit_p  = (stvmix_Unit_t*) Handle ;

if (VBIType==STVMIX_VBI_TXT_ODD)
        {
            DispVBIType=STDISP_VBI_TXT_ODD;
        }
        else
        {
            DispVBIType=STDISP_VBI_TXT_EVEN;
        }

  Err = STDISP_OpenVBIViewPort(Unit_p->Handle,DispVBIType,(STDISP_VBIViewPortHandle_t *)VPHandle_p);
#else
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(VBIType);
    UNUSED_PARAMETER(VPHandle_p);
#endif


return(Err);
}




/*******************************************************************************
Name        : STVMIX_SetVBIViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_SetVBIViewPortParams(STVMIX_VBIViewPortHandle_t  VPHandle,
                                          STVMIX_VBIViewPortParams_t* Params_p)
{

    ST_ErrorCode_t              Err                 = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined(ST_5105) || defined(ST_5188)|| defined(ST_5525) || defined(ST_5107) || defined(ST_5162)
    STDISP_VBIViewPortParams_t DispParams_p ;

    DispParams_p.Source_p=(Params_p->Source_p);
    DispParams_p.LineMask=(Params_p->LineMask);
    Err = STDISP_SetVBIViewPortParams((STDISP_VBIViewPortHandle_t )VPHandle,&DispParams_p);
#else
    UNUSED_PARAMETER(VPHandle);
    UNUSED_PARAMETER(Params_p);
#endif

return(Err);
}

/*******************************************************************************
Name        : STVMIX_EnableVBIViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_EnableVBIViewPort(STVMIX_VBIViewPortHandle_t  VPHandle)
{

    ST_ErrorCode_t              Err                 = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107) || defined(ST_5162)
    Err = STDISP_EnableVBIViewPort((STDISP_VBIViewPortHandle_t )VPHandle);
#else
    UNUSED_PARAMETER(VPHandle);
#endif

    return(Err);
}
/*******************************************************************************
Name        : STVMIX_DisbleVBIViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_DisableVBIViewPort(STVMIX_VBIViewPortHandle_t  VPHandle)
{

    ST_ErrorCode_t              Err                 = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107) || defined(ST_5162)
    Err = STDISP_DisableVBIViewPort((STDISP_VBIViewPortHandle_t )VPHandle);
#else
    UNUSED_PARAMETER(VPHandle);
#endif

    return(Err);
}

/*******************************************************************************
Name        : STVMIX_CloseVBIViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_CloseVBIViewPort(STVMIX_VBIViewPortHandle_t  VPHandle)
{

    ST_ErrorCode_t              Err         = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107) || defined(ST_5162)
    Err = STDISP_CloseVBIViewPort((STDISP_VBIViewPortHandle_t )VPHandle);
#else
    UNUSED_PARAMETER(VPHandle);
#endif

    return(Err);
}

/*******************************************************************************
Name        : GetDisplayLatencyAndBufferHoldTime
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : The display latency and the buffers hold time
*******************************************************************************/
static void GetDisplayLatencyAndFrameBufferHoldTime(stvmix_Device_t* Device_p, U8* DisplayLatency_p, U8* FrameBufferHoldTime_p)
{
    switch(Device_p->DeviceType)
    {
        case STVMIX_COMPOSITOR:
        case STVMIX_COMPOSITOR_FIELD:
        case STVMIX_COMPOSITOR_FIELD_SDRAM:
        case STVMIX_COMPOSITOR_422:
        case STVMIX_COMPOSITOR_FIELD_422:
        case STVMIX_COMPOSITOR_FIELD_SDRAM_422:
            *DisplayLatency_p       = 4;
            *FrameBufferHoldTime_p  = 4;
            break;
        case STVMIX_COMPOSITOR_FIELD_COMBINED:
        case STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM:
        case STVMIX_COMPOSITOR_FIELD_COMBINED_422:
        case STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422:
            *DisplayLatency_p       = 2;
            *FrameBufferHoldTime_p  = 1;
            break;
        default:
            *DisplayLatency_p       = 1;
            *FrameBufferHoldTime_p  = 1;
            break;
    }
} /* End of GetDisplayLatencyAndFrameBufferHoldTime() function */

/* Global Flicker Filter management */

/*******************************************************************************
Name        : STVMIX_EnableGlobalFlickerFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_EnableGlobalFlickerFilter(const STVMIX_Handle_t Handle)
{
    ST_ErrorCode_t   Err = ST_NO_ERROR;
    stvmix_Unit_t*   Unit_p = (stvmix_Unit_t*) Handle ;
	stvmix_Device_t* Device_p;


    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
    {
		return ST_ERROR_INVALID_HANDLE;
	}

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    if((Device_p->DeviceType == STVMIX_COMPOSITOR) || (Device_p->DeviceType == STVMIX_COMPOSITOR_422))
    {
		/* Set STDISP */
		Err = STDISP_EnableGlobalFlickerFilter(Unit_p->Handle);
    	if (Err != ST_NO_ERROR)
    	{
    	    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_EnableGlobalFlickerFilter : %d\n",Err));
    	    return(Err);
    	}
	}
	else
	{
		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	}


    return(Err);
}

/*******************************************************************************
Name        : STVMIX_DisableGlobalFlickerFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVMIX_DisableGlobalFlickerFilter(const STVMIX_Handle_t Handle)
{
    ST_ErrorCode_t   Err = ST_NO_ERROR;
    stvmix_Unit_t*   Unit_p = (stvmix_Unit_t*) Handle ;
	stvmix_Device_t* Device_p;

    /* Check Handle validity */
    if (Handle == (STVMIX_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Handle Validity */
    if (!IsValidHandle(Handle))
    {
		return ST_ERROR_INVALID_HANDLE;
	}

    if (Unit_p->UnitValidity != STVMIX_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Retrive Device structure */
    Device_p = Unit_p->Device_p;

    if((Device_p->DeviceType == STVMIX_COMPOSITOR) || (Device_p->DeviceType == STVMIX_COMPOSITOR_422))
    {
    	/* Set STDISP */
		Err = STDISP_DisableGlobalFlickerFilter(Unit_p->Handle);
    	if (Err != ST_NO_ERROR)
    	{
    	    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Err STDISP_DisableGlobalFlickerFilter : %d\n",Err));
    	    return(Err);
    	}
	}
	else
	{
		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	}

    return(Err);
}
