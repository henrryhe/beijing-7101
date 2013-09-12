/*******************************************************************************

File name   : disp_gdm.c

Description : module allowing to display a graphic throught GDMA
 *            Warning: do not use along with STLAYER nor STAVMEM.

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
17 Dec 2003        Created (based on Disp_gam.c)                     MH
25 Feb 2004        added 5100 support                                MH
30 Mar 2004        5100 support update                               TM
03 dec 2004        Port to 5105 + 5301 + 8010, supposing same as 5100 TA+HG
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include "testcfg.h"

#ifdef USE_DISP_GDMA

#include "stdevice.h"
#include "stsys.h"
#include "testtool.h"
#include "api_cmd.h"
#include "stgxobj.h"

#ifndef USE_VTGSIM
    #include "stevt.h"
    #include "clevt.h"
#ifdef USE_VTG
    #include "stvtg.h"
    #include "vtg_cmd.h"
#else /* not USE_VTG */
    #error ERROR: Cannot display if there is no VTG !
#endif /* not USE_VTG */
#endif /* USE_VTGSIM */


/* Variable ----------------------------------------------------------------- */

/* Private constants -------------------------------------------------------- */

#ifndef GAM_BITMAP_NAME
#define GAM_BITMAP_NAME_1 "merou4.gam"  /* 422 Raster YCbCr */
#define GAM_BITMAP_NAME_2 "clown.gam"  /* 422 Raster YCbCr */
#endif /* GAM_BITMAP_NAME */

/* GDMA Register */
#if defined (mb428)
#define GDMA_BASE_ADD_1 (ST5525_GDMA1_BASE_ADDR)
#define GDMA_BASE_ADD_2 (ST5525_GDMA2_BASE_ADDR)
#else
#define GDMA_BASE_ADD_1 (VOS_BASE_ADDRESS + 0xc00)
#define GDMA_BASE_ADD_2 (VOS_BASE_ADDRESS + 0xe00)
#endif /* mb428 */

#if defined (ST_5301)
#define AVMEM_BASE_ADDRESS  0xC0800000
#elif defined (ST_5525)
#define AVMEM_BASE_ADDRESS  0x81000000
#endif
#define REG_CTL 0x00
#define REG_NIN 0x04
#define REG_NVN 0x08
#define REG_LVF 0x0c
#define REG_PML 0x1c
#define REG_PKZ 0xfc
#define VTG_VFE_ITM                         0x98        /* Interrupt Mask */
#define VTG_VFE_ITS                         0x9C        /* Interrupt Status */
#define VTG_VFE_STA                         0xA0        /* Status register */
#define VTG_VFE_IT_VSB                      0x00000008         /* 5100 */
#define VTG_VFE_IT_VST                      0x00000010         /* 5100 */

/* Gamma File type.                                                  */
#define GAMMA_FILE_YCBCR422R  0x92

/* Types -------------------------------------------------------------------- */

typedef struct Bitmap_s
{
    U32 Width;
    U32 Height;
    U32 Pitch;
    void * Data1_p;
} Bitmap_t;

typedef struct Node_s
{
    U32 GDMA_NIP;
    U32 GDMA_VPO;
    U32 GDMA_VPS;
    U32 GDMA_PML;
    U32 GDMA_PTY;

    /* Stuffing to be 16 Bytes multiple */
    U32  Stuffing1;
    U32  Stuffing2;
    U32  Stuffing3;
  } Node_t;

/* Private variables  ------------------------------------------------------- */

static BOOL     DispGdm_GDMADisplayCalled0nce  = FALSE;
static U32      DispGdm_MemAddress             = AVMEM_BASE_ADDRESS;
static Node_t*  DispGdm_Node1_p;
static Node_t*  DispGdm_Node2_p;
static Node_t*  DispGdm_Node3_p;
static Node_t*  DispGdm_Node4_p;
static Bitmap_t DispGdm_Bitmap_1;
static Bitmap_t DispGdm_Bitmap_2;
static S32 GdmaChoice;
static S32  XDO, YDO ;

#ifndef USE_VTGSIM
    static STEVT_DeviceSubscribeParams_t     SubscribeParams;
    static STEVT_Handle_t                    Handle ;
    static STEVT_OpenParams_t OpenParams;
#endif /* USE_VTGSIM */

/* Private Function prototypes ---------------------------------------------- */

static ST_ErrorCode_t GUTIL_LoadBitmap(char *filename, Bitmap_t * Bitmap_p);

/* Functions ---------------------------------------------------------------- */
/*  #define USE_VTGSIM -----TA--- */

#if defined(mb390)|| defined(mb400)|| defined(mb424)|| defined(mb421)|| defined(mb457)|| defined(mb428) || defined(mb436)\
 || defined(DTT5107) || defined(SAT5107)

#ifdef USE_VTGSIM
void Vtg_SetInterrupt(void)
{

    U32  InMasks;
    InMasks = STSYS_ReadRegDev32LE(VTG_BASE_ADDRESS + VTG_VFE_ITM);
    InMasks |= VTG_VFE_IT_VSB | VTG_VFE_IT_VST;
    STSYS_WriteRegDev32LE(((U32)VTG_BASE_ADDRESS + VTG_VFE_ITM), InMasks);
    /*STSYS_WriteRegDev32LE(((U32)VTG_BASE_ADDRESS + VTG_VFE_ITM),0x18); */
    #if defined (ST_5525)
      if(GdmaChoice == 1)
      {  InMasks = STSYS_ReadRegDev32LE(VTG_1_BASE_ADDRESS + VTG_VFE_ITM);
        InMasks |= VTG_VFE_IT_VSB | VTG_VFE_IT_VST;
        STSYS_WriteRegDev32LE(((U32)VTG_1_BASE_ADDRESS + VTG_VFE_ITM), InMasks);
      }
    #endif


} /* end of Vtg_SetInterrupt */
#ifdef ST_OS21
int vtg_InterruptHandler(void * Param)
#endif  /* ST_OS21 */
#ifdef ST_OS20
void vtg_InterruptHandler(void * Param)
#endif /* ST_OS20 */
{
    U32 InStatus;
    #if defined (ST_5525)
       U32 InStatus1;
        InStatus  = STSYS_ReadRegDev32LE(VTG_BASE_ADDRESS + VTG_VFE_ITS);

        if ((InStatus & VTG_VFE_IT_VST) == VTG_VFE_IT_VST)
        {
            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_NVN, DispGdm_Node1_p);
        }
        else if ((InStatus & VTG_VFE_IT_VSB) == VTG_VFE_IT_VSB)
        {
            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_NVN, DispGdm_Node2_p);
        }
        if(GdmaChoice == 1)
        {
            InStatus1 = STSYS_ReadRegDev32LE(VTG_1_BASE_ADDRESS + VTG_VFE_ITS);
            if ((InStatus1 & VTG_VFE_IT_VST) == VTG_VFE_IT_VST)
            {
                STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_NVN, DispGdm_Node3_p);
            }
            else if ((InStatus1 & VTG_VFE_IT_VSB) == VTG_VFE_IT_VSB)
            {
                STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_NVN, DispGdm_Node4_p);
            }
        }

    #else
    InStatus = STSYS_ReadRegDev32LE(VTG_BASE_ADDRESS + VTG_VFE_ITS);

    /*STSYS_WriteRegDev32LE(GDMA_BASE_ADD + REG_NVN, DispGdm_Node1_p);   */
    if ((InStatus & VTG_VFE_IT_VST) == VTG_VFE_IT_VST)
    {
        STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_NVN, DispGdm_Node1_p);
        if(GdmaChoice == 1)
        {
            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_NVN, DispGdm_Node3_p);
        }
    }
    else if ((InStatus & VTG_VFE_IT_VSB) == VTG_VFE_IT_VSB)
    {
        STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_NVN, DispGdm_Node2_p);
         if(GdmaChoice == 1)
        {
            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_NVN, DispGdm_Node4_p);
        }
    }
    #endif /*5525*/
#ifdef ST_OS21
    return(OS21_SUCCESS);
#endif /* ST_OS21 */
} /* End of vtg_InterruptHandler() function */

#else  /* USE_VTGSIM not defined  */

static void vtg_InterruptHandl(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, \
                          STEVT_EventConstant_t Event, const void *EventData, const void *SubscriberData_p)
{
    STVTG_VSYNC_t * VSyncData = (STVTG_VSYNC_t*) EventData;
    if ((*VSyncData) == STVTG_TOP)
    {
        STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_NVN, DispGdm_Node1_p);
         if(GdmaChoice == 1)
        {
            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_NVN, DispGdm_Node3_p);
        }
    }
    else
    {
        STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_NVN, DispGdm_Node2_p);
         if(GdmaChoice == 1)
        {
            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_NVN, DispGdm_Node4_p);
        }
    }

}
#endif /* USE_VTGSIM */

BOOL GDMA_Display(void)
{

    char                             FileName_1[80];
    char                             FileName_2[80];
    ST_ErrorCode_t                   Error;

    if (DispGdm_GDMADisplayCalled0nce)
    {
        printf("the Gam file was already loaded \r\n");


    }
    else
    {
        DispGdm_GDMADisplayCalled0nce = TRUE;
        DispGdm_MemAddress            = AVMEM_BASE_ADDRESS;

        /* Allocate 2 nodes (bot + top) */
        /* ---------------------------- */
        /* affect pointers */
        DispGdm_Node1_p     = (Node_t*) DispGdm_MemAddress;
        DispGdm_Node2_p     = (Node_t*)((U8*)(DispGdm_Node1_p) + sizeof(Node_t));
        sprintf(FileName_1, "%s%s", StapigatDataPath, GAM_BITMAP_NAME_1);
        Error = GUTIL_LoadBitmap(FileName_1,&DispGdm_Bitmap_1);
        if (Error)
        {
            printf("ERROR LOADING BITMAP\r\n");
            return(TRUE);
        }
        else
        {
            printf("LOADING BITMAP OK\r\n");
        }


        if(GdmaChoice == 1)
        {

            DispGdm_Node3_p     = (Node_t*) DispGdm_MemAddress;
            DispGdm_Node4_p     = (Node_t*)((U8*)(DispGdm_Node3_p) + sizeof(Node_t));
            sprintf(FileName_2, "%s%s", StapigatDataPath, GAM_BITMAP_NAME_2);
            Error = GUTIL_LoadBitmap(FileName_2,&DispGdm_Bitmap_2);

            if (Error)
            {
                printf("ERROR LOADING BITMAP\r\n");
                return(TRUE);
            }
            else
            {
                printf("LOADING BITMAP OK\r\n");
            }
        }

        /*******for the GDMA_MAIN*****/

        DispGdm_Node1_p->GDMA_NIP = 0;
        DispGdm_Node1_p->GDMA_VPO = (YDO << 16)   |  XDO ;
        DispGdm_Node1_p->GDMA_VPS = ((YDO + DispGdm_Bitmap_1.Height - 1) << 16) | (XDO + DispGdm_Bitmap_1.Width - 1 );
        DispGdm_Node1_p->GDMA_PML = (U32)DispGdm_Bitmap_1.Data1_p;
        DispGdm_Node1_p->GDMA_PTY = ((U32)1  << 31)                          /* last node */
                                    | ((U32)0  << 24)                          /* = big endian */
                                    | ((U32)18 << 16)                          /* = YCbCr888 */
                                    | (((U32)DispGdm_Bitmap_1.Pitch * 2) << 0);  /* pitch */

        /* duplicate and link 2nd node */
        *DispGdm_Node2_p          = *DispGdm_Node1_p;
        DispGdm_Node2_p->GDMA_PML = (U32)DispGdm_Bitmap_1.Data1_p + DispGdm_Bitmap_1.Pitch;


        /*******for the GDMA_AUX*****/
         if(GdmaChoice == 1)
        {

            DispGdm_Node3_p->GDMA_NIP = 0;
            DispGdm_Node3_p->GDMA_VPO = (YDO << 16)   |  XDO ;
            DispGdm_Node3_p->GDMA_VPS = ((YDO + DispGdm_Bitmap_2.Height - 1) << 16) | (XDO + DispGdm_Bitmap_2.Width - 1 );
            DispGdm_Node3_p->GDMA_PML = (U32)DispGdm_Bitmap_2.Data1_p;
            DispGdm_Node3_p->GDMA_PTY = ((U32)1  << 31)                          /* last node */
                                    | ((U32)0  << 24)                          /* = big endian */
                                    | ((U32)18 << 16)                          /* = YCbCr888 */
                                    | (((U32)DispGdm_Bitmap_2.Pitch * 2) << 0);  /* pitch */

            /* duplicate and link 2nd node */
            *DispGdm_Node4_p          = *DispGdm_Node3_p;
             DispGdm_Node4_p->GDMA_PML = (U32)DispGdm_Bitmap_2.Data1_p + DispGdm_Bitmap_2.Pitch;

            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_CTL, 0x04108080);
            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_LVF, (YDO - 1));
            STSYS_WriteRegDev32LE(GDMA_BASE_ADD_2 + REG_PKZ, 0);
         }

        /* Set GDMA_CTL register :
        Chroma_format = 0 (offset, ie unsigned)
        VBI_Present   = 0
        Video_Present  = 1
        601_709_sel   = 0 (ITU601)
        BigNotLittle  = 0 (Little)
        BlankingYCbCr = 0x10, 0x80, 0x80
        */

        STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_CTL, 0x04108080);
        STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_LVF, (YDO - 1));
        STSYS_WriteRegDev32LE(GDMA_BASE_ADD_1 + REG_PKZ, 0);


   }

#ifdef USE_VTGSIM
        Vtg_SetInterrupt();
        #if defined (ST_5100)
        interrupt_install(ST5100_VTG_VSYNC_INTERRUPT, 0, (void(*)(void*)) vtg_InterruptHandler, NULL) ;
        #elif defined (ST_5105)
        interrupt_install(ST5105_VTG_VSYNC_INTERRUPT, 0, (void(*)(void*)) vtg_InterruptHandler, NULL) ;
        #elif defined (ST_5188)
        interrupt_install(ST5188_VTG_VSYNC_INTERRUPT, 0, (void(*)(void*)) vtg_InterruptHandler, NULL) ;
        #elif defined (ST_5107)
        interrupt_install(ST5107_VTG_VSYNC_INTERRUPT, 0, (void(*)(void*)) vtg_InterruptHandler, NULL) ;
        #else
            #ifdef ST_OS21
                if(interrupt_install(VTG_VSYNC_INTERRUPT, 0, vtg_InterruptHandler, NULL)!= OS21_SUCCESS )
                {
                    Error = ST_ERROR_INTERRUPT_INSTALL ;
                    printf("INTERRUPT INSTALL FAILED") ;
                    return(TRUE);
                }
                #if defined (ST_5525)
                  if(GdmaChoice == 1)
                  {
                    if(interrupt_install(VTG_1_VSYNC_INTERRUPT, 0, vtg_InterruptHandler, NULL)!= OS21_SUCCESS )
                    {
                        Error = ST_ERROR_INTERRUPT_INSTALL ;
                        printf("INTERRUPT INSTALL FAILED") ;
                        return(TRUE);
                    }
                  }
               #endif
            #endif /*ST_OS21*/
         #endif
   #ifdef STAPI_INTERRUPT_BY_NUMBER
        if (interrupt_enable_number(VTG_VSYNC_INTERRUPT) != 0)
         {
             Error = ST_ERROR_INTERRUPT_INSTALL ;
             printf("INTERRUPT ENABLE FAILED") ;
             return(TRUE);
         }
         #if defined (ST_5525)
           if(GdmaChoice == 1)
           {
             if (interrupt_enable_number(VTG_1_VSYNC_INTERRUPT) != 0)
             {
                 Error = ST_ERROR_INTERRUPT_INSTALL ;
                 printf("INTERRUPT ENABLE FAILED") ;
                 return(TRUE);
             }
           }
         #endif
   #else /*STAPI_INTERRUPT_BY_NUMBER not defined */
        #ifdef ST_OS21
            #if defined (ST_5301) || defined (ST_5525) || defined (ST_8010)
                if (interrupt_enable_number(VTG_VSYNC_INTERRUPT) != 0)
                {
                    Error = ST_ERROR_INTERRUPT_INSTALL ;
                    printf("INTERRUPT ENABLE FAILED") ;
                    return(TRUE);
                }
                #if defined (ST_5525)
                  if(GdmaChoice == 1)
                  {
                    if (interrupt_enable_number(VTG_1_VSYNC_INTERRUPT) != 0)
                    {
                        Error = ST_ERROR_INTERRUPT_INSTALL ;
                        printf("INTERRUPT ENABLE FAILED") ;
                        return(TRUE);
                    }
                  }
                #endif
            #endif /*5301-5525-8010*/
        #endif /*ST_OS21*/
        #ifdef ST_OS20
            interrupt_enable(0);
        #endif
   #endif /*STAPI_INTERRUPT_BY_NUMBER*/
#else      /* not USE_VTGSIM */

        Error = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams,&Handle);
        if(Error)
        {
            printf("ERROR IN EVT OPEN\r\n");
            return(TRUE);
        }

        SubscribeParams.NotifyCallback      = vtg_InterruptHandl;
        Error = STEVT_SubscribeDeviceEvent(Handle, STVTG_DEVICE_NAME, STVTG_VSYNC_EVT, &SubscribeParams);
        if(Error)
        {
            printf("ERROR IN EVT SUBSCRIBE\r\n");
            return(TRUE);
        }

#endif  /* USE_VTGSIM */

    return(FALSE);
} /* end of GDMA_Display() */

BOOL Gdma_ChgXDO(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr = FALSE;
    S32 choice;
    RetErr = STTST_GetInteger(pars_p, 0, &choice);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "choice param (SQP/NSQP)" );
        return(TRUE);
    }

 if(choice)
  {
    /* Update XDO for square pixel display */
    XDO = 170;
  }
  else
  {
    /* not square pixel mode */
    XDO = 132;
  }
       DispGdm_Node1_p->GDMA_VPO = (YDO << 16)   |  XDO ;
       DispGdm_Node1_p->GDMA_VPS = ((YDO + DispGdm_Bitmap_1.Height - 1) << 16) | (XDO + DispGdm_Bitmap_1.Width - 1 );
      *DispGdm_Node2_p           = *DispGdm_Node1_p;

 return(FALSE);

}


#endif /* mb390 - mb400 - mb424 - mb421 */

#ifndef USE_VTGSIM
BOOL GDMA_CloseEVT(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t                   ErrCode;

    /* Unsubscribe to VTG event */
    ErrCode = STEVT_UnsubscribeDeviceEvent(Handle, STVTG_DEVICE_NAME, STVTG_VSYNC_EVT);
    if(ErrCode)
        {
            printf("ERROR IN EVT_Unsubscribe\r\n");
            return(TRUE);
        }
    /* Free handle */
    ErrCode = STEVT_Close(Handle);
    if(ErrCode)
        {
            printf("ERROR IN EVT_Close\r\n");
            return(TRUE);
        }
        return(FALSE);
}
#endif


static ST_ErrorCode_t GUTIL_LoadBitmap(char *filename, Bitmap_t * Bitmap_p)
{

    FILE *fstream;                      /* File handle for read operation          */
    U32 ExpectedSize;
    U32  size;                          /* Used to test returned size for a read   */
    U32 dummy[2];                       /* used to read colorkey, but not used     */
    struct
    {
        U16 Header_size;    /* In 32 bit word      */
        U16 Signature;      /* usualy 0x444F       */
        U16 Type;           /* See previous define */
        U8  Properties;     /* Usualy 0x0          */
        U8  NbPict;         /* Number of picture   */
        U32 width;
        U32 height;
        U32 nb_pixel;       /* width*height        */
        U32 zero;           /* Usualy 0x0          */
    } Gamma_Header;                   /* Header of Bitmap file                   */
    U32 Signature;
    ST_ErrorCode_t ErrCode;

    ErrCode   = ST_NO_ERROR;
    Signature = 0x0;

    /* Check parameter                                               */
    if ((filename == NULL) || (Bitmap_p == NULL))
    {
        printf("Error: Null Pointer, bad parameter\n");
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Open file handle                                              */
    if (ErrCode == ST_NO_ERROR)
    {
        fstream = fopen(filename, "rb");
        if( fstream == NULL )
        {
            printf("Unable to open \'%s\'\n", filename );
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Read Header                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        printf("Reading file Header ... \n");
        ExpectedSize = sizeof(Gamma_Header);
        size = fread((void *)&Gamma_Header, 1, ExpectedSize, fstream);
        if (size != ExpectedSize)
        {
            printf("Read error %d byte instead of %d\n",
                          size, ExpectedSize);
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
        }
    }

    /* Check Header                                                  */
    if (ErrCode == ST_NO_ERROR)
    {
        /* For historical reason, NbPict = 0, means that one      */
       /* picture is on the file, so updae it.                   */
       if (Gamma_Header.NbPict == 0)
       {
           Gamma_Header.NbPict = 1;
       }
    }

    /* If present read colorkey but do not use it.                   */
    if (ErrCode == ST_NO_ERROR)
    {
        if (Gamma_Header.Header_size == 0x8)
        {
            /* colorkey are 2 32-bits word, but it's safer to use    */
            /* sizeof(dummy) when reading. And to check that it's 4. */
            size = fread((void *)dummy, 1, sizeof(dummy), fstream);
            if (size != 4)
            {
                printf("Read error %d byte instead of %d\n", size,4);
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }
        }
    }

    /* In function of bitmap type, configure some variables          */
    /* And do bitmap allocation                                      */
    if (ErrCode == ST_NO_ERROR)
    {
        Bitmap_p->Width                  = Gamma_Header.width;
        Bitmap_p->Height                 = Gamma_Header.height;

        /* Configure in function of the bitmap type                  */
        switch (Gamma_Header.Type)
        {
            case GAMMA_FILE_YCBCR422R :
                Signature = 0x422F;
                if ((Gamma_Header.width % 2) == 0)
                {
                    Bitmap_p->Pitch = (U32)(Gamma_Header.width * 2);
                }
                else
                {
                    Bitmap_p->Pitch = (U32)((Gamma_Header.width - 1) * 2 + 4);
                }
                break;

            default :
                printf("Type not supported 0x%08x\n", Gamma_Header.Type);
                ErrCode = ST_ERROR_BAD_PARAMETER;
                break;
        } /* switch (Gamma_Header.Type) */
    }

    if (ErrCode == ST_NO_ERROR)
    {
        if (Gamma_Header.Signature != Signature)
        {
            printf("Error: Read 0x%08x, Waited 0x%08x\n",
                          Gamma_Header.Signature, Signature);
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Read bitmap                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        /* align on 0x100 */
        DispGdm_MemAddress = (DispGdm_MemAddress + 0xff ) & ~0xff;
        Bitmap_p->Data1_p = (void *) DispGdm_MemAddress;

        printf(("Reading file Bitmap data ... \n"));

        ExpectedSize = (Bitmap_p->Pitch) * (Bitmap_p->Height) * Gamma_Header.NbPict;
        size = fread (Bitmap_p->Data1_p, 1, ExpectedSize, fstream);
        if (size != ExpectedSize)
        {
            printf("Read error %d byte instead of %d\n", size, ExpectedSize);
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
        fclose (fstream);
    }

    if (ErrCode == ST_NO_ERROR)
    {
        DispGdm_MemAddress += ExpectedSize;
        printf("GUTIL loaded correctly %s file\n",filename);
    }

    return ErrCode;

} /* GUTIL_LoadBitmap () */

/*-------------------------------------------------------------------------
 * Function : GDMADisplayCmd
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : FALSE
 * ----------------------------------------------------------------------*/
BOOL GDMADisplayCmd( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr = FALSE;
    S32 ScanType;
    S32 SelectGdma;


    RetErr = STTST_GetInteger(pars_p, 1, &ScanType);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "dummy param" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 0,&SelectGdma);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected GDMA  (0=Main/1= Main and Aux)");
           return(TRUE);

        }
    GdmaChoice = SelectGdma;

    RetErr = STTST_GetInteger(pars_p, 132, &XDO);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected XStart" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 23, &YDO);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected YStart" );
        return(TRUE);
    }

#if defined(mb390)|| defined (mb400)|| defined (mb424)|| defined (mb421)|| defined (mb457)|| defined (mb428)|| defined(mb436) \
 || defined(DTT5107)
    RetErr = GDMA_Display();
#endif /* mb390 - mb400 - mb424 - mb421 */
    return(RetErr);
} /* end of GDMADisplayCmd() function */

/*-------------------------------------------------------------------------
 * Function : DispGDMA_RegisterCmd
 *            Definition of register command
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DispGDMA_RegisterCmd(void)
{
    BOOL RetErr = FALSE;

    RetErr = STTST_RegisterCommand( "GDMADisplay", GDMADisplayCmd, "Gamma Display : <Interlaced mode(def TRUE/FALSE))>");
    RetErr = STTST_RegisterCommand( "Gdma_ChgXDO", Gdma_ChgXDO, " Gdma_ChgXDO ");

    #ifndef USE_VTGSIM
        RetErr = STTST_RegisterCommand( "GDMACloseEVT", GDMA_CloseEVT, "Gamma Close");
    #endif
    if (RetErr)
    {
        printf("DispGDMA_RegisterCmd() \t: failed !\n");
    }
    else
    {
        printf("DispGDMA_RegisterCmd() \t: ok\n");
    }
    return(!RetErr);
} /* end of DispGDMA_RegisterCmd() */

#endif /* USE_DISP_GDMA */

/* end of disp_gdm.c */
