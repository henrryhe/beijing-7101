/*******************************************************************************

File name   : back_end.c

Description : functions back end

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
2007-02-08         Support 7109C3 MSPP                        Slim Ben Ezzeddine Ben Ayed


    Back end functions
    ------------------
    stlayer_InsertNodeLayerType
    stlayer_ExtractNodeLayerType
    stlayer_UpdateNodeGeneric
    stlayer_EnableCursor
    stlayer_DisableCursor
    stlayer_RemapDevice

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#ifndef ST_OSLINUX
    #include <string.h>
#endif

#include "stsys.h"
#include "stlayer.h"
#include "back_end.h"

#include "hard_mng.h"
#include "layergfx.h"

#include "filters.h"


/* Private constants -------------------------------------------------------- */

/* Color Key Config */


#define B_Cb_Pos    0
#define B_Cb_Pos    0
#define G_Y_Pos     2
#define R_Cr_Pos    4
#define BCbInRange  (0x01 << B_Cb_Pos)
#define BCbOutRange (0x03 << B_Cb_Pos)
#define GYInRange   (0x01 << G_Y_Pos)
#define GYOutRange  (0x03 << G_Y_Pos)
#define RCrInRange  (0x01 << R_Cr_Pos)
#define RCrOutRange (0x03 << R_Cr_Pos)

/* Macros ------------------------------------------------------------------- */

#define XOFFSET (stlayer_context.XContext[LayerHandle].OutputParams.XStart \
                + stlayer_context.XContext[LayerHandle].OutputParams.XOffset)

#define YOFFSET (stlayer_context.XContext[LayerHandle].OutputParams.YStart \
                + stlayer_context.XContext[LayerHandle].OutputParams.YOffset)

#define DeviceAdd(Add) ((U32)Add \
        + (U32)stlayer_context.XContext[PtViewPort->LayerHandle].\
                            VirtualMapping.PhysicalAddressSeenFromDevice_p\
        - (U32)stlayer_context.XContext[PtViewPort->LayerHandle].\
                            VirtualMapping.PhysicalAddressSeenFromCPU_p)

#define DeviceAdd2(Add) ((U32)Add \
        + (U32)Layer_p->VirtualMapping.PhysicalAddressSeenFromDevice_p\
        - (U32)Layer_p->VirtualMapping.PhysicalAddressSeenFromCPU_p)


#define BitmapAdd(Add) (U32)(STAVMEM_VirtualToDevice(Add,      \
        &stlayer_context.XContext[PtViewPort->LayerHandle].VirtualMapping))

/* Types -------------------------------------------------------------------- */

typedef void (*WriteFct_p)(stlayer_Node_t *, stlayer_Node_t *,STLAYER_Handle_t);

/* Private Function prototypes ---------------------------------------------- */

static void stlayer_ImmediateMemWrite(stlayer_Node_t *, stlayer_Node_t *,
                                        STLAYER_Handle_t);
static void stlayer_ImmediateRegWrite(stlayer_Node_t *, stlayer_Node_t *,
                                        STLAYER_Handle_t);
static ST_ErrorCode_t stlayer_UpdateBKLNode(stlayer_ViewPortHandle_t VPHandle,
                                        WriteFct_p WriteFunction1,
                                        WriteFct_p WriteFunction2 );
static ST_ErrorCode_t stlayer_UpdateCURNode(stlayer_ViewPortHandle_t VPHandle,
                                        WriteFct_p WriteFunction1,
                                        WriteFct_p WriteFunction2 );
static void stlayer_ExtractColorType(STGXOBJ_ColorType_t,U32*,U32*);
static U32 stlayer_UpdateCTLReg(stlayer_ViewPortDescriptor_t * PtViewPort);


#if defined(ST_OS21)
extern FILE * dumpstream;
#ifdef DUMP_GDPS_NODES
static void stlayer_DumpGDPNode(stlayer_Node_t node1,U8 *str);
#endif
#endif




/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stlayer_InsertNodeLayerType
Description : Insert the two nodes of a viewport in the nodes list.
            : insertion may be differed if the layer is synchronized on vtg.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_InsertNodeLayerType(stlayer_ViewPortHandle_t VPHandle)
{
    stlayer_ViewPortDescriptor_t *  PtViewPort;
    STLAYER_Handle_t                Layer;
    WriteFct_p                      WriteFunction1;
    WriteFct_p                      WriteFunction2;
    ST_DeviceName_t                 Empty="\0\0";
    U32                             Value;
    stlayer_Node_t                  InsTopNode, InsBotNode,
                                    PrevTopNode, PrevBotNode;

#if defined (DVD_SECURED_CHIP) && !defined(STLAYER_NO_STMES)
    unsigned int uMemoryStatus;
#endif

    PtViewPort = (stlayer_ViewPortDescriptor_t *)VPHandle;
    Layer      = PtViewPort->LayerHandle;

    if(strcmp(stlayer_context.XContext[Layer].VTGName, Empty) == 0)
    {
        /* No synchro : affect immediatly to prevent queues overflow */
        WriteFunction1 = stlayer_ImmediateMemWrite;
        WriteFunction2 = stlayer_ImmediateMemWrite;
    }
    else
    {
        /* Synchro */
        WriteFunction1 = stlayer_DifferedToTopWrite;
        WriteFunction2 = stlayer_DifferedToBotWrite;
    }

    /* case : 0 viewport -> 1 viewport */
    /* ------------------------------- */
    if(stlayer_context.XContext[Layer].NumberLinkedViewPorts == 1)
    {
        /* Current nodes = last nodes cross NVN */
        InsTopNode.BklNode.NVN = (U32)DeviceAdd(stlayer_context.XContext[Layer].
                                                        TmpBotNode_p);
        InsBotNode.BklNode.NVN = (U32)DeviceAdd(stlayer_context.XContext[Layer].
                                                        TmpTopNode_p);
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)

       uMemoryStatus = STMES_IsMemorySecure((void *)InsTopNode.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
      if(uMemoryStatus == SECURE_REGION)
      {
            InsTopNode.BklNode.NVN = (U32)(InsTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            InsBotNode.BklNode.NVN = (U32)(InsBotNode.BklNode.NVN | STLAYER_SECURE_ON);
      }
      else
      {
            InsTopNode.BklNode.NVN = (U32)(InsTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            InsBotNode.BklNode.NVN = (U32)(InsBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
      }


#endif

        Value = stlayer_UpdateCTLReg(PtViewPort);
        InsTopNode.BklNode.CTL = Value;
        InsBotNode.BklNode.CTL = Value;
        WriteFunction1(PtViewPort->TopNode_p,&InsTopNode,Layer);
        WriteFunction2(PtViewPort->BotNode_p,&InsBotNode,Layer);
        /* Prev nodes = tmp nodes */
        PrevTopNode.BklNode.NVN = (U32)(DeviceAdd(PtViewPort->TopNode_p));
        PrevBotNode.BklNode.NVN = (U32)(DeviceAdd(PtViewPort->BotNode_p));

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)PrevTopNode.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }


#endif

        PrevTopNode.BklNode.CTL = 0x00000007;
        PrevBotNode.BklNode.CTL = 0x00000007;
#ifdef WA_GNBycbcr
        /* patch the value for GDP2 : reserved for DVP */
        if(stlayer_context.XContext[Layer].DeviceId == 0x200)
        {
            PrevTopNode.BklNode.CTL = 0x00000012;
            PrevBotNode.BklNode.CTL = 0x00000012;
        }
#endif
        WriteFunction1((stlayer_Node_t*)
          (stlayer_context.XContext[Layer].TmpTopNode_p),&PrevTopNode,Layer);
        WriteFunction2((stlayer_Node_t*)
          (stlayer_context.XContext[Layer].TmpBotNode_p),&PrevBotNode,Layer);
    }
    /* case : last viewport in the list */
    /* -------------------------------- */
    else if(PtViewPort->Next_p==stlayer_context.XContext[Layer].LinkedViewPorts)
    {
        /* Current nodes = last nodes in the list : cross NVN */
        InsTopNode.BklNode.NVN = (U32)DeviceAdd(stlayer_context.XContext[Layer].
                                                        TmpBotNode_p);
        InsBotNode.BklNode.NVN = (U32)DeviceAdd(stlayer_context.XContext[Layer].
                                                        TmpTopNode_p);
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    uMemoryStatus = STMES_IsMemorySecure((void *)InsTopNode.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
    if(uMemoryStatus == SECURE_REGION)
        {
            InsTopNode.BklNode.NVN = (U32)(InsTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            InsBotNode.BklNode.NVN = (U32)(InsBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            InsTopNode.BklNode.NVN = (U32)(InsTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            InsBotNode.BklNode.NVN = (U32)(InsBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }

#endif

        Value = stlayer_UpdateCTLReg(PtViewPort);
        InsTopNode.BklNode.CTL = Value;
        InsBotNode.BklNode.CTL = Value;
        WriteFunction1(PtViewPort->TopNode_p,&InsTopNode,Layer);
        WriteFunction2(PtViewPort->BotNode_p,&InsBotNode,Layer);
        /* Previous nodes */
        PrevTopNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->TopNode_p);
        PrevBotNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->BotNode_p);
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)PrevTopNode.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }


#endif



        Value = stlayer_UpdateCTLReg(PtViewPort->Prev_p);
        PrevTopNode.BklNode.CTL = Value;
        PrevBotNode.BklNode.CTL = Value;
        WriteFunction1(PtViewPort->Prev_p->TopNode_p,&PrevTopNode,Layer);
        WriteFunction2(PtViewPort->Prev_p->BotNode_p,&PrevBotNode,Layer);
    }
    /* case : first viewport in the list */
    /* --------------------------------- */
    else  if (PtViewPort == stlayer_context.XContext[Layer].LinkedViewPorts)
    {
        /* Current nodes */
        InsTopNode.BklNode.NVN  = (U32)DeviceAdd(PtViewPort->Next_p->TopNode_p);
        InsBotNode.BklNode.NVN  = (U32)DeviceAdd(PtViewPort->Next_p->BotNode_p);

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)InsTopNode.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            InsTopNode.BklNode.NVN = (U32)(InsTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            InsBotNode.BklNode.NVN = (U32)(InsBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            InsTopNode.BklNode.NVN = (U32)(InsTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            InsBotNode.BklNode.NVN = (U32)(InsBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }


#endif

        InsTopNode.BklNode.CTL  = NON_VALID_REG;
        InsBotNode.BklNode.CTL  = NON_VALID_REG;
        WriteFunction1(PtViewPort->TopNode_p,&InsTopNode,Layer);
        WriteFunction2(PtViewPort->BotNode_p,&InsBotNode,Layer);
        /* Previous nodes */
        PrevTopNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->TopNode_p);
        PrevBotNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->BotNode_p);
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)PrevTopNode.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }

#endif

        PrevTopNode.BklNode.CTL  = NON_VALID_REG;
        PrevBotNode.BklNode.CTL  = NON_VALID_REG;
        WriteFunction1(stlayer_context.XContext[Layer].TmpTopNode_p,
                                                        &PrevTopNode,Layer);
        WriteFunction2(stlayer_context.XContext[Layer].TmpBotNode_p,
                                                        &PrevBotNode,Layer);
    }
    /* normal case */
    /* ----------- */
    else
    {
        /* Current nodes */
        InsTopNode.BklNode.NVN  = (U32)DeviceAdd(PtViewPort->Next_p->TopNode_p);
        InsBotNode.BklNode.NVN  = (U32)DeviceAdd(PtViewPort->Next_p->BotNode_p);

        InsTopNode.BklNode.CTL  = NON_VALID_REG;
        InsBotNode.BklNode.CTL  = NON_VALID_REG;
        WriteFunction1(PtViewPort->TopNode_p,&InsTopNode,Layer);
        WriteFunction2(PtViewPort->BotNode_p,&InsBotNode,Layer);
        /* Previous nodes */
        PrevTopNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->TopNode_p);
        PrevBotNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->BotNode_p);
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)PrevTopNode.BklNode.NVN , (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }

#endif


        PrevTopNode.BklNode.CTL = NON_VALID_REG;
        PrevBotNode.BklNode.CTL = NON_VALID_REG;
        WriteFunction1(PtViewPort->Prev_p->TopNode_p,&PrevTopNode,Layer);
        WriteFunction2(PtViewPort->Prev_p->BotNode_p,&PrevBotNode,Layer);
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stlayer_ExtractNodeLayerType
Description : Update the linked list of nodes (differed)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_ExtractNodeLayerType(stlayer_ViewPortHandle_t VPHandle)
{
    stlayer_ViewPortDescriptor_t *  PtViewPort;
    STLAYER_Handle_t                Layer;
    U32                             Value;
    WriteFct_p      WriteFunction1;
    WriteFct_p      WriteFunction2;
    ST_DeviceName_t Empty="\0\0";
    stlayer_Node_t  PrevTopNode, PrevBotNode;

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    unsigned int uMemoryStatus;
#endif


    PtViewPort = (stlayer_ViewPortDescriptor_t *)VPHandle;
    Layer      = PtViewPort->LayerHandle;
    if(strcmp(stlayer_context.XContext[Layer].VTGName, Empty) == 0)
    {
        /* No synchro : affect immediatly to prevent queues overflow */
        WriteFunction1 = stlayer_ImmediateMemWrite;
        WriteFunction2 = stlayer_ImmediateMemWrite;
    }
    else
    {
        /* Synchro */
        WriteFunction1 = stlayer_DifferedToTopWrite;
        WriteFunction2 = stlayer_DifferedToBotWrite;
    }

    /* case : 1 viewport -> 0 viewport */
    /* ------------------------------- */
    if(stlayer_context.XContext[Layer].NumberLinkedViewPorts == 1)
    /* num VP will be decremented by desc_manager */
    {
        /* Prev nodes = tmp nodes: cross NVN */
        PrevTopNode.BklNode.NVN =(U32)DeviceAdd(stlayer_context.XContext[Layer].
                                                        TmpBotNode_p);
        PrevBotNode.BklNode.NVN =(U32)DeviceAdd(stlayer_context.XContext[Layer].
                                                        TmpTopNode_p);

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)PrevTopNode.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }

#endif

        PrevTopNode.BklNode.CTL = 0x80000007;
        PrevBotNode.BklNode.CTL = 0x80000007;
#ifdef WA_GNBycbcr
        /* patch the value for GDP2 : reserved for DVP */
        if(stlayer_context.XContext[Layer].DeviceId == 0x200)
        {
            PrevTopNode.BklNode.CTL = 0x80000012;
            PrevBotNode.BklNode.CTL = 0x80000012;
        }
#endif
        WriteFunction1(stlayer_context.XContext[Layer].TmpTopNode_p,
                                                     &PrevTopNode,Layer);
        WriteFunction2(stlayer_context.XContext[Layer].TmpBotNode_p,
                                                     &PrevBotNode,Layer);
    }
    /* case : last viewport in the list */
    /* -------------------------------- */
    else if(PtViewPort->Next_p
            == stlayer_context.XContext[Layer].LinkedViewPorts)
    {
        /* Prev nodes = last nodes : cross NVN */
        PrevTopNode.BklNode.NVN =(U32)DeviceAdd(stlayer_context.XContext[Layer].
                                                        TmpBotNode_p);
        PrevBotNode.BklNode.NVN =(U32)DeviceAdd(stlayer_context.XContext[Layer].
                                                        TmpTopNode_p);
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)(PrevTopNode.BklNode.NVN), (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }


#endif


        Value = stlayer_UpdateCTLReg(PtViewPort->Prev_p);
        Value |= ((U32)1 << 31); /* Set the WaitVSync bit */
        PrevTopNode.BklNode.CTL = Value;
        PrevBotNode.BklNode.CTL = Value;
        WriteFunction1(PtViewPort->Prev_p->TopNode_p,&PrevTopNode,Layer);
        WriteFunction2(PtViewPort->Prev_p->BotNode_p,&PrevBotNode,Layer);


    }
    /* case : first viewport in the list */
    /* --------------------------------- */
    else if (PtViewPort == stlayer_context.XContext[Layer].LinkedViewPorts)
    {
        /* Prev nodes = tmp nodes */
        PrevTopNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->Next_p->TopNode_p);
        PrevBotNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->Next_p->BotNode_p);
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)PrevTopNode.BklNode.NVN , (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }


#endif


        PrevTopNode.BklNode.CTL = NON_VALID_REG;
        PrevBotNode.BklNode.CTL = NON_VALID_REG;
        WriteFunction1(stlayer_context.XContext[Layer].TmpTopNode_p,
                                                        &PrevTopNode,Layer);
        WriteFunction2(stlayer_context.XContext[Layer].TmpBotNode_p,
                                                        &PrevBotNode,Layer);
     }
    else
    {
        /* Extract the nodes */
        /* ----------------- */
        PrevTopNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->Next_p->TopNode_p);
        PrevBotNode.BklNode.NVN = (U32)DeviceAdd(PtViewPort->Next_p->BotNode_p);

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
        uMemoryStatus = STMES_IsMemorySecure((void *)PrevTopNode.BklNode.NVN, (U32)(MAX_GDP_NODE_SIZE), 0);
        if(uMemoryStatus == SECURE_REGION)
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN | STLAYER_SECURE_ON);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN | STLAYER_SECURE_ON);
        }
        else
        {
            PrevTopNode.BklNode.NVN = (U32)(PrevTopNode.BklNode.NVN & STLAYER_SECURE_MASK);
            PrevBotNode.BklNode.NVN = (U32)(PrevBotNode.BklNode.NVN & STLAYER_SECURE_MASK);
        }


#endif


        PrevTopNode.BklNode.CTL = NON_VALID_REG;
        PrevBotNode.BklNode.CTL = NON_VALID_REG;
        WriteFunction1(PtViewPort->Prev_p->TopNode_p,&PrevTopNode,Layer);
        WriteFunction2(PtViewPort->Prev_p->BotNode_p,&PrevBotNode,Layer);
    }

     return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stlayer_UpdateNodeGeneric
Description : Update Node : Enabled/Disabled - BKL/GDP/Cursor
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_UpdateNodeGeneric(stlayer_ViewPortHandle_t VPHandle)
{
    WriteFct_p      WriteFunction1;
    WriteFct_p      WriteFunction2;
    ST_DeviceName_t Empty="\0\0";

    stlayer_ViewPortDescriptor_t *  PtViewPort;
    PtViewPort  = (stlayer_ViewPortDescriptor_t *)VPHandle;

    /* Calculate the good function to write in the nodes */
    if(strcmp(stlayer_context.XContext[PtViewPort->LayerHandle].VTGName, Empty) == 0)
    {
        /* No synchro : affect immediatly to prevent queues overflow */
        if (stlayer_context.XContext[PtViewPort->LayerHandle].LayerType
                == STLAYER_GAMMA_CURSOR)
        {
            WriteFunction1 = stlayer_ImmediateRegWrite;
            WriteFunction2 = stlayer_ImmediateRegWrite;
        }
        else
        {
            WriteFunction1 = stlayer_ImmediateMemWrite;
            WriteFunction2 = stlayer_ImmediateMemWrite;
        }
    }
    else
    {
        WriteFunction1 = stlayer_DifferedToTopWrite;
        WriteFunction2 = stlayer_DifferedToBotWrite;
    }

    /* Calculate the good function to update the nodes */
    switch (stlayer_context.XContext[PtViewPort->LayerHandle].LayerType)
    {
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
        case STLAYER_GAMMA_BKL:
            /* BKL And GDP use the same update */
            stlayer_UpdateBKLNode(VPHandle,WriteFunction1,WriteFunction2);
            break;

        case STLAYER_GAMMA_CURSOR:
            /* Cursor Node Update */
            stlayer_UpdateCURNode(VPHandle,WriteFunction1,WriteFunction2);
            break;

        default:
            /* req for no warning */
            break;
    }
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : stlayer_EnableCursor
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_EnableCursor(stlayer_ViewPortHandle_t VPHandle)
{
    stlayer_ViewPortDescriptor_t *  PtViewPort;

    /* Update the ViewPort */
    PtViewPort  = (stlayer_ViewPortDescriptor_t *)VPHandle;
    PtViewPort->Enabled = TRUE;
    stlayer_context.XContext[PtViewPort->LayerHandle].LinkedViewPorts
                = PtViewPort;
    /* Update the node */
    stlayer_UpdateNodeGeneric(VPHandle);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stlayer_DisableCursor
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_DisableCursor(stlayer_ViewPortHandle_t VPHandle)
{
    stlayer_ViewPortDescriptor_t *  PtViewPort;
    /* Update the viewport */
    PtViewPort  = (stlayer_ViewPortDescriptor_t *)VPHandle;
    PtViewPort->Enabled = FALSE;
    stlayer_context.XContext[PtViewPort->LayerHandle].LinkedViewPorts
                = 0;
    /* Update the node */
    stlayer_UpdateNodeGeneric(VPHandle);
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stlayer_UpdateCURNode
Description : Update the hw cursor register (Top Node is mapped on reg)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stlayer_UpdateCURNode(stlayer_ViewPortHandle_t VPHandle,
                                        WriteFct_p WriteFunction1,
                                        WriteFct_p WriteFunction2 )
{
    stlayer_ViewPortDescriptor_t *  PtViewPort;
    U32                             Value,LineOffset;
    STLAYER_Handle_t                LayerHandle;
    stlayer_Node_t                  TopNode;
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    unsigned int                    uMemoryStatus;
    int                             PaletteSize=0;
    ST_ErrorCode_t                  RetError = ST_NO_ERROR;
    STGXOBJ_PaletteAllocParams_t    PaletteParams;
#endif


    UNUSED_PARAMETER(WriteFunction2);

    PtViewPort  = (stlayer_ViewPortDescriptor_t *)VPHandle;
    LayerHandle = PtViewPort->LayerHandle;
    /* SIZE Register */
    Value =  PtViewPort->ClippedIn.Width
        | ( PtViewPort->ClippedIn.Height << 16);
    TopNode.CurNode.SIZE = Value;

    /* VPO Register */
    Value =( PtViewPort->ClippedOut.PositionX + XOFFSET       )      /* X */
         | ( (PtViewPort->ClippedOut.PositionY + YOFFSET) << 16);   /* Y */
    TopNode.CurNode.VPO = Value;

    if((PtViewPort->Enabled) && (!(PtViewPort->TotalClipped)))
    {
        /* AWS Register */      /* upper left */
        TopNode.CurNode.AWS =  Value;

        /* AWE Regsiter */      /* bottom right */
        Value = (PtViewPort->ClippedOut.PositionX + XOFFSET
              + PtViewPort->ClippedIn.Width)
              | ((PtViewPort->ClippedOut.PositionY + YOFFSET
                 + PtViewPort->ClippedIn.Height) << 16);
        TopNode.CurNode.AWE =   Value;
    }
    else
    {
        /* dummy values are used to disable the cursor */
        /* AWS Register */      /* upper left */
        Value = (PtViewPort->ClippedOut.PositionX + XOFFSET + 2
              + PtViewPort->ClippedIn.Width)
              | ((PtViewPort->ClippedOut.PositionY + YOFFSET + 2
                 + PtViewPort->ClippedIn.Height) << 16);
        TopNode.CurNode.AWS =   Value;

        /* AWE Regsiter */      /* bottom right */
        Value = (PtViewPort->ClippedOut.PositionX + XOFFSET + 4
              + PtViewPort->ClippedIn.Width)
              | ((PtViewPort->ClippedOut.PositionY + YOFFSET + 4
                 + PtViewPort->ClippedIn.Height) << 16);
        TopNode.CurNode.AWE =   Value;
    }

    /* CTL Register */
    Value =  1 << 1; /* ena cursuor CLUT refresh */
    TopNode.CurNode.CTL = Value;

    /* PML Register */
    LineOffset = PtViewPort->ClippedIn.PositionX;
    Value = (U32)BitmapAdd(PtViewPort->Bitmap.Data1_p)
         + (PtViewPort->Bitmap.Pitch * PtViewPort->ClippedIn.PositionY)
         + LineOffset;            /* add */

    TopNode.CurNode.PML = Value;
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    uMemoryStatus = STMES_IsMemorySecure((void *)TopNode.CurNode.PML, (U32)(PtViewPort->Bitmap.Size1), 0);
    if(uMemoryStatus == SECURE_REGION)
    {
        TopNode.CurNode.PML = (U32)(TopNode.CurNode.PML | STLAYER_SECURE_ON);
    }
    else
    {
        TopNode.CurNode.PML = (U32)(TopNode.CurNode.PML & STLAYER_SECURE_MASK);
    }
#endif


    /* PMP Register */
    Value = PtViewPort->Bitmap.Pitch;                        /* pitch */
    TopNode.CurNode.PMP = Value;

    /* CML Register */
    Value =  (U32)BitmapAdd(PtViewPort->Palette.Data_p);       /* add */
    TopNode.CurNode.CML = Value;

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    RetError = LAYERGFX_GetPaletteAllocParams(LayerHandle,&PtViewPort->Palette,&PaletteParams);
    PaletteSize = PaletteParams.AllocBlockParams.Size;

    uMemoryStatus = STMES_IsMemorySecure((void *)TopNode.CurNode.CML, (U32)(PaletteSize), 0);
    if(uMemoryStatus == SECURE_REGION)
        {
        TopNode.CurNode.CML = (U32)(TopNode.CurNode.CML | STLAYER_SECURE_ON);
        }
        else
        {
        TopNode.CurNode.CML = (U32)(TopNode.CurNode.CML & STLAYER_SECURE_MASK);
        }

#endif
    /* PKZ Register */
    /* TopNode.CurNode.PKZ = 0;*/

    WriteFunction1(PtViewPort->TopNode_p,&TopNode,LayerHandle);

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        :  stlayer_UpdateCTLReg
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 stlayer_UpdateCTLReg(stlayer_ViewPortDescriptor_t * PtViewPort)
{
    STLAYER_Handle_t                LayerHandle;

    U32 HResize=0;
    U32 WaitVsync=0;
    U32 ColorKeyConfig=0;
    U32 VResize=0;
    U32 ColorType=0;
    U32 ChromaFormat=0;
    U32 Value=0;
    U32 AlphaRange=0;
    U32 ColoConversionMode=0;


    #if defined (ST_7109)  || defined (ST_7200)
        U32 FFilter=0;
        U32 CFill=0;
        U32 EnableClut=0;
        U32 FForVF=0;
        U32 CutId=0;
    #endif




   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif



    LayerHandle = PtViewPort->LayerHandle;

    /* Calculate the WaitVSync field */
    if ((PtViewPort->Enabled) && (!(PtViewPort->TotalClipped))
        && (PtViewPort->Next_p       /* VP is the last in the linked list */
            == stlayer_context.XContext[LayerHandle].LinkedViewPorts))
    {
        WaitVsync = 1;
    }
    else
    {
        WaitVsync = 0;
    }

    /* Calculate the HResize */
    if (PtViewPort->InputRect.Width != PtViewPort->OutputRect.Width)
    {
        HResize = TRUE;
    }
    else
    {
        HResize = FALSE;
    }


    /* Calculate the VResize */
    if ((PtViewPort->InputRect.Height != PtViewPort->OutputRect.Height)
            ||(stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_FILTER)) /* flicker filter : enable VSCR */
    {
        VResize = TRUE;
    }
    else
    {
        VResize = FALSE;
    }


    /* Calculate the ColorKeyConfig field */
    ColorKeyConfig = 0;
    switch (PtViewPort->ColorKeyValue.Type)
    {
        /* Color key uses 8bits to define the color */
        case STGXOBJ_COLOR_KEY_TYPE_RGB888:
            if (PtViewPort->ColorKeyValue.Value.RGB888.REnable)
            {
                if (PtViewPort->ColorKeyValue.Value.RGB888.ROut)
                    ColorKeyConfig |= RCrOutRange;
                else
                    ColorKeyConfig |= RCrInRange;
            }
            if (PtViewPort->ColorKeyValue.Value.RGB888.GEnable)
            {
                if (PtViewPort->ColorKeyValue.Value.RGB888.GOut)
                    ColorKeyConfig |= GYOutRange;
                else
                    ColorKeyConfig |= GYInRange;
            }
            if (PtViewPort->ColorKeyValue.Value.RGB888.BEnable)
            {
                if (PtViewPort->ColorKeyValue.Value.RGB888.BOut)
                    ColorKeyConfig |= BCbOutRange;
                else
                    ColorKeyConfig |= BCbInRange;
            }
            break;
        case STGXOBJ_COLOR_KEY_TYPE_YCbCr888_UNSIGNED:
            if (PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.CrEnable)
            {
                if (PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.CrOut)
                    ColorKeyConfig |= RCrOutRange;
                else
                    ColorKeyConfig |= RCrInRange;
            }
            if (PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.YEnable)
            {
                if (PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.YOut)
                    ColorKeyConfig |= GYOutRange;
                else
                    ColorKeyConfig |= GYInRange;
            }
            if (PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.CbEnable)
            {
                if (PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.CbOut)
                    ColorKeyConfig |= BCbOutRange;
                else
                    ColorKeyConfig |= BCbInRange;
            }
            break;

        default:
            break;
    }

    /* Calculate the Chromat format and color type */
    stlayer_ExtractColorType(PtViewPort->Bitmap.ColorType,&ColorType,
                &ChromaFormat);

    if((PtViewPort->Bitmap.ColorType ==  STGXOBJ_COLOR_TYPE_ARGB8888_255)
     ||(PtViewPort->Bitmap.ColorType ==  STGXOBJ_COLOR_TYPE_ARGB8565_255)
     ||(PtViewPort->Bitmap.ColorType ==  STGXOBJ_COLOR_TYPE_ALPHA8_255))
    {
        AlphaRange = 1;
    }
    else
    {
        AlphaRange = 0;
    }


#if defined (ST_7109) || defined (ST_7200)

    #if defined (ST_7109)

    if (CutId >= 0xC0)
    {
    #endif

    /* Calculate the VResize */
    if (PtViewPort->InputRect.Height != PtViewPort->OutputRect.Height) /*  enable VSCR */
    {
        VResize = TRUE;
    }
    else
    {
        VResize = FALSE;
    }

    /*Update or not CLUT*/
    if((PtViewPort->Bitmap.ColorType ==  STGXOBJ_COLOR_TYPE_CLUT8)
     ||(PtViewPort->Bitmap.ColorType ==  STGXOBJ_COLOR_TYPE_ACLUT88))
    {
        EnableClut = 1;
    }
    else
    {
        EnableClut = 0;
    }


    /*Update or not FlickerFilter*/
    if(PtViewPort->FFilterEnabled==  TRUE)
        {
        FFilter = 1;
    }
    else
    {
        FFilter = 0;
    }

   /*Update or not FlickerFilter*/
    if(PtViewPort->ColorFillEnabled ==  TRUE)
        {
        CFill = 1;
        }
        else
        {
         CFill = 0;
    }

    /*Update or not FlickerFilter*/
    if ((FFilter ==  1) ||( VResize == 1))
        {
        FForVF  = 1;
    }
    else
    {
        FForVF  = 0;
    }

#if defined (ST_7109)
   }
#endif

#endif                                  /* #if defined (ST_7109) || defined (ST_7200) */

if (PtViewPort->Bitmap.ColorSpaceConversion ==STGXOBJ_ITU_R_BT709)
        {
            ColoConversionMode = 1;
        }
        else
        {
            ColoConversionMode = 0;
        }


    /* CTL Register */
    if(stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_ALPHA)
    {
        Value = ColorType                                      /* data format */
        | ( AlphaRange                            <<  5)       /* alpha range */
        | ( (HResize ? 1 : 0)                     << 10)       /* enable HSRC */
        | ( (VResize ? 1 : 0)                     << 11)       /* enable VSRC */
        | ( 0                                     << 24)    /* A1 bytes start */
        | ( 0                                     << 27)    /* A1 bytes order */
        | ( 0                                     << 29)     /* reverse alpha */
        | ( (HResize ? 1 : 0)                     << 30)   /* update H Filter */
        | ( WaitVsync                             << 31);       /* wait Vsync */
    }
    else
    {
        Value = ColorType                                      /* data format */
        | ( AlphaRange                            <<  5)       /* alpha range */
        | ( (HResize ? 1 : 0)                     << 10)       /* enable HSRC */
        | ( (VResize ? 1 : 0)                     << 11)       /* enable VSRC */
        | ( (PtViewPort->BorderAlphaOn ? 3 : 0)   << 12)  /* enable alphaVHBr */
        | ( (PtViewPort->ColorKeyEnabled ? 1 : 0) << 14)     /* ena color key */
        | ( ColorKeyConfig                        << 16)  /* color key config */
        | ( (PtViewPort->Bitmap.BigNotLittle ? 1:0)       << 23)/* big endian */
        | ( (PtViewPort->Bitmap.PreMultipliedColor ? 1:0) << 24)/* premul fmt */
        | ( ColoConversionMode << 25)       /* color sel */
        | ( ChromaFormat                          << 26)     /* chroma format */
      /*  | ( (EnableClut ? 1 : 0)                  << 27)*/       /* ena cut Update */  /*to be verifed when this must be done */
      /*  | ( (FForVF ? 1 : 0)                      << 28)*/   /* enable H Filter */
        | ( 0                                     << 29)      /* LSB stuffing */
        | ( (HResize ? 1 : 0)                     << 30)   /* enable H Filter */
        /*| ( (FFilter ? 1 : 0)  << 9)*/         /* ena FF filter */
        | ( WaitVsync                             << 31);       /* wait Vsync */

    }


#if defined (ST_7109) || defined (ST_7200)

    #if defined (ST_7109)
    if (CutId >= 0xC0)
    {
    #endif

        Value = Value                                         /* orig */
        | ( (EnableClut ? 1 : 0) << 27)                       /* ena cut Update */  /*to be verifed when this must be done */
        | ( (FForVF ? 1 : 0)                      << 28)
        /*| ( (1)                      << 22)*/
        | ( (FFilter ? 1 : 0)  << 9)
        | ( (CFill ? 1 : 0)  << 8);                          /* Enable the color fill */

  #if defined (ST_7109)
   }
  #endif

#endif                                  /* #if defined (ST_7109) || defined (ST_7200) */

    return(Value);
}

/*******************************************************************************
Name        : stlayer_UpdateBKLNode
Description : Also used for a GDP Node !!
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stlayer_UpdateBKLNode(stlayer_ViewPortHandle_t VPHandle,
                                        WriteFct_p WriteFunction1,
                                        WriteFct_p WriteFunction2 )
{
    stlayer_ViewPortDescriptor_t *  PtViewPort;
    STLAYER_Handle_t                LayerHandle;
    U32                             LineOffset,Zoom;
    U32                             Value, PixlocTop = 0,PixlocBot = 0,Pitch,HeightTop = 0,HeightBot = 0;
    U32                             FirstLine,SecondLine;
    U32                             SubPos,Inc;
    stlayer_Node_t                  TopNode, BotNode;
    U32                             Ref,Ref1;

#if defined (ST_7109) || defined (ST_7200)
    U32 SkipLine;
#endif

#if defined (ST_7109)
    U32 CutId;
#endif

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    unsigned int uMemoryStatus;
    int                             PaletteSize=0;
    ST_ErrorCode_t                  RetError = ST_NO_ERROR;
    STGXOBJ_PaletteAllocParams_t    PaletteParams;
#endif


#if defined (ST_7109)
    CutId = ST_GetCutRevision();
#endif

    /* shortcuts */
    PtViewPort  = (stlayer_ViewPortDescriptor_t *)VPHandle;
    LayerHandle = PtViewPort->LayerHandle;

    /* HFilters Values */



    /* CTL Register */
    Value = stlayer_UpdateCTLReg(PtViewPort);
    TopNode.BklNode.CTL = Value;
    BotNode.BklNode.CTL = Value;

    /* AGC Register */
    Value =     PtViewPort->Alpha.A0                 /* global alpha reg a0 */
            | ( PtViewPort->Alpha.A1 << 8)           /* global alpha reg a1 */
            | ( PtViewPort->Gain.GainLevel << 16)    /* gain reg            */
            | ( PtViewPort->Gain.BlackLevel << 24);  /* black level reg     */
    TopNode.BklNode.AGC = Value;
    BotNode.BklNode.AGC = Value;

    if(PtViewPort->ColorKeyValue.Type == STGXOBJ_COLOR_KEY_TYPE_RGB888)
    {
        /* KEY1 Register */
        Value =  PtViewPort->ColorKeyValue.Value.RGB888.BMin        /* min B */
            | ( PtViewPort->ColorKeyValue.Value.RGB888.GMin << 8)   /* min G */
            | ( PtViewPort->ColorKeyValue.Value.RGB888.RMin << 16); /* min R */
        TopNode.BklNode.KEY1 = Value;
        BotNode.BklNode.KEY1 = Value;

        /* KEY2 Register */
        Value =  PtViewPort->ColorKeyValue.Value.RGB888.BMax         /* max B */
            | (PtViewPort->ColorKeyValue.Value.RGB888.GMax  << 8)    /* max G */
            | (PtViewPort->ColorKeyValue.Value.RGB888.RMax  << 16);  /* max R */
        TopNode.BklNode.KEY2 = Value;
        BotNode.BklNode.KEY2 = Value;
    }
    else
    {
        /* KEY1 Register */
        Value =  PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.CbMin
            | ( PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.YMin  << 8)
            | ( PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.CrMin << 16);
        TopNode.BklNode.KEY1 = Value;
        BotNode.BklNode.KEY1 = Value;

        /* KEY2 Register */
        Value =  PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.CbMax
            | (PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.YMax  << 8)
            | (PtViewPort->ColorKeyValue.Value.UnsignedYCbCr888.CrMax << 16);
        TopNode.BklNode.KEY2 = Value;
        BotNode.BklNode.KEY2 = Value;
    }

    /* HFP Register (GDP-Filter-alpha Only) */
    /* first table by default */
    if(stlayer_context.XContext[LayerHandle].LayerType != STLAYER_GAMMA_BKL)
    {
        Zoom =(PtViewPort->ClippedIn.Width * 1024) /PtViewPort->ClippedOut.Width;  /* should be Inc*/

        /* default table (zoom>x1.1) */
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    uMemoryStatus = STMES_IsMemorySecure((void *)DeviceAdd(stlayer_context.XContext[LayerHandle].FilterCoefs), \
                            (U32)(NB_HSRC_FILTERS* (NB_HSRC_COEFFS + FILTERS_ALIGN)), 0);
    if(uMemoryStatus == SECURE_REGION)
        {
            Value = (U32)(DeviceAdd(stlayer_context.XContext[LayerHandle].FilterCoefs)| STLAYER_SECURE_ON);
        }
        else
        {
            Value = (U32)(DeviceAdd(stlayer_context.XContext[LayerHandle].FilterCoefs)& STLAYER_SECURE_MASK);
         }

#else
       Value = DeviceAdd(stlayer_context.XContext[LayerHandle].FilterCoefs);
#endif

        if (Zoom < 1024)
        {
            /* next table */
            Value += 0;
        }
        else if(Zoom < 1331)
        {
            /* next table */
            Value += (NB_HSRC_COEFFS + FILTERS_ALIGN)*1;
        }
        else if(Zoom < 1433)
        {
            /* next table */
            Value += (NB_HSRC_COEFFS + FILTERS_ALIGN)*2;
        }
        else if(Zoom < 1536)
        {
            /* next table */
            Value += (NB_HSRC_COEFFS + FILTERS_ALIGN)*3;
        }
        else if(Zoom < 2048)
        {
            /* next table */
            Value += (NB_HSRC_COEFFS + FILTERS_ALIGN)*4;
        }
        else if(Zoom < 3072)
        {
            /* next table */
            Value += (NB_HSRC_COEFFS + FILTERS_ALIGN)*5;
        }
      else if(Zoom < 4096)
        {
            /* next table */
            Value += (NB_HSRC_COEFFS + FILTERS_ALIGN)*6;
        }
      else if(Zoom < 5120)
        {
            /* next table */
            Value += (NB_HSRC_COEFFS + FILTERS_ALIGN)*7;
        }
      else
        {
            /* next table */
            Value += (NB_HSRC_COEFFS + FILTERS_ALIGN)*8;
        }

    }
    else
    {
        Value = 0;
    }

    TopNode.BklNode.HFP = Value;
    BotNode.BklNode.HFP = Value;

        /* VSRC Register */
    Ref = (PtViewPort->ClippedIn.Width) * 256/ (PtViewPort->ClippedOut.Width);
    Ref = Ref*10;
    Ref1 = (PtViewPort->ClippedIn.Width) * 2560/ (PtViewPort->ClippedOut.Width);

    Inc = ((PtViewPort->ClippedIn.Width ) * 256/ (PtViewPort->ClippedOut.Width ));

    /* Update Inc to the neasrest Integer value to the zoom factor */
    if((Ref1 - Ref) > 5)
{
    Inc++;

}

    Value = (   Inc                                  /* increment */
                | ( 0 << 16)                         /* offset    */
                | ( 1 << 24));                       /* filter enable */


    TopNode.BklNode.HSRC = Value;
    BotNode.BklNode.HSRC = Value;


    /* VSRC Register (GDP-filter-alpha Only) */
    Value =  (PtViewPort->ClippedIn.Height) * 256
                   / (PtViewPort->ClippedOut.Height); /* increment */
    Value = Value   | ( 0 << 16)                          /* offset */
                    | ( 1 << 24);                         /* filter */
    TopNode.BklNode.VSRC = Value;
    BotNode.BklNode.VSRC = Value;

    Pitch = PtViewPort->Bitmap.Pitch;        /* Pitch  */

#if defined (ST_7109) || defined (ST_7200)

    #if defined (ST_7109)

    if (CutId >= 0xC0)
    {
    #endif

    /* VFP Register (GDP-Filter) */


    if(PtViewPort->FFilterEnabled==TRUE)
            {
            Value = (U32)(DeviceAdd(stlayer_context.XContext[LayerHandle].FFilterCoefs));
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
            uMemoryStatus = STMES_IsMemorySecure((void *)DeviceAdd(stlayer_context.XContext[LayerHandle].FFilterCoefs), \
            (U32)(NB_FF_FILTERS * (NB_FF_COEFFS + FF_FILTERS_ALIGN)), 0);
            if(uMemoryStatus == SECURE_REGION)
                 { Value = (U32)(DeviceAdd(stlayer_context.XContext[LayerHandle].FFilterCoefs)| STLAYER_SECURE_ON); }
            else
                { Value = (U32)(DeviceAdd(stlayer_context.XContext[LayerHandle].FFilterCoefs)& STLAYER_SECURE_MASK);}
#endif
            }
            else
            {

        Zoom =(PtViewPort->ClippedIn.Height * 256) /PtViewPort->ClippedOut.Height;  /* should be Inc*/

        /* default table (zoom>x1.1) */

        Value = DeviceAdd(stlayer_context.XContext[LayerHandle].VFilterCoefs);


#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    uMemoryStatus = STMES_IsMemorySecure((void *)DeviceAdd(stlayer_context.XContext[LayerHandle].VFilterCoefs), \
                            (U32)(NB_VSRC_FILTERS* (NB_VSRC_COEFFS + VFILTERS_ALIGN)), 0);
            if(uMemoryStatus == SECURE_REGION)
                 { Value = (U32)(DeviceAdd(stlayer_context.XContext[LayerHandle].VFilterCoefs)| STLAYER_SECURE_ON); }
            else
                { Value = (U32)(DeviceAdd(stlayer_context.XContext[LayerHandle].VFilterCoefs)& STLAYER_SECURE_MASK);}
#endif



        if (Zoom <= 256)
        {
            Value +=0;
        }
        else if(Zoom <= 384)
        {
            /* next table */
            Value +=(NB_VSRC_COEFFS + VFILTERS_ALIGN)*1;
        }
        else if(Zoom <= 512)
        {
            /* next table */
            Value +=(NB_VSRC_COEFFS + VFILTERS_ALIGN)*2;
        }
        else if(Zoom <= 768)
        {
            /* next table */
            Value +=(NB_VSRC_COEFFS + VFILTERS_ALIGN)*3;
        }
         else if(Zoom <= 1024)
        {
            /* next table */
            Value +=(NB_VSRC_COEFFS + VFILTERS_ALIGN)*4;
        }
         else if(Zoom > 1024)
        {
            /* next table */
            Value +=(NB_VSRC_COEFFS + VFILTERS_ALIGN)*5;
        }

        /* Added for GNBvd63648 */
        if (Zoom > 512)
        {
            SkipLine =  (Zoom/512)+1;
            PtViewPort->ClippedIn.Height = (PtViewPort->ClippedIn.Height/SkipLine);
            Pitch= Pitch *   SkipLine;
        }

     }
    TopNode.BklNode.VFP =  Value;
    BotNode.BklNode.VFP =  Value;

    #if defined (ST_7109)
    }
    #endif

#endif                                  /* #if defined (ST_7109) || defined (ST_7200)*/

    /* VPS Register */
    Value =  (PtViewPort->ClippedOut.PositionX + XOFFSET
           + PtViewPort->ClippedOut.Width - 1 )                /* X */
          | ( PtViewPort->ClippedOut.PositionY  + YOFFSET
           + PtViewPort->ClippedOut.Height - 1  ) << 16;       /* Y */
    TopNode.BklNode.VPS = Value;
    BotNode.BklNode.VPS = Value;

    /* VPO Register */
    Value = ( PtViewPort->ClippedOut.PositionX + XOFFSET       )    /* X */
         | (( PtViewPort->ClippedOut.PositionY + YOFFSET) << 16);   /* Y */
    TopNode.BklNode.VPO = Value;
    BotNode.BklNode.VPO = Value;

    /* PML , SIZE and PMP Registers */
    /* Calculate LineOffset */

    switch(PtViewPort->Bitmap.ColorType)
    {
        case STGXOBJ_COLOR_TYPE_RGB565:
        case STGXOBJ_COLOR_TYPE_ARGB1555:
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            LineOffset = PtViewPort->ClippedIn.PositionX * 2;
                break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            LineOffset = (PtViewPort->ClippedIn.PositionX * 2)
                + ((PtViewPort->ClippedIn.PositionX & 0x1) ? 1 : 0);
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
        case STGXOBJ_COLOR_TYPE_RGB888:
        case STGXOBJ_COLOR_TYPE_ARGB8565:
        case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            LineOffset = PtViewPort->ClippedIn.PositionX * 3;
                break;
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
            LineOffset = PtViewPort->ClippedIn.PositionX * 4;
                break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            LineOffset = PtViewPort->ClippedIn.PositionX / 8;
                break;
        case STGXOBJ_COLOR_TYPE_ALPHA8:
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
            LineOffset = PtViewPort->ClippedIn.PositionX ;
                break;

        case STGXOBJ_COLOR_TYPE_ACLUT88:
        case   STGXOBJ_COLOR_TYPE_CLUT8:
            LineOffset = PtViewPort->ClippedIn.PositionX ;

            break;


        default:
                return(ST_ERROR_BAD_PARAMETER);
            break;
    }
    switch(stlayer_context.XContext[LayerHandle].LayerType)
    {
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_FILTER:
        /* top values can be different as bot values                    */
        /*      case 1 : prog bitmap       -> interlaced layer          */
        /*      case 2 : prog bitmap       -> prog layer                */
        /*      case 3 : top-bottom bitmap -> interlaced layer          */
        /*      case 4 : top-bottom bitmap -> prog layer : can't match  */

        /* case 1 : prog bitmap -> interlaced layer */
        /*------------------------------------------*/
        if((stlayer_context.XContext[LayerHandle].LayerParams.ScanType
            == STGXOBJ_INTERLACED_SCAN)
        &&(PtViewPort->Bitmap.BitmapType
            == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE))
        {
            FirstLine       = (U32)BitmapAdd(PtViewPort->Bitmap.Data1_p)
                +(PtViewPort->Bitmap.Pitch * PtViewPort->ClippedIn.PositionY)
                +  LineOffset;
            Pitch = Pitch * 2;
            if(PtViewPort->ClippedIn.Height == PtViewPort->ClippedOut.Height)
            {
                /* no resize , top node scans top lines ...*/
                if((PtViewPort->ClippedOut.PositionY + YOFFSET)%2== 0)/* Even */
                {
                    PixlocTop = FirstLine;
                    PixlocBot = FirstLine + Pitch/2;
                    HeightBot = PtViewPort->ClippedIn.Height/ 2;
                    HeightTop = HeightBot + PtViewPort->ClippedIn.Height % 2;

                }
                else                                         /* Odd position */
                {
                    PixlocBot = FirstLine;
                    PixlocTop = FirstLine + Pitch/2;
                    HeightTop = PtViewPort->ClippedIn.Height/ 2;
                    HeightBot = HeightTop + PtViewPort->ClippedIn.Height % 2;
                }
            }
            else
            {
                /* resize : all lines must be scaned by each node */
                PixlocTop       = FirstLine;
                PixlocBot       = FirstLine;
                HeightBot       = PtViewPort->ClippedIn.Height;
                HeightTop       = PtViewPort->ClippedIn.Height;
                /* VSRC Register (GDP-filter-alpha Only) */
                Inc  =(256*(PtViewPort->ClippedIn.Height )) / (PtViewPort->ClippedOut.Height) ;

                Value     = Inc             /* sample rate  */
                         | ( 0 << 16)       /* init phase   */
                         | ( 0 << 25)       /* init phase   */
                         | ( 1 << 24);      /* filtre ON    */
                if((PtViewPort->ClippedOut.PositionY + YOFFSET)%2== 0)/* Even */
                {
                    TopNode.BklNode.VSRC = Value;
                    SubPos    = ((Inc/2)%(256))>>5;
                    Value     = Inc             /* sample rate */
                             | ( SubPos << 16)  /* init phase  */
                             | ( 1      << 24); /* filtre ON   */
                    BotNode.BklNode.VSRC = Value;
                    PixlocBot += (Inc/2)/256 * Pitch;
                    HeightBot -= (Inc/2)/256;
                }
                else                                         /* Odd position */
                {
                    BotNode.BklNode.VSRC = Value;
                    SubPos    = ((Inc/2)%(256))>>5;
                    Value     = Inc             /* sample rate */
                             | ( SubPos << 16)  /* init phase  */
                             | ( 1      << 24); /* filtre ON   */
                    TopNode.BklNode.VSRC = Value;
                    PixlocTop += (Inc/2)/256 * Pitch;
                    HeightTop -= (Inc/2)/256;
                }
            }
        }
        /* case 2 : prog bitmap -> prog layer */
        /*------------------------------------*/
        else if ((stlayer_context.XContext[LayerHandle].LayerParams.ScanType
            == STGXOBJ_PROGRESSIVE_SCAN)
        &&(PtViewPort->Bitmap.BitmapType
            == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE))
        {
            HeightTop =  PtViewPort->ClippedIn.Height;  /* Height */
            PixlocTop = (U32)BitmapAdd(PtViewPort->Bitmap.Data1_p)
                +(PtViewPort->Bitmap.Pitch * PtViewPort->ClippedIn.PositionY)
                +  LineOffset;
            HeightBot = HeightTop;
            PixlocBot = PixlocTop;
        }
        /* case 3 : top bottom bitmap -> interlaced layer */
        /*------------------------------------------------*/
        else if((stlayer_context.XContext[LayerHandle].LayerParams.ScanType
            == STGXOBJ_INTERLACED_SCAN)
        &&(PtViewPort->Bitmap.BitmapType
            == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM))
        {
            HeightBot = PtViewPort->ClippedIn.Height / 2;
            HeightTop = HeightBot;
            if (PtViewPort->ClippedIn.PositionY % 2 == 0)
            {
                FirstLine = (U32)BitmapAdd(PtViewPort->Bitmap.Data1_p)
                    + Pitch * PtViewPort->ClippedIn.PositionY / 2
                    + LineOffset;
                SecondLine = (U32)BitmapAdd(PtViewPort->Bitmap.Data2_p)
                    + Pitch * PtViewPort->ClippedIn.PositionY / 2
                    + LineOffset;
            }
            else
            {
                FirstLine = (U32)BitmapAdd(PtViewPort->Bitmap.Data2_p)
                    + Pitch * PtViewPort->ClippedIn.PositionY / 2
                    + LineOffset;
                SecondLine = (U32)BitmapAdd(PtViewPort->Bitmap.Data1_p)
                    + Pitch * (PtViewPort->ClippedIn.PositionY / 2 + 1)
                    + LineOffset;
            }
            if ((PtViewPort->ClippedOut.PositionY + YOFFSET )% 2 == 0)
            {
                HeightTop += PtViewPort->ClippedIn.Height % 2;
                PixlocTop = FirstLine;
                PixlocBot = SecondLine;
            }
            else
            {
                HeightBot += PtViewPort->ClippedIn.Height % 2;
                PixlocBot = FirstLine;
                PixlocTop = SecondLine;
            }
        }
        /* case 4 : interlaced bitmap -> prog layer */
        /*------------------------------------------*/
        else
        {
            /* Dummy values because non supported */
            PixlocBot = 0;
            HeightBot = 0;
            return(ST_ERROR_BAD_PARAMETER);
        }
        break; /* end case GDP-Alpha */

        case STLAYER_GAMMA_BKL:
        /* Layer is a BKL one: top values = bot values */
        /* Only progressiv bitmap are supported */
        PixlocTop = (U32)BitmapAdd(PtViewPort->Bitmap.Data1_p)
                    + Pitch * PtViewPort->ClippedIn.PositionY
                    + LineOffset;
        PixlocBot = PixlocTop;
        HeightTop = PtViewPort->ClippedIn.Height;
        HeightBot = HeightTop;
        break; /* end case BKL */

        default:
        /* Should not get in here */
        break;
    } /* end switch layer type */


    TopNode.BklNode.PMP  = Pitch;
    BotNode.BklNode.PMP  = Pitch;
    TopNode.BklNode.PML  = PixlocTop;
    BotNode.BklNode.PML  = PixlocBot;
#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    uMemoryStatus = STMES_IsMemorySecure((void *)TopNode.BklNode.PML, (U32)(PtViewPort->Bitmap.Size1), 0);
    if(uMemoryStatus == SECURE_REGION)
        {
            TopNode.BklNode.PML = (U32)(TopNode.BklNode.PML | STLAYER_SECURE_ON);
        }
        else
        {
            TopNode.BklNode.PML = (U32)(TopNode.BklNode.PML & STLAYER_SECURE_MASK);
        }

    uMemoryStatus = STMES_IsMemorySecure((void *)BotNode.BklNode.PML , (U32)(32), 0);
    if(uMemoryStatus == SECURE_REGION)
        {
        BotNode.BklNode.PML = (U32)(BotNode.BklNode.PML | STLAYER_SECURE_ON);
        }
        else
        {
        BotNode.BklNode.PML = (U32)(BotNode.BklNode.PML & STLAYER_SECURE_MASK);
        }

#endif

 if(PtViewPort->FFilterEnabled==TRUE)
 {
     BotNode.BklNode.PML  = TopNode.BklNode.PML;
 }

    TopNode.BklNode.SIZE = PtViewPort->ClippedIn.Width | ( (HeightTop) << 16);
    BotNode.BklNode.SIZE = PtViewPort->ClippedIn.Width | ( (HeightBot) << 16);

/*----12/16/2006 11:36AM----Added by RS for 7200 porting----*/
#if defined (ST_7109) || defined (ST_7200)

    #if defined (ST_7109)

    if (CutId >= 0xC0)
    {
    #endif

  /* Add support du Clut 8 */
    if((PtViewPort->Bitmap.ColorType ==  STGXOBJ_COLOR_TYPE_CLUT8)
     ||(PtViewPort->Bitmap.ColorType ==  STGXOBJ_COLOR_TYPE_ACLUT88))
    {

    /* CML Register */
    Value =  (U32)BitmapAdd(PtViewPort->Palette.Data_p);       /* add */


#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    RetError = LAYERGFX_GetPaletteAllocParams(LayerHandle,&PtViewPort->Palette,&PaletteParams);
    PaletteSize = PaletteParams.AllocBlockParams.Size;

    uMemoryStatus = STMES_IsMemorySecure((void *)Value, (U32)(PaletteSize), 0);
    if(uMemoryStatus == SECURE_REGION)
            { Value = (U32)(Value | STLAYER_SECURE_ON); }
            else
            { Value = (U32)(Value & STLAYER_SECURE_MASK); }


#endif
    TopNode.BklNode.CML = Value;
    BotNode.BklNode.CML = Value;

    }

   #if defined (ST_7109)
    }
    #endif

#endif                                  /* #if defined (ST_7109) || defined (ST_7200)*/

    /* NVN Register : updated when insert/extract */
    TopNode.BklNode.NVN = NON_VALID_REG;
    BotNode.BklNode.NVN = NON_VALID_REG;

    /* PPT Register (GDP/ALP/FLT Only) */
    Value = ((  0                                   << 0)   /* ignore on mix1 */
            |( (PtViewPort->HideOnMix2 ? 1 : 0)     << 1)   /* ignore on mix2 */
            |(  0                                   << 2)   /* force on mix1 */
            |(  0                                   << 3)   /* force on mix2 */
            |(  PtViewPort->AssociatedToMainLayer   << 4)); /* attach */

    if (stlayer_context.XContext[LayerHandle].LayerType==STLAYER_GAMMA_GDPVBI)
            {
               Value=0x0000000C|Value;
            }

    if(PtViewPort->ColorFillEnabled == TRUE)
    			{
                    /*Value=0xFFFF0000|Value;*/ /* R=255,G=255,B=0 ->yellow */
                  /*  Value=0xFFFF0000|Value; *//* R=255,G=255,B=0 ->yellow */
                    /*Value=(((PtViewPort->ColorFillValue.R & 0xFF) << 24) + ((PtViewPort->ColorFillValue.G & 0xFF) << 16) +((PtViewPort->ColorFillValue.B & 0xFF) << 8))|Value;*/
                     Value=(((PtViewPort->ColorFillValue.R) << 24) + ((PtViewPort->ColorFillValue.G) << 16) +((PtViewPort->ColorFillValue.B & 0xFF) << 8))|Value;

                }
               /* Value=0xFFFF0000|Value; *//* R=255,G=255,B=0 ->yellow */

    TopNode.BklNode.PPT = Value;
    BotNode.BklNode.PPT = Value;

if(PtViewPort->ColorFillEnabled == TRUE)
        {
          TopNode.BklNode.AGC = (TopNode.BklNode.AGC & 0xFFFFFF00) + (PtViewPort->ColorFillValue.Alpha);
          BotNode.BklNode.AGC = (BotNode.BklNode.AGC & 0xFFFFFF00) + (PtViewPort->ColorFillValue.Alpha);

        }

#if defined (ST_7109) || defined (ST_7200)
/*if flicker filter enabler*/
    #if defined (ST_7109)

    if (CutId >= 0xC0)
    {
    #endif

if (PtViewPort->FFilterEnabled==TRUE)
    {

    TopNode.BklNode.PMP  = TopNode.BklNode.PMP/2;
    BotNode.BklNode.PMP  = BotNode.BklNode.PMP/2;


    TopNode.BklNode.SIZE = PtViewPort->ClippedIn.Width | ( (HeightTop*2) << 16);
    BotNode.BklNode.SIZE = PtViewPort->ClippedIn.Width | ( (HeightBot*2) << 16);


    if (PtViewPort->FFilterMode==STLAYER_FLICKER_FILTER_MODE_ADAPTIVE)
            {
               TopNode.BklNode.VSRC= (TopNode.BklNode.VSRC) | 0x2000000;
               BotNode.BklNode.VSRC= (BotNode.BklNode.VSRC) | 0x2000000;

            }

    }

    #if defined (ST_7109)
    }
    #endif

#endif                                  /* #if defined (ST_7109) || defined (ST_7200) */

#if defined(ST_OS21)
 #ifdef DUMP_GDPS_NODES
    stlayer_DumpGDPNode(TopNode,"DUMP NODE_TOP");
    stlayer_DumpGDPNode(BotNode,"DUMP NODE_BOTTOM");
#endif
#endif

    if (stlayer_context.XContext[LayerHandle].LayerType != STLAYER_GAMMA_FILTER)
    {
        WriteFunction1(PtViewPort->TopNode_p,&TopNode,LayerHandle);
        WriteFunction2(PtViewPort->BotNode_p,&BotNode,LayerHandle);
    }
    else /* flicker filter  */
    {
        TopNode.BklNode.VSRC |= ( 4 << 16)              /* offset */
    	                     |  ( 1 << 24);             /* filter */
    	BotNode.BklNode.VSRC |= ( 4 << 16)              /* offset */
     	               	     |  ( 1 << 24);             /* filter */
        if(((TopNode.BklNode.VPO) >> 16)%2 == 0)/* Even */   /* add YDO offset */
        {
            BotNode.BklNode.VPO = BotNode.BklNode.VPO + (2 << 16);
        }
        else
        {
            TopNode.BklNode.VPO = TopNode.BklNode.VPO + (2 << 16);
        }

       /* invert top/bot */
        WriteFunction1(PtViewPort->TopNode_p,&BotNode,LayerHandle);
        WriteFunction2(PtViewPort->BotNode_p,&TopNode,LayerHandle);
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stlayer_DifferedToTopWrite
Description : The affectation will be done by the hardware manager task
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_DifferedToTopWrite( stlayer_Node_t *  NodeToAffect_p,
                                 stlayer_Node_t *  NodeValue_p,
                                 STLAYER_Handle_t  LayerHandle)
{
    stlayer_HWManagerMsg_t * PtMessage;

    /* Reserve a message */
    PtMessage = (stlayer_HWManagerMsg_t *)
    STOS_MessageQueueClaim(stlayer_context.XContext[LayerHandle].MsgQueueTop_p);

    /* fill the message */
    PtMessage->NodeToAffect_p = NodeToAffect_p; /* add */
    STOS_memcpy(&PtMessage->NodeValue, NodeValue_p, sizeof(stlayer_Node_t)); /* val */

    /* send the message */
    STOS_MessageQueueSend(stlayer_context.XContext[LayerHandle].MsgQueueTop_p,
                 (void *)PtMessage);
}

/*******************************************************************************
Name        : stlayer_DifferedToBotWrite
Description : The affectation will be done by the hardware manager task
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void stlayer_DifferedToBotWrite (stlayer_Node_t *  NodeToAffect_p,
                                 stlayer_Node_t *  NodeValue_p,
                                 STLAYER_Handle_t  LayerHandle)
{
    stlayer_HWManagerMsg_t * PtMessage;

    /* Reserve a message */
    PtMessage = (stlayer_HWManagerMsg_t *)
    STOS_MessageQueueClaim(stlayer_context.XContext[LayerHandle].MsgQueueBot_p);

    /* fill the message */
    PtMessage->NodeToAffect_p = NodeToAffect_p; /* add */
    STOS_memcpy(&PtMessage->NodeValue, NodeValue_p, sizeof(stlayer_Node_t)); /* val */

    /* send the message */
    STOS_MessageQueueSend(stlayer_context.XContext[LayerHandle].MsgQueueBot_p,
                 (void *)PtMessage);
}

/*******************************************************************************
Name        : stlayer_ImmediateMemWrite
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stlayer_ImmediateMemWrite(stlayer_Node_t *  NodeToAffect_p,
                                      stlayer_Node_t * NodeValue_p,
                                      STLAYER_Handle_t LayerHandle)
{

    #if defined (ST_7109)
        U32 CutId;
    #endif



   #if defined (ST_7109)
    CutId = ST_GetCutRevision();
   #endif

   UNUSED_PARAMETER(LayerHandle);


    /* Can be called only by a BKL or GDP layer */
    if(NodeValue_p->BklNode.NVN == NON_VALID_REG)
    {
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.CTL),
                            NodeValue_p->BklNode.CTL);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.AGC),
                            NodeValue_p->BklNode.AGC);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.HSRC),
                            NodeValue_p->BklNode.HSRC);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.VPO),
                            NodeValue_p->BklNode.VPO);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.VPS),
                            NodeValue_p->BklNode.VPS);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.PML),
                            NodeValue_p->BklNode.PML);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.PMP),
                            NodeValue_p->BklNode.PMP);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.SIZE),
                            NodeValue_p->BklNode.SIZE);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.VSRC),
                            NodeValue_p->BklNode.VSRC);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.KEY1),
                            NodeValue_p->BklNode.KEY1);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.KEY2),
                            NodeValue_p->BklNode.KEY2);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.HFP),
                            NodeValue_p->BklNode.HFP);
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.PPT),
                            NodeValue_p->BklNode.PPT);


#if defined (ST_7109) || defined (ST_7200)

    #if defined (ST_7109)

    if (CutId >= 0xC0)
    {
    #endif

        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.CML),
                            NodeValue_p->BklNode.CML);

    #if defined (ST_7109)
    }
    #endif

#endif                                  /* #if defined (ST_7109) || defined (ST_7200) */

    }
    else
    {
        STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.NVN),
                            NodeValue_p->BklNode.NVN);



        if(NodeValue_p->BklNode.CTL != NON_VALID_REG)
        {
            STSYS_WriteRegMem32LE(&(NodeToAffect_p->BklNode.CTL),
                            NodeValue_p->BklNode.CTL);
        }
    }
}
/*******************************************************************************
Name        : stlayer_ImmediateRegWrite
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stlayer_ImmediateRegWrite(stlayer_Node_t *  NodeToAffect_p,
                                      stlayer_Node_t *  NodeValue_p,
                                      STLAYER_Handle_t  LayerHandle)
{
    if (stlayer_context.XContext[LayerHandle].LayerType == STLAYER_GAMMA_CURSOR)
    {
        /* the node is a cursor register */
#ifdef WA_GNBvd06319
            /* Do not drive the CTL register */
#else
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.CTL),
                        NodeValue_p->CurNode.CTL);
#endif
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.VPO),
                        NodeValue_p->CurNode.VPO);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.PML),
                        NodeValue_p->CurNode.PML);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.PMP),
                        NodeValue_p->CurNode.PMP);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.SIZE),
                        NodeValue_p->CurNode.SIZE);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.CML),
                        NodeValue_p->CurNode.CML);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.AWS),
                        NodeValue_p->CurNode.AWS);
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->CurNode.AWE),
                        NodeValue_p->CurNode.AWE);
    }
    else
    {
        /* the node is a BKL/GDP register (Base Address) */
        STSYS_WriteRegDev32LE(&(NodeToAffect_p->BklNode.NVN),
                            NodeValue_p->BklNode.NVN);
    }
}


/*******************************************************************************
Name        : stlayer_ExtractColorType
Description : Calculate the values used by the CTL register for color format
                and chroma format.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stlayer_ExtractColorType(STGXOBJ_ColorType_t ColorType,
                    U32 * ColorTypeValue,U32 * ChromaFormat)
{
    *ColorTypeValue = 0;
    *ChromaFormat   = 0;
    switch (ColorType)
    {
        case STGXOBJ_COLOR_TYPE_RGB565:
            *ColorTypeValue = 0;
            break;
        case STGXOBJ_COLOR_TYPE_RGB888:
            *ColorTypeValue = 1;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8565:
        case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            *ColorTypeValue = 4;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            *ColorTypeValue = 5;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB1555:
            *ColorTypeValue = 6;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            *ColorTypeValue = 7;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT1:
            *ColorTypeValue = 8;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            *ColorTypeValue = 9;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *ColorTypeValue = 10;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT8:
            *ColorTypeValue = 11;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT44:
            *ColorTypeValue = 12;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88:
            *ColorTypeValue = 13;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            *ChromaFormat   = 1;
            /* no break */
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
            *ColorTypeValue = 16;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            *ChromaFormat   = 1;
            /* no break */
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            *ColorTypeValue = 18;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_AYCBCR8888:
            *ChromaFormat   = 1;
            /* no break */
        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
            *ColorTypeValue = 21;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            *ColorTypeValue = 24;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
        case STGXOBJ_COLOR_TYPE_ALPHA8:
            *ColorTypeValue = 25;
            break;
        default:
            /* req for no warning */
            break;
    }
}
/*******************************************************************************
Name        :  stlayer_RemapDevice
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_RemapDevice(STLAYER_Handle_t Layer, U32 DeviceId)
{
    void *                      OldBaseAddress;
    stlayer_XLayerContext_t *   Layer_p;
    stlayer_Node_t *            NodeToAffect_p;

    Layer_p = &(stlayer_context.XContext[Layer]);

    /* test param */
    if (!(Layer_p->Initialised))
    {
        layergfx_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }
    /* Set the new base address of the pipe */
    OldBaseAddress = Layer_p->BaseAddress;
    Layer_p->BaseAddress = (void*)((U32)OldBaseAddress - Layer_p->DeviceId
                                                       + DeviceId);
    Layer_p->DeviceId = DeviceId;

    /* Stop the old pipeline */
    /* -> Should be done by the mixer */

    /* link the new pipeline to the linked list */
    NodeToAffect_p = (stlayer_Node_t *)(Layer_p->BaseAddress);
    STSYS_WriteRegDev32LE(&(NodeToAffect_p->BklNode.NVN),
                                   (U32)DeviceAdd2((U32)Layer_p->TmpTopNode_p));

    /* Start the new pipeline */
    /* -> Should be done by the mixer */

    return(ST_NO_ERROR);
}

#if defined(ST_OS21)
#ifdef DUMP_GDPS_NODES

/*******************************************************************************
Name        :  stlayer_DumpGDPNode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stlayer_DumpGDPNode(stlayer_Node_t node1,U8 *str)
{





          fprintf(dumpstream,"------  %s ------- \n",str);
          fprintf(dumpstream,"GDP_CTL  %x \n",node1.BklNode.CTL);
          fprintf(dumpstream,"GDP_AGC  %x \n",node1.BklNode.AGC);
          fprintf(dumpstream,"GDP_HSRC %x \n",node1.BklNode.HSRC);
          fprintf(dumpstream,"GDP_VPO  %x \n",node1.BklNode.VPO);
          fprintf(dumpstream,"GDP_VPS  %x \n",node1.BklNode.VPS);
          fprintf(dumpstream,"GDP_PML  %x \n",node1.BklNode.PML);
          fprintf(dumpstream,"GDP_PMP  %x \n",node1.BklNode.PMP);
          fprintf(dumpstream,"GDP_SIZE %x \n",node1.BklNode.SIZE);
          fprintf(dumpstream,"GDP_VSRC %x \n",node1.BklNode.VSRC);
          fprintf(dumpstream,"GDP_NVN  %x \n",node1.BklNode.NVN);
          fprintf(dumpstream,"GDP_KEY1 %x \n",node1.BklNode.KEY1);
          fprintf(dumpstream,"GDP_KEY2 %x \n",node1.BklNode.KEY2);
          fprintf(dumpstream,"GDP_HFP  %x \n",node1.BklNode.HFP);
          fprintf(dumpstream,"GDP_PPT  %x \n",node1.BklNode.PPT);



}
#endif
#endif




































































/* end of back_end.c */
