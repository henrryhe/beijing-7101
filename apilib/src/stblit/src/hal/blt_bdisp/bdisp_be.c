/*******************************************************************************

File name   : bdisp_be.c

Description : back-end source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Jun 2004        Created                                          HE
30 May 2006        Modified for WinCE platform					  WinCE Team-ST Noida
                   All changes for Wince are under
				   ST_OSWINCE flag
19 Jun 2006        Defined macros BLITTER_READ_REGISTER 		  WinCE Team-ST Noida
				   and BLITTER_WRITE_REGISTER for WinCE
27 sep 2006        - Re Open the job in stblit_InterruptProcess() WinCE Noida Team
                   - All changes are under ST_OSWINCE option.
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */



/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#include "stos.h"
#include "stddefs.h"
#include "bdisp_be.h"
#include "bdisp_init.h"

#ifdef ST_OSLINUX
#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stblit_ioctl.h"
#endif
#else
#include "stsys.h"
#endif

#include "stevt.h"
#include "bdisp_mem.h"
#include "bdisp_pool.h"
#include "bdisp_com.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif

#include "stcommon.h"

#if defined (DVD_SECURED_CHIP)
#include "stmes.h"
#endif

/*#define STBLIT_REPRODUCE_NODE 1*/

#if defined(ST_7109) && defined(ST_OSWINCE)
static U32 filenumber = 0;
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
/*#define BLIT_DEBUG 1*/
/*#define STBLIT_REPRODUCE_NODE 1*/

#ifdef STBLIT_DEBUG_GET_STATISTICS
#if defined (ST_5100)
#define STBLIT_NB_TICKS_PER_MICRO_SECOND   15625
#elif defined (ST_5105)
#define STBLIT_NB_TICKS_PER_MICRO_SECOND   6.25
#elif defined (ST_5301)
#define STBLIT_NB_TICKS_PER_MICRO_SECOND   1002345
#elif defined (ST_7200)
#define STBLIT_NB_TICKS_PER_MICRO_SECOND   0.341796
#elif defined (ST_7109)
#ifdef ST_OSLINUX
#define STBLIT_NB_TICKS_PER_MILLI_SECOND   1
#else /* !ST_OSLINUX */
#define STBLIT_NB_TICKS_PER_MICRO_SECOND   0.259277
#endif /* ST_OSLINUX */
#endif /* 7109 */
#endif /* STBLIT_DEBUG_GET_STATISTICS */


/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */
/*#if defined (ST_OSWINCE)*/
#define BLITTER_READ_REGISTER(Address_p, Value)                      \
    {                                                                   \
        (U32) (Value) = *((volatile U32*) (Address_p)) ;                  \
    }


#define BLITTER_WRITE_REGISTER(Address_p, Value)                      \
    {                                                                   \
        *((volatile U32*) (Address_p)) = (U32) (Value);                  \
    }
/*#endif*/

#ifdef ST_OSLINUX
#ifdef MODULE

#define PrintTrace(x)       STTBX_Print(x)

#else   /* MODULE */

#define PrintTrace(x)		printf x

#endif  /* MODULE */
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
extern STBLITMod_StatusBuffer_t *gpStatusBuffer;
#endif



/* Private Function prototypes ---------------------------------------------- */
#ifndef STBLIT_LINUX_FULL_USER_VERSION
static ST_ErrorCode_t PopInterruptCommand(stblit_ItStatusBuffer * const Buffer_p, stblit_ItStatus * const Data_p);
static void PushInterruptStatus(stblit_ItStatusBuffer * const Buffer_p, const stblit_ItStatus Data);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

static void stblit_InsertNodesAtTheFront(stblit_Device_t * const Device_p,
                                        void* FirstNode_p, void* LastNode_p,
                                        U32 AQSubmit,
                                        BOOL LockBeforeFirstNode );

static void stblit_WriteLastNodeAdress(stblit_Device_t * const Device_p,
                                        void* LastNode_p, U32  AQSubmit);

/* Functions ---------------------------------------------------------------- */
#if defined(STBLIT_SINGLE_BLIT_DCACHE_NODES_ALLOCATION) || defined(STBLIT_JOB_DCACHE_NODES_ALLOCATION)
/*******************************************************************************
Name        : stblit_SoftCacheFlushData
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void  stblit_SoftCacheFlushData()
{
    /* INSbl24777: This is a software cache flush...
    The cache is flushed by reading from a CACHE_SIZE buffer (which must be
    in cacheable memory - we assume all RAM is cached here).  A read is made
    from each line, forcing the 'scratchpad' buffer into cache and flushing
    any dirty cache lines back to memory.
    */

#ifndef CONF_CACHE_SIZE
#if defined(ST_5188) || defined(ST_5105) || defined(ST_5107)
#define CONF_CACHE_SIZE             (4*1024)
#elif defined(ST_5100)
#define CONF_CACHE_SIZE             (8*1024)
#else
#define CONF_CACHE_SIZE             (32*1024)
#endif
#endif /* CONF_CACHE_SIZE */
#ifndef CONF_WORDS_PER_CACHE_LINE
#define CONF_WORDS_PER_CACHE_LINE   4
#endif /* CONF_WORDS_PER_CACHE_LINE */
#define CACHE_SIZE_WORDS            (CONF_CACHE_SIZE / sizeof(U32))

    static      U32  ScratchPad[CACHE_SIZE_WORDS];
    volatile    U32 *Tab_p;
    U32         i,j;

    Tab_p = (volatile U32 *) &ScratchPad;

    for (i = 0, j = 0; i < (CACHE_SIZE_WORDS / CONF_WORDS_PER_CACHE_LINE);
        i++, j += CONF_WORDS_PER_CACHE_LINE)
    {
        (void)Tab_p[j];
    }
}
#endif


#ifdef STBLIT_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : stblit_ConvertFromTicsToMs
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 stblit_ConvertFromTicsToUs(STOS_Clock_t Time)
{
#ifdef ST_OSLINUX
    return ((U32)((Time*1000)/STBLIT_NB_TICKS_PER_MILLI_SECOND));
#else /* !ST_OSLINUX */
    return ((U32)(Time/STBLIT_NB_TICKS_PER_MICRO_SECOND));
#endif /* ST_OSLINUX */

}


/*******************************************************************************
Name        : stblit_CalculateStatistics
Description : Calculate STBLIT statistics
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stblit_CalculateStatistics(stblit_Device_t* Device_p)
{
    if ((Device_p->GenerationEndTime > Device_p->GenerationStartTime) &&
        (Device_p->ExecutionEndTime > Device_p->ExecutionStartTime) &&
        (Device_p->ExecutionEndTime > Device_p->GenerationStartTime))
    {
        /* Update CorrectTimeStatisticsValues */
        Device_p->stblit_Statistics.CorrectTimeStatisticsValues++;

        /* Calculate generation values (us) */
        Device_p->stblit_Statistics.LatestBlitGenerationTime = stblit_ConvertFromTicsToUs(STOS_time_minus(Device_p->GenerationEndTime, Device_p->GenerationStartTime));
        if (Device_p->stblit_Statistics.MinGenerationTime > Device_p->stblit_Statistics.LatestBlitGenerationTime)
        {
            Device_p->stblit_Statistics.MinGenerationTime = Device_p->stblit_Statistics.LatestBlitGenerationTime;
        }
        if (Device_p->stblit_Statistics.MaxGenerationTime < Device_p->stblit_Statistics.LatestBlitGenerationTime)
        {
            Device_p->stblit_Statistics.MaxGenerationTime = Device_p->stblit_Statistics.LatestBlitGenerationTime;
        }
        Device_p->TotalGenerationTime += Device_p->stblit_Statistics.LatestBlitGenerationTime;
        Device_p->stblit_Statistics.AverageGenerationTime = Device_p->TotalGenerationTime / Device_p->stblit_Statistics.CorrectTimeStatisticsValues;

        /* Calculate execution values (us) */
        Device_p->stblit_Statistics.LatestBlitExecutionTime = stblit_ConvertFromTicsToUs(STOS_time_minus(Device_p->ExecutionEndTime, Device_p->ExecutionStartTime));
        if (Device_p->stblit_Statistics.MinExecutionTime > Device_p->stblit_Statistics.LatestBlitExecutionTime)
        {
            Device_p->stblit_Statistics.MinExecutionTime = Device_p->stblit_Statistics.LatestBlitExecutionTime;
        }
        if (Device_p->stblit_Statistics.MaxExecutionTime < Device_p->stblit_Statistics.LatestBlitExecutionTime)
        {
            Device_p->stblit_Statistics.MaxExecutionTime = Device_p->stblit_Statistics.LatestBlitExecutionTime;
        }
        Device_p->TotalExecutionTime += Device_p->stblit_Statistics.LatestBlitExecutionTime;
        Device_p->stblit_Statistics.AverageExecutionTime = Device_p->TotalExecutionTime / Device_p->stblit_Statistics.CorrectTimeStatisticsValues;

        /* Calculate processing time values (us) */
        Device_p->stblit_Statistics.LatestBlitProcessingTime = stblit_ConvertFromTicsToUs(STOS_time_minus(Device_p->ExecutionEndTime, Device_p->GenerationStartTime));
        if (Device_p->stblit_Statistics.MinProcessingTime > Device_p->stblit_Statistics.LatestBlitProcessingTime)
        {
            Device_p->stblit_Statistics.MinProcessingTime = Device_p->stblit_Statistics.LatestBlitProcessingTime;
        }
        if (Device_p->stblit_Statistics.MaxProcessingTime < Device_p->stblit_Statistics.LatestBlitProcessingTime)
        {
            Device_p->stblit_Statistics.MaxProcessingTime = Device_p->stblit_Statistics.LatestBlitProcessingTime;
        }
        Device_p->TotalProcessingTime += Device_p->stblit_Statistics.LatestBlitProcessingTime;
        Device_p->stblit_Statistics.AverageProcessingTime = Device_p->TotalProcessingTime / Device_p->stblit_Statistics.CorrectTimeStatisticsValues;

        /* Calculate execution rate values (Mpixels/ms) */
        Device_p->stblit_Statistics.LatestBlitExecutionRate = ((Device_p->LatestBlitWidth * Device_p->LatestBlitHeight * 1000) / Device_p->stblit_Statistics.LatestBlitProcessingTime);
        if (Device_p->stblit_Statistics.MinExecutionRate > Device_p->stblit_Statistics.LatestBlitExecutionRate)
        {
            Device_p->stblit_Statistics.MinExecutionRate = Device_p->stblit_Statistics.LatestBlitExecutionRate;
        }
        if (Device_p->stblit_Statistics.MaxExecutionRate < Device_p->stblit_Statistics.LatestBlitExecutionRate)
        {
            Device_p->stblit_Statistics.MaxExecutionRate = Device_p->stblit_Statistics.LatestBlitExecutionRate;
        }
        Device_p->TotalExecutionRate += Device_p->stblit_Statistics.LatestBlitExecutionRate;
        Device_p->stblit_Statistics.AverageExecutionRate = Device_p->TotalExecutionRate / Device_p->stblit_Statistics.CorrectTimeStatisticsValues;
    }
}
#endif /* STBLIT_DEBUG_GET_STATISTICS */


#ifdef STBLIT_REPRODUCE_NODE
/*******************************************************************************
Name        : stblit_ReproduceNode
Description : Set the node to the given values
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stblit_ReproduceNode(void* FirstNode_p)
{
    stblit_Node_t* Node_p;

    Node_p = (stblit_Node_t*) FirstNode_p;

    Node_p->HWNode.BLT_NIP         = 0x00000000;
    Node_p->HWNode.BLT_CIC         = 0x00021ff0;
    Node_p->HWNode.BLT_INS         = 0x00000128;
    Node_p->HWNode.BLT_ACK         = 0x00bd0307;
    Node_p->HWNode.BLT_TBA         = Node_p->HWNode.BLT_TBA;
    Node_p->HWNode.BLT_TTY         = 0x14120050;
    Node_p->HWNode.BLT_TXY         = 0x00000000;
    Node_p->HWNode.BLT_T_S1_SZ     = 0x0007000f;
    Node_p->HWNode.BLT_S1CF        = Node_p->HWNode.BLT_S2BA;
    Node_p->HWNode.BLT_S2CF        = 0x30740090;
    Node_p->HWNode.BLT_S1BA        = 0x00000000;
    Node_p->HWNode.BLT_S1TY        = 0x00040008;
    Node_p->HWNode.BLT_S1XY        = Node_p->HWNode.BLT_S3BA;
    Node_p->HWNode.BLT_STUFF_1     = 0x00000090;
    Node_p->HWNode.BLT_S2BA        = 0x00000000;
    Node_p->HWNode.BLT_S2TY        = 0x0007000f;
    Node_p->HWNode.BLT_S2XY        = 0x3a2c7fa6;
    Node_p->HWNode.BLT_S2SZ        = 0x419b21e7;
    Node_p->HWNode.BLT_S3BA        = 0x000d0000;
    Node_p->HWNode.BLT_S3TY        = 0xc01000d0;
    Node_p->HWNode.BLT_S3XY        = 0x001c3064;
    Node_p->HWNode.BLT_S3SZ        = 0xef4de67f;
    Node_p->HWNode.BLT_CWO         = 0x01240155;
    Node_p->HWNode.BLT_CWS         = 0x00194178;
    Node_p->HWNode.BLT_CCO         = Node_p->HWNode.BLT_HFP;
    Node_p->HWNode.BLT_CML         = Node_p->HWNode.BLT_VFP;
    Node_p->HWNode.BLT_F_RZC_CTL   = 0x040002aa;
    Node_p->HWNode.BLT_PMK         = 0x21c922c7;
    Node_p->HWNode.BLT_RSF         = Node_p->HWNode.BLT_Y_HFP;
    Node_p->HWNode.BLT_RZI         = Node_p->HWNode.BLT_Y_VFP;
    Node_p->HWNode.BLT_HFP         = 0x07210f50;
    Node_p->HWNode.BLT_VFP         = 0x096d0f04;
    Node_p->HWNode.BLT_Y_RSF       = 0x0e371831;
    Node_p->HWNode.BLT_Y_RZI       = 0x00027509;
    Node_p->HWNode.BLT_Y_HFP       = 0x080271f1;
    Node_p->HWNode.BLT_Y_VFP       = 0x4939e0f7;
    Node_p->HWNode.BLT_FF0         = 0x00000000;
    Node_p->HWNode.BLT_FF1         = 0x07b083a0;
    /*Node_p->HWNode.BLT_FF2         = 0x00000000;*/
    /*Node_p->HWNode.BLT_FF3         = 0x00000000;*/
    /*Node_p->HWNode.BLT_KEY1        = 0x00000000;*/
    /*Node_p->HWNode.BLT_KEY2        = 0x00000000;*/
    /*Node_p->HWNode.BLT_XYL         = 0x00000000;*/
    /*Node_p->HWNode.BLT_XYP         = 0x00000000;*/
    /*Node_p->HWNode.BLT_SAR         = 0x00000000;*/
    /*Node_p->HWNode.BLT_USR         = 0x00000000;*/
#if defined (ST_7109)
    /*Node_p->HWNode.BLT_IVMX0       = 0x00000000;*/
    /*Node_p->HWNode.BLT_IVMX1       = 0x00000000;*/
    /*Node_p->HWNode.BLT_IVMX2       = 0x00000000;*/
    /*Node_p->HWNode.BLT_IVMX3       = 0x00000000;*/
    /*Node_p->HWNode.BLT_OVMX0       = 0x00000000;*/
    /*Node_p->HWNode.BLT_OVMX1       = 0x00000000;*/
    /*Node_p->HWNode.BLT_OVMX2       = 0x00000000;*/
    /*Node_p->HWNode.BLT_OVMX3       = 0x00000000;*/
#endif
}
#endif /* STBLIT_REPRODUCE_NODE */


#if defined(ST_OSWINCE)
// JLX begin
int DumpNodes = 0;

void STBLIT_DumpNodes(void* FirstNode_p, void* LastNode_p)
{
    U32* NodeBase = (U32*)FirstNode_p;
    U32 value;

    while (1)
    {
        printf("NODE at %p\n", NodeBase);

        value = *NodeBase;
        printf("\tNIP  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*1);
        printf("\tCIC  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*2);
        printf("\tINS  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*3);
        printf("\tACK  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*4);
        printf("\tTBA  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*5);
        printf("\tTTY  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*6);
        printf("\tTXY  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*7);
        printf("\tTSZ  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*8);
        printf("\tS1CF 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*9);
        printf("\tS2CF 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*10);
        printf("\tS1BA 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*11);
        printf("\tS1TY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*12);
        printf("\tS1XY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*14);
        printf("\tS2BA 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*15);
        printf("\tS2TY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*16);
        printf("\tS2XY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*17);
        printf("\tS2SZ 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*18);
        printf("\tS3BA 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*19);
        printf("\tS3TY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*20);
        printf("\tS3XY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*21);
        printf("\tS3SZ 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*22);
        printf("\tCW0  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*23);
        printf("\tCWS  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*24);
        printf("\tCCO  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*25);
        printf("\tCML  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*26);
        printf("\tFCTL 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*27);
        printf("\tPMK  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*28);
        printf("\tRSF  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*29);
        printf("\tRZI  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*30);
        printf("\tHFP  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*31);
        printf("\tVFP  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*32);
        printf("\tYRSF 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*33);
        printf("\tYRZI 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*34);
        printf("\tYHFP 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*35);
        printf("\tYVFP 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*36);
        printf("\tFF0  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*37);
        printf("\tFF1  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*38);
        printf("\tFF2  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*39);
        printf("\tFF3  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*40);
        printf("\tKEY1 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*41);
        printf("\tKEY2 0x%8x \n", value);
        value = *(U32*)((U32)NodeBase + 4*42);
        printf("\tXYL  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*43);
        printf("\tXYP  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*44);
        printf("\tSAR  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*45);
        printf("\tUSR  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*46);
        printf("\tIVMX0  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*47);
        printf("\tIVMX1  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*48);
        printf("\tIVMX2  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*49);
        printf("\tIVMX3  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*50);
        printf("\tOVMX0  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*51);
        printf("\tOVMX1  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*52);
        printf("\tOVMX2  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*53);
        printf("\tOVMX3  0x%x \n",value);

        if  (NodeBase == LastNode_p)
            break;

        NodeBase = (U32*)((*NodeBase) | 0xA0000000);
    }
}

// JLX end
#endif /*ST_OSWINCE*/

#ifdef ST_OSLINUX
/*******************************************************************************
Name        : stblit_DumpRegistersAndNodes
Description : Display all Registers and nodes
Parameters  :
Assumptions : All nodes linked togther via NIP
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stblit_DumpRegistersAndNodes( stblit_Device_t * const Device_p, void* FirstNode_p, void* LastNode_p )
{
    U32     RegisterBase = (U32)Device_p->BaseAddress_p;
    U32*    NodeBase     = (U32*)FirstNode_p;
    U32*    ActualNodeAdress;
    U32     value;
    U32     i = 0;


    /* Recording data */
    PrintTrace(("---------------------------------------- \n"));
    PrintTrace(("DUMP REGISTERS\n"));
    PrintTrace(("---------------------------------------- \n"));

    PrintTrace(("\nGenerals\n"));
    PrintTrace(("=========================== \n"));

    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_ITS)) ;
    PrintTrace(("\tITS  0x%x \n",value));
    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_STA)) ;
    PrintTrace(("\tSTA  0x%x \n",value));

    PrintTrace(("\nStatic Source Base Address\n"));
    PrintTrace(("=========================== \n"));

    for ( i=1 ; i<=8; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_SSBA1+(i-1)*4)) ;
        PrintTrace(("\tBLT_SSBA%d   0x%x \n",i,value));
    }
 /* These registers are inacessible on STi5301 */
#if !defined (ST_5301) && !defined (ST_7109)
    for ( i=1 ; i<=8; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_SSBA9+(i-1)*4)) ;

        PrintTrace(("\tBLT_SSBA%d   0x%x \n",(i+8),value));
    }

    PrintTrace(("\nStatic Target Base Address\n"));
    PrintTrace(("=========================== \n"));

    for ( i=1 ; i<=4; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_STBA1+(i-1)*4)) ;
        PrintTrace(("\tBLT_STBA%d   0x%x \n",i,value));
    }
#endif
    PrintTrace(("\nComposition Queue\n"));
    PrintTrace(("=========================== \n"));

    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CQ1_TRIG_IP)) ;
    PrintTrace(("\tBLT_CQ_TRIG_IP   0x%x \n",value));
    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CQ1_TRIG_CTL)) ;
    PrintTrace(("\tBLT_CQ_TRIG_CTL  0x%x \n",value));

    i = 0;

    PrintTrace(("\n\n---------------------------------------- \n"));
    PrintTrace(("DUMP NODES\n"));
    PrintTrace(("---------------------------------------- \n"));

    do
	{
        PrintTrace(("\nNODE %d  : 0x%x \n",i++,(U32)NodeBase));

#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( NodeBase !=  (U32*)FirstNode_p )
        {
            if ( PhysicJobBlitNodeBuffer_p )
            {
                NodeBase = (U32*) ((U32)NodeBase - (U32) PhysicJobBlitNodeBuffer_p + (U32) Device_p->JobBlitNodeBuffer_p) ;
                PrintTrace(("New Job NODE %d : 0x%x\n", i, (U32)NodeBase ));
            }
            else
            if ( PhysicSingleBlitNodeBuffer_p )
            {
                NodeBase = (U32*) ((U32)NodeBase - (U32) PhysicSingleBlitNodeBuffer_p + (U32) Device_p->SingleBlitNodeBuffer_p) ;
                PrintTrace(("New NODE %d : 0x%x\n", i, (U32)NodeBase ));
            }
        }
#elif ST_OSLINUX
        if ( NodeBase !=  (U32*)FirstNode_p )
        {
            NodeBase = (U32*)((U32) NodeBase + (U32)0xa0000000 );
            PrintTrace(("New NODE %d : 0x%x\n", i, (U32) NodeBase ));
        }
#endif

        PrintTrace(("=========================== \n"));

        value = *NodeBase;
        PrintTrace(("\tNIP  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*1);
        PrintTrace(("\tCIC  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*2);
        PrintTrace(("\tINS  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*3);
        PrintTrace(("\tACK  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*4);
        PrintTrace(("\tTBA  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*5);
        PrintTrace(("\tTTY  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*6);
        PrintTrace(("\tTXY  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*7);
        PrintTrace(("\tTSZ  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*8);
        PrintTrace(("\tS1CF 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*9);
        PrintTrace(("\tS2CF 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*10);
        PrintTrace(("\tS1BA 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*11);
        PrintTrace(("\tS1TY 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*12);
        PrintTrace(("\tS1XY 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*14);
        PrintTrace(("\tS2BA 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*15);
        PrintTrace(("\tS2TY 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*16);
        PrintTrace(("\tS2XY 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*17);
        PrintTrace(("\tS2SZ 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*18);
        PrintTrace(("\tS3BA 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*19);
        PrintTrace(("\tS3TY 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*20);
        PrintTrace(("\tS3XY 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*21);
        PrintTrace(("\tS3SZ 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*22);
        PrintTrace(("\tCW0  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*23);
        PrintTrace(("\tCWS  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*24);
        PrintTrace(("\tCCO  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*25);
        PrintTrace(("\tCML  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*26);
        PrintTrace(("\tFCTL 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*27);
        PrintTrace(("\tPMK  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*28);
        PrintTrace(("\tRSF  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*29);
        PrintTrace(("\tRZI  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*30);
        PrintTrace(("\tHFP  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*31);
        PrintTrace(("\tVFP  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*32);
        PrintTrace(("\tYRSF 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*33);
        PrintTrace(("\tYRZI 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*34);
        PrintTrace(("\tYHFP 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*35);
        PrintTrace(("\tYVFP 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*36);
        PrintTrace(("\tFF0  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*37);
        PrintTrace(("\tFF1  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*38);
        PrintTrace(("\tFF2  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*39);
        PrintTrace(("\tFF3  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*40);
        PrintTrace(("\tKEY1 0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*41);
        PrintTrace(("\tKEY2 0x%x \n", value));
        value = *(U32*)((U32)NodeBase + 4*42);
        PrintTrace(("\tXYL  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*43);
        PrintTrace(("\tXYP  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*44);
        PrintTrace(("\tSAR  0x%x \n",value));
        value = *(U32*)((U32)NodeBase + 4*45);
        PrintTrace(("\tUSR  0x%x \n",value));

        ActualNodeAdress = NodeBase;
        NodeBase = (U32*)(*NodeBase);
    }
    while( ActualNodeAdress != (U32*)LastNode_p );


    return (ST_NO_ERROR);
}
#elif defined (ST_OSWINCE)
/*******************************************************************************
Name        : stblit_DumpRegistersAndNodes
Description : Display all Registers and nodes
Parameters  :
Assumptions : All nodes linked togther via NIP
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stblit_DumpRegistersAndNodes( stblit_Device_t * const Device_p, void* FirstNode_p, void* LastNode_p )
{
    FILE*   fstream;      /* File Handle for write operation        */
    U32     RegisterBase = (U32) Device_p->BaseAddress_p;
    U32*    NodeBase     = (U32*)FirstNode_p;
    U32*    ActualNodeAdress;
    U32     value;
	U32		i 		= 0;
	unsigned char   filename[255];

    /* Open stream */
#if defined(ST_7109)
    sprintf(filename, "%d_STBLIT_Dump.txt", filenumber);
	filenumber++;
	fstream = fopen(filename, "wb");
#else
    fstream = fopen("STBLIT_Dump.txt", "wb");
#endif
    if (fstream == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Recording data */
    fprintf(fstream,"---------------------------------------- \n");
    fprintf(fstream,"DUMP REGISTERS\n");
    fprintf(fstream,"---------------------------------------- \n");

    fprintf(fstream,"\nGenerals\n");
    fprintf(fstream,"=========================== \n");

    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CTL)) ;
    fprintf(fstream,"\tCTL  0x%8x \n",value);
    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_ITS)) ;
    fprintf(fstream,"\tITS  0x%8x \n",value);
    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_STA)) ;
    fprintf(fstream,"\tSTA  0x%8x \n",value);

    fprintf(fstream,"\nStatic Source Base Address\n");
    fprintf(fstream,"=========================== \n");

    for ( i=1 ; i<=8; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_SSBA1+(i-1)*4)) ;
        fprintf(fstream,"\tBLT_SSBA%d   0x%8x \n",i,value);

	}
 /* These registers are inacessible on STi5301 */
 #if !defined (ST_5301)

   for ( i=1 ; i<=8; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_SSBA9+(i-1)*4)) ;
        fprintf(fstream,"\tBLT_SSBA%d   0x%8x \n",(i+8),value);
    }

    fprintf(fstream,"\nStatic Target Base Address\n");
    fprintf(fstream,"=========================== \n");

    for ( i=1 ; i<=4; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_STBA1+(i-1)*4)) ;
        fprintf(fstream,"\tBLT_STBA%d   0x%8x \n",i,value);
    }
#endif
    fprintf(fstream,"\nComposition Queue\n");
    fprintf(fstream,"=========================== \n");

#if defined(ST_7109)
	for (i=1; i<=2; i++)
#else
	for (i=1; i<=1; i++)
#endif
	{
		value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CQ1_TRIG_IP + (i-1)*0x10)) ;
		fprintf(fstream,"\tCQ%d_TRIG_IP   0x%8x \n",i , value);
		value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CQ1_TRIG_CTL+ (i-1)*0x10)) ;
		fprintf(fstream,"\tCQ%d_TRIG_CTL  0x%8x \n", i, value);
		value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CQ1_PACE_CTL+ (i-1)*0x10)) ;
		fprintf(fstream,"\tCQ%d_PACE_CTL  0x%8x \n", i, value);
		value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CQ1_IP+ (i-1)*0x10)) ;
		fprintf(fstream,"\tCQ%d_IP  0x%8x \n", i, value);
	}

    fprintf(fstream,"\nApplication Queue\n");
    fprintf(fstream,"=========================== \n");

#if defined(ST_7109)
	for (i=1; i<=4; i++)
#else
	for (i=1; i<1; i++)
#endif
	{
		value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_AQ1_CTL + (i-1)*0x10)) ;
		fprintf(fstream,"\tAQ%d_CTL   0x%8x \n",i , value);
		value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_AQ1_IP + (i-1)*0x10)) ;
		fprintf(fstream,"\tAQ%d_IP   0x%8x \n",i , value);
		value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_AQ1_LNA + (i-1)*0x10)) ;
		fprintf(fstream,"\tAQ%d_LNA   0x%8x \n",i , value);
		value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_AQ1_STA + (i-1)*0x10)) ;
		fprintf(fstream,"\tAQ%d_STA   0x%8x \n",i , value);
	}

    i = 0;

    fprintf(fstream,"\n\n---------------------------------------- \n");
    fprintf(fstream,"DUMP NODES\n");
    fprintf(fstream,"---------------------------------------- \n");

    do
	{
        fprintf(fstream,"\nNODE %d  : 0x%x \n",i++,(U32)NodeBase);
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( NodeBase !=  (U32*)FirstNode_p )
        {
            if ( PhysicJobBlitNodeBuffer_p )
            {
                NodeBase = (U32) NodeBase - (U32) PhysicJobBlitNodeBuffer_p + (U32) Device_p->JobBlitNodeBuffer_p ;
                fprintf(fstream,"New Job NODE %d : 0x%x\n", i, NodeBase ));
            }
            else
            if ( PhysicSingleBlitNodeBuffer_p )
            {
                NodeBase = (U32) NodeBase - (U32) PhysicSingleBlitNodeBuffer_p + (U32) Device_p->SingleBlitNodeBuffer_p ;
                fprintf(fstream,"New NODE %d : 0x%x\n", i, NodeBase ));
            }
        }
#endif

        fprintf(fstream,"=========================== \n");

        value = *NodeBase;
        fprintf(fstream,"\tNIP  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*1);
        fprintf(fstream,"\tCIC  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*2);
        fprintf(fstream,"\tINS  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*3);
        fprintf(fstream,"\tACK  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*4);
        fprintf(fstream,"\tTBA  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*5);
        fprintf(fstream,"\tTTY  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*6);
        fprintf(fstream,"\tTXY  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*7);
        fprintf(fstream,"\tTSZ  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*8);
        fprintf(fstream,"\tS1CF 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*9);
        fprintf(fstream,"\tS2CF 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*10);
        fprintf(fstream,"\tS1BA 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*11);
        fprintf(fstream,"\tS1TY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*12);
        fprintf(fstream,"\tS1XY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*14);
        fprintf(fstream,"\tS2BA 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*15);
        fprintf(fstream,"\tS2TY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*16);
        fprintf(fstream,"\tS2XY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*17);
        fprintf(fstream,"\tS2SZ 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*18);
        fprintf(fstream,"\tS3BA 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*19);
        fprintf(fstream,"\tS3TY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*20);
        fprintf(fstream,"\tS3XY 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*21);
        fprintf(fstream,"\tS3SZ 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*22);
        fprintf(fstream,"\tCW0  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*23);
        fprintf(fstream,"\tCWS  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*24);
        fprintf(fstream,"\tCCO  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*25);
        fprintf(fstream,"\tCML  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*26);
        fprintf(fstream,"\tFCTL 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*27);
        fprintf(fstream,"\tPMK  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*28);
        fprintf(fstream,"\tRSF  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*29);
        fprintf(fstream,"\tRZI  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*30);
        fprintf(fstream,"\tHFP  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*31);
        fprintf(fstream,"\tVFP  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*32);
        fprintf(fstream,"\tYRSF 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*33);
        fprintf(fstream,"\tYRZI 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*34);
        fprintf(fstream,"\tYHFP 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*35);
        fprintf(fstream,"\tYVFP 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*36);
        fprintf(fstream,"\tFF0  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*37);
        fprintf(fstream,"\tFF1  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*38);
        fprintf(fstream,"\tFF2  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*39);
        fprintf(fstream,"\tFF3  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*40);
        fprintf(fstream,"\tKEY1 0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*41);
        fprintf(fstream,"\tKEY2 0x%8x \n", value);
        value = *(U32*)((U32)NodeBase + 4*42);
        fprintf(fstream,"\tXYL  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*43);
        fprintf(fstream,"\tXYP  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*44);
        fprintf(fstream,"\tSAR  0x%8x \n",value);
        value = *(U32*)((U32)NodeBase + 4*45);
        fprintf(fstream,"\tUSR  0x%8x \n",value);
#if defined (ST_7109)
        value = *(U32*)((U32)NodeBase + 4*46);
        fprintf(fstream,"\tIVMX0  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*47);
        fprintf(fstream,"\tIVMX1  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*48);
        fprintf(fstream,"\tIVMX2  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*49);
        fprintf(fstream,"\tIVMX3  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*50);
        fprintf(fstream,"\tOVMX0  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*51);
        fprintf(fstream,"\tOVMX1  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*52);
        fprintf(fstream,"\tOVMX2  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*53);
        fprintf(fstream,"\tOVMX3  0x%x \n",value);
#endif

        ActualNodeAdress = NodeBase;
        NodeBase = (U32*)(*NodeBase);
        NodeBase = (U32*)stblit_DeviceToCpu( NodeBase, &(Device_p->VirtualMapping) );
    }
    while( ActualNodeAdress != (U32*)LastNode_p );

    /* Close stream */
    fclose (fstream);

    return (ST_NO_ERROR);
}
#else /* !ST_OSLINUX && !ST_OSWINCE */
/*******************************************************************************
Name        : stblit_DumpRegistersAndNodes
Description : Display all Registers and nodes
Parameters  :
Assumptions : All nodes linked togther via NIP
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stblit_DumpRegistersAndNodes( stblit_Device_t * const Device_p, void* FirstNode_p, void* LastNode_p )
{
    FILE*   fstream;      /* File Handle for write operation        */
    U32     RegisterBase = (U32) Device_p->BaseAddress_p;
    U32*    NodeBase     = (U32*)FirstNode_p;
    U32*    ActualNodeAdress;
    U32     value;
    U32     i = 0;
#if defined (ST_7109)
    U32 CutId;
#endif
    /* Open stream */
    fstream = fopen("STBLIT_Dump.txt", "wb");
    if (fstream == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Recording data */
    fprintf(fstream,"---------------------------------------- \n");
    fprintf(fstream,"DUMP REGISTERS\n");
    fprintf(fstream,"---------------------------------------- \n");

    fprintf(fstream,"\nGenerals\n");
    fprintf(fstream,"=========================== \n");

    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_ITS)) ;
    fprintf(fstream,"\tITS  0x%x \n",value);
    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_STA)) ;
    fprintf(fstream,"\tSTA  0x%x \n",value);

    fprintf(fstream,"\nStatic Source Base Address\n");
    fprintf(fstream,"=========================== \n");

    for ( i=1 ; i<=8; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_SSBA1+(i-1)*4)) ;
        fprintf(fstream,"\tBLT_SSBA%d   0x%x \n",i,value);
    }
 /* These registers are inacessible on STi5301 */
 #if !defined (ST_5301)

   for ( i=1 ; i<=8; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_SSBA9+(i-1)*4)) ;
        fprintf(fstream,"\tBLT_SSBA%d   0x%x \n",(i+8),value);
    }

    fprintf(fstream,"\nStatic Target Base Address\n");
    fprintf(fstream,"=========================== \n");

    for ( i=1 ; i<=4; i++ )
    {
        value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_STBA1+(i-1)*4)) ;
        fprintf(fstream,"\tBLT_STBA%d   0x%x \n",i,value);
    }
#endif
    fprintf(fstream,"\nComposition Queue\n");
    fprintf(fstream,"=========================== \n");

    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CQ1_TRIG_IP)) ;
    fprintf(fstream,"\tBLT_CQ_TRIG_IP   0x%x \n",value);
    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CQ1_TRIG_CTL)) ;
    fprintf(fstream,"\tBLT_CQ_TRIG_CTL  0x%x \n",value);

    i = 0;

    fprintf(fstream,"\n\n---------------------------------------- \n");
    fprintf(fstream,"DUMP NODES\n");
    fprintf(fstream,"---------------------------------------- \n");

    do
	{
        fprintf(fstream,"\nNODE %d  : 0x%x \n",i++,(U32)NodeBase);
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( NodeBase !=  (U32*)FirstNode_p )
        {
            if ( PhysicJobBlitNodeBuffer_p )
            {
                NodeBase = (U32) NodeBase - (U32) PhysicJobBlitNodeBuffer_p + (U32) Device_p->JobBlitNodeBuffer_p ;
                fprintf(fstream,"New Job NODE %d : 0x%x\n", i, NodeBase ));
            }
            else
            if ( PhysicSingleBlitNodeBuffer_p )
            {
                NodeBase = (U32) NodeBase - (U32) PhysicSingleBlitNodeBuffer_p + (U32) Device_p->SingleBlitNodeBuffer_p ;
                fprintf(fstream,"New NODE %d : 0x%x\n", i, NodeBase ));
            }
        }
#endif

        fprintf(fstream,"=========================== \n");

        value = *NodeBase;
        fprintf(fstream,"\tNIP  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*1);
        fprintf(fstream,"\tCIC  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*2);
        fprintf(fstream,"\tINS  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*3);
        fprintf(fstream,"\tACK  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*4);
        fprintf(fstream,"\tTBA  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*5);
        fprintf(fstream,"\tTTY  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*6);
        fprintf(fstream,"\tTXY  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*7);
        fprintf(fstream,"\tTSZ  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*8);
        fprintf(fstream,"\tS1CF 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*9);
        fprintf(fstream,"\tS2CF 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*10);
        fprintf(fstream,"\tS1BA 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*11);
        fprintf(fstream,"\tS1TY 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*12);
        fprintf(fstream,"\tS1XY 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*14);
        fprintf(fstream,"\tS2BA 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*15);
        fprintf(fstream,"\tS2TY 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*16);
        fprintf(fstream,"\tS2XY 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*17);
        fprintf(fstream,"\tS2SZ 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*18);
        fprintf(fstream,"\tS3BA 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*19);
        fprintf(fstream,"\tS3TY 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*20);
        fprintf(fstream,"\tS3XY 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*21);
        fprintf(fstream,"\tS3SZ 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*22);
        fprintf(fstream,"\tCW0  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*23);
        fprintf(fstream,"\tCWS  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*24);
        fprintf(fstream,"\tCCO  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*25);
        fprintf(fstream,"\tCML  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*26);
        fprintf(fstream,"\tFCTL 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*27);
        fprintf(fstream,"\tPMK  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*28);
        fprintf(fstream,"\tRSF  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*29);
        fprintf(fstream,"\tRZI  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*30);
        fprintf(fstream,"\tHFP  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*31);
        fprintf(fstream,"\tVFP  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*32);
        fprintf(fstream,"\tYRSF 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*33);
        fprintf(fstream,"\tYRZI 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*34);
        fprintf(fstream,"\tYHFP 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*35);
        fprintf(fstream,"\tYVFP 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*36);
        fprintf(fstream,"\tFF0  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*37);
        fprintf(fstream,"\tFF1  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*38);
        fprintf(fstream,"\tFF2  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*39);
        fprintf(fstream,"\tFF3  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*40);
        fprintf(fstream,"\tKEY1 0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*41);
        fprintf(fstream,"\tKEY2 0x%x \n", value);
        value = *(U32*)((U32)NodeBase + 4*42);
        fprintf(fstream,"\tXYL  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*43);
        fprintf(fstream,"\tXYP  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*44);
        fprintf(fstream,"\tSAR  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*45);
        fprintf(fstream,"\tUSR  0x%x \n",value);
#if defined(ST_7109) || defined(ST_7200)
        value = *(U32*)((U32)NodeBase + 4*46);
        fprintf(fstream,"\tIVMX0  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*47);
        fprintf(fstream,"\tIVMX1  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*48);
        fprintf(fstream,"\tIVMX2  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*49);
        fprintf(fstream,"\tIVMX3  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*50);
        fprintf(fstream,"\tOVMX0  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*51);
        fprintf(fstream,"\tOVMX1  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*52);
        fprintf(fstream,"\tOVMX2  0x%x \n",value);
        value = *(U32*)((U32)NodeBase + 4*53);
        fprintf(fstream,"\tOVMX3  0x%x \n",value);

#if defined (ST_7109)
        /* get the cut ID*/
        CutId = ST_GetCutRevision();
        /* Check for device ID for 7109 Cut */
        if (CutId >= 0xC0)
#endif /*ST_7109*/
        {
            value = *(U32*)((U32)NodeBase + 4*54);
            fprintf(fstream,"\tPACE  0x%x \n",value);
            value = *(U32*)((U32)NodeBase + 4*55);
            fprintf(fstream,"\tDOT  0x%x \n",value);
            value = *(U32*)((U32)NodeBase + 4*56);
            fprintf(fstream,"\tVC1R  0x%x \n",value);
        }
#endif   /*ST_7109 && ST_7200*/

        ActualNodeAdress = NodeBase;
        NodeBase = (U32*)(*NodeBase);
        NodeBase = (U32*)stblit_DeviceToCpu( NodeBase, &(Device_p->VirtualMapping) );
    }
    while( ActualNodeAdress != (U32*)LastNode_p );

    /* Close stream */
    fclose (fstream);

    return (ST_NO_ERROR);
}
#endif

#ifdef ST_OSWINCE
stblit_DumpNode(U32* reg)
{
	U32 value;
	U32 mode;
	U32* NodeBase = *reg;

	NodeBase = (U32*)((U32)NodeBase | 0xA0000000);
	printf("Dumping node at address %p ...\n", NodeBase);

	value = *NodeBase;
	printf("\tNIP  0x%8x \n",value);

	value = *(U32*)((U32)NodeBase + 4*2);
	printf("\tINS  0x%8x \n",value);
	printf("\t\tENABLE_IRQ = %d\n", (value >> STBLIT_INS_ENABLE_IRQ_SHIFT) & STBLIT_INS_ENABLE_IRQ_MASK);
	printf("\t\tENABLE_RESCALE = %d\n", (value >> STBLIT_INS_ENABLE_RESCALE_SHIFT) & STBLIT_INS_ENABLE_RESCALE_MASK);
	mode = (value >> STBLIT_INS_SRC1_MODE_SHIFT) & STBLIT_INS_SRC1_MODE_MASK;
	printf("\t\tSRC1_MODE = %d ", mode);
	if (mode == STBLIT_INS_SRC1_MODE_MEMORY) printf("bitmap");
	if (mode == STBLIT_INS_SRC1_MODE_COLOR_FILL) printf("color");
	if (mode == STBLIT_INS_SRC1_MODE_DIRECT_COPY) printf("direct copy");
	printf("\n");
	mode = (value >> STBLIT_INS_SRC2_MODE_SHIFT) & STBLIT_INS_SRC2_MODE_MASK;
	printf("\t\tSRC2_MODE = %d ", mode);
	if (mode == STBLIT_INS_SRC2_MODE_MEMORY) printf("bitmap");
	if (mode == STBLIT_INS_SRC2_MODE_COLOR_FILL) printf("color");
	printf("\n");

	value = *(U32*)((U32)NodeBase + 4*3);
	printf("\tACK  0x%08x \n",value);
	printf("\t\tmode is %d\n", (value >> STBLIT_ACK_ALU_MODE_SHIFT) & STBLIT_ACK_ALU_MODE_MASK);
	if ((value >> STBLIT_ACK_ALU_MODE_SHIFT) & STBLIT_ACK_ALU_MODE_MASK == 2)
		printf("\t\t\t=> Alpha blending\n");
	if ((value >> STBLIT_ACK_ALU_MODE_SHIFT) & STBLIT_ACK_ALU_MODE_MASK == 1)
		printf("\t\t\t=> logical op (copy)\n");

	printf("\t\talpha or op is %d\n", (value >> STBLIT_ACK_ALU_ROP_MODE_SHIFT) & STBLIT_ACK_ALU_ROP_MODE_MASK);

	value = *(U32*)((U32)NodeBase + 4*4);
	printf("\tTBA  0x%8x \n",value);

	value = *(U32*)((U32)NodeBase + 4*5);
	printf("\tTTY  0x%8x \n",value);
	printf("\t\talpha range is %d\n", (value >> STBLIT_TTY_ALPHA_RANGE_SHIFT) & STBLIT_TTY_ALPHA_RANGE_MASK);
	printf("\t\tcolor format is %d (%s)\n", (value >> STBLIT_TTY_COLOR_SHIFT) & STBLIT_TTY_COLOR_MASK,
		((value >> STBLIT_TTY_COLOR_SHIFT) & STBLIT_TTY_COLOR_MASK) >> 4 ? "YUV" : "RGB");
	printf("\t\tpitch is %d\n", (value >> STBLIT_TTY_PITCH_SHIFT) & STBLIT_TTY_PITCH_MASK);
	printf("\t\tscan order is %d (should be 0)\n", (value >> STBLIT_TTY_SCAN_ORDER_SHIFT) & STBLIT_TTY_SCAN_ORDER_MASK);

	value = *(U32*)((U32)NodeBase + 4*6);
	printf("\tTXY  0x%8x (i.e. positionY, positionX)\n",value);

	value = *(U32*)((U32)NodeBase + 4*7);
	printf("\tTSZ  0x%8x (i.e. height, width)\n",value);

	value = *(U32*)((U32)NodeBase + 4*8);
	printf("\tS1CF 0x%8x \n",value);

	value = *(U32*)((U32)NodeBase + 4*9);
	printf("\tS2CF 0x%8x \n",value);

	value = *(U32*)((U32)NodeBase + 4*10);
	printf("\tS1BA 0x%8x \n",value);

	value = *(U32*)((U32)NodeBase + 4*11);
	printf("\tS1TY 0x%8x \n",value);
	printf("\t\talpha range is %d\n", (value >> STBLIT_S1TY_ALPHA_RANGE_SHIFT) & STBLIT_S1TY_ALPHA_RANGE_MASK);
	printf("\t\tcolor format is %d (%s)\n", (value >> STBLIT_S1TY_COLOR_SHIFT) & STBLIT_S1TY_COLOR_MASK,
		((value >> STBLIT_S1TY_COLOR_SHIFT) & STBLIT_S1TY_COLOR_MASK) >> 4 ? "YUV" : "RGB");
	printf("\t\tpitch is %d\n", (value >> STBLIT_S1TY_PITCH_SHIFT) & STBLIT_S1TY_PITCH_MASK);
	printf("\t\tscan order is %d (should be 0)\n", (value >> STBLIT_S1TY_SCAN_ORDER_SHIFT) & STBLIT_S1TY_SCAN_ORDER_MASK);

	value = *(U32*)((U32)NodeBase + 4*12);
	printf("\tS1XY 0x%8x (i.e. positionY, positionX)\n",value);

	value = *(U32*)((U32)NodeBase + 4*14);
	printf("\tS2BA 0x%8x \n",value);

	value = *(U32*)((U32)NodeBase + 4*15);
	printf("\tS2TY 0x%8x \n",value);
	printf("\t\talpha range is %d\n", (value >> STBLIT_S2TY_ALPHA_RANGE_SHIFT) & STBLIT_S2TY_ALPHA_RANGE_MASK);
	printf("\t\tcolor format is %d (%s)\n", (value >> STBLIT_S2TY_COLOR_SHIFT) & STBLIT_S2TY_COLOR_MASK,
		((value >> STBLIT_S2TY_COLOR_SHIFT) & STBLIT_S2TY_COLOR_MASK) >> 4 ? "YUV" : "RGB");
	printf("\t\tpitch is %d\n", (value >> STBLIT_S1TY_PITCH_SHIFT) & STBLIT_S2TY_PITCH_MASK);
	printf("\t\tscan order is %d (should be 0)\n", (value >> STBLIT_S2TY_SCAN_ORDER_SHIFT) & STBLIT_S2TY_SCAN_ORDER_MASK);

	value = *(U32*)((U32)NodeBase + 4*16);
	printf("\tS2XY 0x%8x (i.e. positionY, positionX)\n",value);

	value = *(U32*)((U32)NodeBase + 4*17);
	printf("\tS2SZ 0x%8x (i.e. height, width)\n",value);

	value = *(U32*)((U32)NodeBase + 4*26);
	printf("\tFCTL 0x%8x (filter control)\n",value);
	printf("\t\tFilter mode is %d\n", (value >> STBLIT_RZC_FFILTER_MODE_SHIFT) & STBLIT_RZC_FFILTER_MODE_MASK);
	printf("\t\tHoriz mode is %d\n", (value >> STBLIT_RZC_2DHFILTER_MODE_SHIFT) & STBLIT_RZC_2DHFILTER_MODE_MASK);
	printf("\t\tVert mode is %d\n", (value >> STBLIT_RZC_2DVFILTER_MODE_SHIFT) & STBLIT_RZC_2DVFILTER_MODE_MASK);

	value = *(U32*)((U32)NodeBase + 4*28);
	printf("\tRSF  0x%8x (vertical/horizontal scaling factor * 0x400\n", value);

	value = *(U32*)((U32)NodeBase + 4*29);
	printf("\tRZI  0x%8x (initial values)\n",value);

	value = *(U32*)((U32)NodeBase + 4*30);
	printf("\tHFP  0x%8x (H filter pointer)\n",value);

	value = *(U32*)((U32)NodeBase + 4*31);
	printf("\tVFP  0x%8x (V filter pointer)\n",value);

	value = *(U32*)((U32)NodeBase + 4*32);
	printf("\tYRSF 0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*33);
	printf("\tYRZI 0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*34);
	printf("\tYHFP 0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*35);
	printf("\tYVFP 0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*36);
	printf("\tFF0  0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*37);
	printf("\tFF1  0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*38);
	printf("\tFF2  0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*39);
	printf("\tFF3  0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*40);
	printf("\tKEY1 0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*41);
	printf("\tKEY2 0x%8x \n", value);
	value = *(U32*)((U32)NodeBase + 4*42);
	printf("\tXYL  0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*43);
	printf("\tXYP  0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*44);
	printf("\tSAR  0x%8x \n",value);
	value = *(U32*)((U32)NodeBase + 4*45);
	printf("\tUSR  0x%8x \n",value);
}

U32 LastIP; // AQ1_IP when the blitter was launched

void STBLIT_debug(void)
{
    U32     RegisterBase = (U32)0xB920B000;
    U32*    ActualNodeAdress;
    U32     value;
	U32		i = 0;

    printf("DUMP REGISTERS\n");
    printf("---------------------------------------- \n");
	printf("Note: LastIP is %p\n", LastIP);

    printf("\nGenerals\n");
    printf("=========================== \n");

    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_CTL)) ;
    printf("\tCTL  0x%8x \n",value);
    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_ITS)) ;
    printf("\tITS  0x%8x \n",value);
	printf("\t\tAQ1_NODE_NOTIF  = %d\n", value & BLT_ITS_AQ1_NODE_NOTIF_MASK);
	printf("\t\tAQ1_STOPPED     = %d\n", value & BLT_ITS_AQ1_STOPPED_MASK);
	printf("\t\tAQ1_LNA_REACHED = %d\n", value & BLT_ITS_AQ1_LNA_REACHED_MASK);
    value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_STA)) ;
    printf("\tSTA  0x%8x \n",value);
	printf("\t\tBDisp is %s\n", value ? "IDLE" : "RUNNING");

    printf("\nApplication Queue\n");
    printf("=========================== \n");

	value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_AQ1_CTL)) ;
	printf("\tAQ1_CTL   0x%8x \n", value);
	value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_AQ1_IP)) ;
	printf("\tAQ1_IP   0x%8x \n", value);
	value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_AQ1_LNA)) ;
	printf("\tAQ1_LNA   0x%8x \n", value);
	value = (U32)STSYS_ReadRegDev32LE((void*)((U32)RegisterBase + BLT_AQ1_STA)) ;
	printf("\tAQ1_STA   0x%8x \n", value);

    printf("\n\n---------------------------------------- \n");
    printf("DUMP NODES\n");
    printf("---------------------------------------- \n");

    printf("DUMP IP NODE\n");
	stblit_DumpNode((U32*)((U32)RegisterBase + BLT_AQ1_IP));

	printf("DUMP LNA NODE\n");
	stblit_DumpNode((U32*)((U32)RegisterBase + BLT_AQ1_LNA));
}

#endif

#ifndef STBLIT_LINUX_FULL_USER_VERSION
/*******************************************************************************
Name        : PopInterruptCommand
Description : Pop an IT status from a circular buffer of ITs status
Parameters  : Buffer, pointer to data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't pop if empty
Returns     : ST_NO_ERROR if success,
              ST_ERROR_NO_MEMORY if buffer empty,
              ST_ERROR_BAD_PARAMETER if bad params
*******************************************************************************/
static ST_ErrorCode_t PopInterruptCommand(stblit_ItStatusBuffer * const Buffer_p, stblit_ItStatus * const Data_p)
{
    if (Data_p == NULL)
    {
        /* No param pointer provided: cannot return data ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access to BeginPointer_p and UsedSize */
    STOS_InterruptLock();

    /* Check that buffer is not empty. */
    if (Buffer_p->UsedSize != 0)
    {
        /* Read character */
        *Data_p = *(Buffer_p->BeginPointer_p);

        if ((Buffer_p->BeginPointer_p) >= (Buffer_p->Data_p + Buffer_p->TotalSize - 1))
        {
            Buffer_p->BeginPointer_p = Buffer_p->Data_p;
        }
        else
        {
            (Buffer_p->BeginPointer_p)++;
        }

        /* Update size */
        (Buffer_p->UsedSize)--;
    }
    else
    {
        /* Un-protect access to BeginPointer_p and UsedSize */
        STOS_InterruptUnlock();
        /* Can't pop command: buffer empty. So go out after interrupt_unlock() */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    STOS_InterruptUnlock();

    return(ST_NO_ERROR);
} /* End of PopInterruptCommand() function */


/*******************************************************************************
Name        : PushInterruptStatus
Description : Push an IT status from a circular buffer of ITs status
Parameters  : Buffer, data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't push if full
Returns     : Nothing
*******************************************************************************/
static void PushInterruptStatus(stblit_ItStatusBuffer * const Buffer_p, const stblit_ItStatus Data)
{
    /* Protect access to BeginPointer_p and UsedSize */
     STOS_InterruptLock();

    if (Buffer_p->UsedSize + 1 <= Buffer_p->TotalSize)
    {
        /* Write character */
        if ((Buffer_p->BeginPointer_p + Buffer_p->UsedSize) >= (Buffer_p->Data_p + Buffer_p->TotalSize))
        {
            /* Wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - Buffer_p->TotalSize) = Data;
        }
        else
        {
            /* No wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize) = Data;
        }

        /* Update size */
        (Buffer_p->UsedSize)++;

        /* Monitor max used size in commands buffer */
        if (Buffer_p->UsedSize > Buffer_p->MaxUsedSize)
        {
            Buffer_p->MaxUsedSize = Buffer_p->UsedSize;
        }
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    STOS_InterruptUnlock();

    /* Can't push command: buffer full */
    return;
} /* End of PushInterruptStatus() function */
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

/*******************************************************************************
Name        : stblit_BlitterInterruptHandler
Description : Interrupt routine callback (part of the main ISR)
Parameters  :
Assumptions : Its thread is the main ISR (Interruot Service routine).
              Device_p is passed through params.
              Must be as short as possible!
Limitations :
Returns     : Nothing
*******************************************************************************/
#ifndef STBLIT_LINUX_FULL_USER_VERSION
#if defined(ST_OSLINUX)
int stblit_BlitterInterruptHandler(int irq, void * Param_p, struct pt_regs * regs)
#endif  /* ST_OSLINUX */
#if defined(ST_OS21) || defined(ST_OSWINCE)
int stblit_BlitterInterruptHandler (void* Param_p)
#endif
#ifdef ST_OS20
void stblit_BlitterInterruptHandler (void* Param_p)
#endif
{
    U32 ITS,AQ1_STA;
    stblit_Device_t*    Device_p;
    stblit_ItStatus     ItStatus;

    Device_p = (stblit_Device_t*)Param_p;

    /* Read ITS register to get the activity of the blitter */
    ITS = (U32)STSYS_ReadRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_ITS));

    /* Read AQ1_STA register to get the node address */
    AQ1_STA = (U32)stblit_DeviceToCpu((void*)((U32)STSYS_ReadRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_AQ1_STA))), &(Device_p->VirtualMapping));

    /* Reset Pending Interrupt */
    STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_ITS),ITS & BLT_ITS_AQ1_RESET_INTERRUPTION);

    /* Copy params into Interrupt Status to be stocked in the interrupts buffer and used by the stblit_InterruptProcess*/
    ItStatus.ITS = ITS;
    ItStatus.AQ1_STA = AQ1_STA;

#ifdef BLIT_DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "stblit_BlitterInterruptHandler ():ITS = 0x%x \n", ItStatus.ITS));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "stblit_BlitterInterruptHandler ():AQ1_STA = 0x%x \n", ItStatus.AQ1_STA));
#endif

    /* Push Interrupt Status in the FIFO */
    PushInterruptStatus(&(Device_p->InterruptsBuffer), ItStatus);
    STOS_SemaphoreSignal(Device_p->InterruptProcessSemaphore);

#if defined(ST_OS21) || defined(ST_OSWINCE)
    return(OS21_SUCCESS);
#endif /* ST_OS21 */
#if defined(ST_OSLINUX)
	return (IRQ_HANDLED);
#endif

}
#endif /*STBLIT_LINUX_FULL_USER_VERSION*/

#ifdef STBLIT_LINUX_FULL_USER_VERSION
/*******************************************************************************
Name        : stblit_InterruptHandlerTask
Description : Task handling interrupt (read status from kernel
           and call InterruptProcess when interrupt occured)
Parameters  :
Limitations :
Returns     : Nothing
*******************************************************************************/

void stblit_InterruptHandlerTask(const void* SubscriberData_p)
{
    STBLITMod_Status_t      STBLITMod_Status;
    U32                     ITS,AQ1_STA,i;
    stblit_ItStatus         ItStatus;

   #if !defined(MODULE) && defined(PTV)
   stblit_Device_t         *Device_p = (stblit_Device_t*)(((PK_ThreadData *)SubscriberData_p)->param);
   #else
   U32                     TempAddress;
   stblit_Device_t         *Device_p;

   TempAddress = (U32)SubscriberData_p;
   Device_p  = (stblit_Device_t *)TempAddress;
   #endif /* !MODULE && PTV */

	/* To remove comipation warnning */
	UNUSED_PARAMETER(ItStatus);
	UNUSED_PARAMETER(AQ1_STA);
	UNUSED_PARAMETER(ITS);


   while (gpStatusBuffer->NextRead != gpStatusBuffer->NextWrite)
   {

#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    /* Take access control */
    STOS_SemaphoreWait(Device_p->BlitterErrorCheck_AccessControlSemaphore);

        if(Device_p->BlitterErrorCheck_WaitingForInterrupt && !Device_p->BlitterErrorCheck_GotOneInterrupt)
        {
#ifdef BLIT_DEBUG
            PrintTrace(("\n>Int_Not"));
#endif

            /* Notify stblit_BlitterErrorCheckProcess for just one interrupt */
            Device_p->BlitterErrorCheck_GotOneInterrupt = TRUE;

            /* Release accesss control */
            STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_AccessControlSemaphore);

            /* Notify stblit_BlitterErrorCheckProcess */
            STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_InterruptSemaphore);
        }
        else
        {
            /* Release accesss control */
            STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_AccessControlSemaphore);
        }
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */


      STBLITMod_Status.Status1 = gpStatusBuffer->Status[gpStatusBuffer->NextRead].Status1;
      for(i = 0; i < STBLIT_NUM_AQS; i++)
      {
         if(STBLITMod_Status.Status1 & (BLT_ITS_AQ1_NODE_NOTIF_MASK << (i*4)))
         {
            if ( PhysicJobBlitNodeBuffer_p )
            {
                STBLITMod_Status.Status_AQ[i] = (U32) Device_p->JobBlitNodeBuffer_p +
                        gpStatusBuffer->Status[gpStatusBuffer->NextRead].Status_AQ[i] - PhysicJobBlitNodeBuffer_p;

            }
            else
            {
                STBLITMod_Status.Status_AQ[i] = (U32) Device_p->SingleBlitNodeBuffer_p +
                        gpStatusBuffer->Status[gpStatusBuffer->NextRead].Status_AQ[i] - PhysicSingleBlitNodeBuffer_p;
            }
         }
         else
         {
            STBLITMod_Status.Status_AQ[i] = 0;
         }
      }

      gpStatusBuffer->NextRead = (gpStatusBuffer->NextRead+1)%1024;

      stblit_InterruptProcess(Device_p, &STBLITMod_Status);
   }
}
#endif

/*******************************************************************************
Name        : stblit_waitallblitscompleted()
Description : Wait until the blitter enter in the idle state (used Sync chip
                    for DirectFB )
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_waitallblitscompleted( stblit_Device_t* Device_p )
{
    /* Waiting start only if there is pending node */
    if ( Device_p->SBlitNodeDataPool.NbFreeElem < (Device_p->SBlitNodeDataPool.NbElem - 1) )
    {
        Device_p->StartWaitBlitComplete = TRUE ;
        /* Code for sync chip : Sleep until waiting node is 0 */
        STOS_SemaphoreWait(Device_p->AllBlitsCompleted_p);
    }
}




/*******************************************************************************
Name        : stblit_InterruptProcess
Description : Process started by ISR :
                + Remove already processed Nodes
                + Notify Event(s)
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/

#ifdef STBLIT_LINUX_FULL_USER_VERSION
void stblit_InterruptProcess (stblit_Device_t * Device_p, STBLITMod_Status_t * Status_p)
{
   void*               UserTag_p = NULL ;
   STEVT_SubscriberID_t SubscriberID = 0;
   U32 ITOpcode;
   STBLIT_JobHandle_t  JobHandle;
   stblit_NodeHandle_t  CurrentHandle, NodeHandle;
   stblit_ItStatus     ItStatus;
   U32 i;

	/* To remove comipation warnning */
	UNUSED_PARAMETER(ItStatus);


   for(i = 0; i < STBLIT_NUM_AQS; i++)
   {
      /* Extract data */
      CurrentHandle = (stblit_NodeHandle_t) Status_p->Status_AQ[i];

      if(CurrentHandle)
      {
        NodeHandle = CurrentHandle;
        /* Overwrite Support */
        UserTag_p = (void*)((stblit_Node_t*)NodeHandle)->UserTag_p;

        SubscriberID=((stblit_Node_t*)NodeHandle)->SubscriberID;

        NodeHandle = ((stblit_Node_t*)NodeHandle)->PreviousNode;

        ITOpcode =((stblit_Node_t*)CurrentHandle)->ITOpcode;

        JobHandle = ((stblit_Node_t*)CurrentHandle)->JobHandle;

        /* Get the virtual adress of BLT_NIP which is the first node at the current list */
        if (JobHandle == STBLIT_NO_JOB_HANDLE)
        {
            Device_p->FirstNodeTab[i] = (stblit_Node_t*)((U32) Device_p->SingleBlitNodeBuffer_p + (U32)(((stblit_Node_t*)CurrentHandle)->HWNode.BLT_NIP) - (U32) PhysicSingleBlitNodeBuffer_p);
        }
        else
        {
            Device_p->FirstNodeTab[i] = (stblit_Node_t*)((U32) Device_p->JobBlitNodeBuffer_p + (U32)(((stblit_Node_t*)CurrentHandle)->HWNode.BLT_NIP) - (U32) PhysicJobBlitNodeBuffer_p);
        }

        stblit_ReleaseListNode(Device_p, CurrentHandle);

        /* Notify Events */
        if ((ITOpcode & STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK) != 0)
        {
            /* Notify Blit completion as required. */
            STEVT_Notify(  Device_p->EvtHandle,
                        Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                        STEVT_EVENT_DATA_TYPE_CAST &UserTag_p );
        }

        if ((ITOpcode & STBLIT_IT_OPCODE_JOB_COMPLETION_MASK) != 0)
        {
            /* Notify JOB completion as required. */
            STEVT_Notify(  Device_p->EvtHandle,
                        Device_p->EventID[STBLIT_JOB_COMPLETED_ID],
                        STEVT_EVENT_DATA_TYPE_CAST &UserTag_p );
        }

#ifdef STBLIT_DEBUG_GET_STATISTICS
        /* Get AccessControl */
        STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

        /* Update statistics structure */
        Device_p->stblit_Statistics.BlitCompletionInterruptions++;
        Device_p->ExecutionEndTime = STOS_time_now();

#ifndef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
        /* Calculate STBLIT statistics */
        stblit_CalculateStatistics(Device_p);
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */

        /* Release AccessControl */
        STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */

      }
   }
}
#else /* !STBLIT_LINUX_FULL_USER_VERSION */
void stblit_InterruptProcess (void* data_p)
{
    stblit_Device_t*    Device_p;
    void*               UserTag_p;
    STEVT_SubscriberID_t SubscriberID;
    U32 ITOpcode;
    STBLIT_JobHandle_t  JobHandle;
    stblit_NodeHandle_t  CurrentHandle;
    BOOL  AQ1_Node_Notif, AQ1_LNA_Reached, AQ1_LNA_Stoped;
    stblit_ItStatus     ItStatus;

#if !defined(MODULE) && defined(PTV)
    Device_p = (stblit_Device_t*) (((PK_ThreadData *)data_p)->param);
#else
    Device_p =  (stblit_Device_t*)data_p;
#endif /* !MODULE && PTV */

    while(TRUE)
    {
        /* Wait for Blitter Interrupt */
        STOS_SemaphoreWait(Device_p->InterruptProcessSemaphore);
#ifdef BLIT_DEBUG
        PrintTrace(("received the interrupt\n"));
#endif


#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
        /* Take access control */
        STOS_SemaphoreWait(Device_p->BlitterErrorCheck_AccessControlSemaphore);

        if(Device_p->BlitterErrorCheck_WaitingForInterrupt && !Device_p->BlitterErrorCheck_GotOneInterrupt)
        {
            /* Notify stblit_BlitterErrorCheckProcess for just one interrupt */
            Device_p->BlitterErrorCheck_GotOneInterrupt = TRUE;

            /* Release accesss control */
            STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_AccessControlSemaphore);

            /* Notify stblit_BlitterErrorCheckProcess */
            STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_InterruptSemaphore);
        }
        else
        {
            /* Release accesss control */
            STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_AccessControlSemaphore);
        }
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */


        ItStatus.ITS     = 0;
        ItStatus.AQ1_STA = 0;
        /* Pop Interrupt Status from the FIFO */
        PopInterruptCommand(&(Device_p->InterruptsBuffer), &ItStatus);

        /* Calculate status */
        AQ1_Node_Notif = ( ((ItStatus.ITS & BLT_ITS_AQ1_NODE_NOTIF_MASK) >> BLT_ITS_AQ1_NODE_NOTIF_SHIFT)==1 );
        AQ1_LNA_Reached = ( ((ItStatus.ITS & BLT_ITS_AQ1_LNA_REACHED_MASK) >> BLT_ITS_AQ1_LNA_REACHED_SHIFT)==1 );
        AQ1_LNA_Stoped = ( ((ItStatus.ITS & BLT_ITS_AQ1_STOPPED_MASK) >> BLT_ITS_AQ1_STOPPED_SHIFT)==1 );

        if (Device_p->TaskTerminate == TRUE)
        {
            break;
        }


        /* Extract data */
        CurrentHandle = (stblit_NodeHandle_t) ItStatus.AQ1_STA;

        /* Overwrite Support */
        UserTag_p = (void*)((stblit_Node_t*)CurrentHandle)->UserTag_p;

        SubscriberID=((stblit_Node_t*)CurrentHandle)->SubscriberID;
        ITOpcode =((stblit_Node_t*)CurrentHandle)->ITOpcode;
        JobHandle = ((stblit_Node_t*)CurrentHandle)->JobHandle;

        /* Get the virtual adress of BLT_NIP which is the first node at the current list (In the case of OS21 there is only one AQ list Supported) */
        Device_p->FirstNodeTab[0] = (stblit_Node_t*)stblit_DeviceToCpu(( ( (stblit_Node_t*)CurrentHandle)->HWNode.BLT_NIP), &(Device_p->VirtualMapping) );

        /* Take access control */
        STOS_SemaphoreWait(Device_p->AccessControl);

#ifdef ST_OSWINCE
        if ( AQ1_Node_Notif)
        {
            // Free up all processed nodes, skipping the "job" ones
            stblit_Node_t* NodeToFree_p;
            stblit_Node_t* NextContinuationNode_p;

            STOS_SemaphoreWait(Device_p->IPSemaphore);
            // Re-Open the job
			if (JobHandle != STBLIT_NO_JOB_HANDLE)
				((stblit_Job_t*)JobHandle)->Closed = FALSE;

            NextContinuationNode_p = (U32)stblit_DeviceToCpu((((stblit_Node_t*)CurrentHandle)->HWNode.BLT_NIP), &(Device_p->VirtualMapping) );

            // compute where to begin releasing
            if (JobHandle == STBLIT_NO_JOB_HANDLE)
                NodeToFree_p = CurrentHandle;
            else
                NodeToFree_p = NextContinuationNode_p->PreviousNode;

            // Free up nodes until beginning of AQ1 is reached
            while (NodeToFree_p != NULL)
            {
                stblit_Node_t* PreviousNode_p = NodeToFree_p->PreviousNode;

                // the first node of a job is actually a single blit continuation node
                NodeToFree_p->JobHandle = STBLIT_NO_JOB_HANDLE;
                stblit_ReleaseNode(Device_p, NodeToFree_p);

                NodeToFree_p = PreviousNode_p;
            }

            // The next continuation node is now the head of AQ1
            NextContinuationNode_p->PreviousNode = STBLIT_NO_NODE_HANDLE;
            STOS_SemaphoreSignal(Device_p->IPSemaphore);
		}
#else /* !ST_OSWINCE */
         /* Release Nodes if required */

        if ( AQ1_Node_Notif )
        {
            STOS_SemaphoreWait(Device_p->IPSemaphore);
            stblit_ReleaseListNode(Device_p, CurrentHandle);
            STOS_SemaphoreSignal(Device_p->IPSemaphore);
        }
#endif /* !ST_OSWINCE */

        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->AccessControl);

        /* Notify Events */
        if ( AQ1_Node_Notif )
        {
#ifdef STBLIT_ENABLE_BASIC_TRACE
            STBLIT_TRACE("\r\n[I%d] U=%s H=%x", ++Device_p->NbBltInt, UserTag_p, (U32)CurrentHandle);
#endif
            if ((ITOpcode & STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK) != 0)
            {
                /* Notify Blit completion as required. */
#ifdef ST_OSLINUX
                STEVT_Notify(  Device_p->EvtHandle,
                            Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                            STEVT_EVENT_DATA_TYPE_CAST &UserTag_p );
#else
#if 0 /*yxl 2007-06-19 temp modify for test*/
                STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID], STEVT_EVENT_DATA_TYPE_CAST &UserTag_p, SubscriberID);
#else
                STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID], STEVT_EVENT_DATA_TYPE_CAST UserTag_p, SubscriberID);

#endif
#endif
            }
            if ((ITOpcode & STBLIT_IT_OPCODE_JOB_COMPLETION_MASK) != 0)
            {
                /* Notify JOB completion as required. */
#ifdef ST_OSLINUX
                STEVT_Notify(  Device_p->EvtHandle,
                           Device_p->EventID[STBLIT_JOB_COMPLETED_ID],
                          STEVT_EVENT_DATA_TYPE_CAST &UserTag_p );
#else
                STEVT_NotifySubscriber( Device_p->EvtHandle,Device_p->EventID[STBLIT_JOB_COMPLETED_ID],  STEVT_EVENT_DATA_TYPE_CAST &UserTag_p,
                                        ((stblit_Job_t*)JobHandle)->SubscriberID);
#endif
            }

#ifdef STBLIT_DEBUG_GET_STATISTICS
            /* Get AccessControl */
            STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);


            /* Update statistics structure */
            Device_p->stblit_Statistics.BlitCompletionInterruptions++;
            Device_p->ExecutionEndTime = STOS_time_now();

#ifndef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
            /* Calculate STBLIT statistics */
            stblit_CalculateStatistics(Device_p);
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */

            /* Release AccessControl */
            STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */
        }


        /* Code for sync chip : Unlock if waiting node is 0 */
        if ( (Device_p->SBlitNodeDataPool.NbFreeElem >= (Device_p->SBlitNodeDataPool.NbElem - 1) ) && (Device_p->StartWaitBlitComplete == TRUE) )
        {
            STOS_SemaphoreSignal( Device_p->AllBlitsCompleted_p );
            Device_p->StartWaitBlitComplete = FALSE ;
        }
    }
}
#endif  /* STBLIT_LINUX_FULL_USER_VERSION */ /* ST_ZEUS */



#ifdef STBLIT_HW_BLITTER_RESET_DEBUG

/*******************************************************************************
Name        : stblit_BlitterErrorCheckProcess
Description : Process launched by stblit_PostSubmitMessage :
                + Reset blitter when no interruption is detected during STBLIT_HW_BLITTER_RESET_CHECK_TIME_OUT

Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_BlitterErrorCheckProcess (void* data_p)
{
    stblit_Device_t*        Device_p;
    STOS_Clock_t            TimeOut;
    ST_ErrorCode_t          Err;
    stblit_NodeHandle_t     NodeHandle;
    stblit_Node_t*          FirstNode_p;
    U32                     NullValue = 0;
    U32                     NIP_Value;
    STBLIT_JobHandle_t      JobHandle;
    U32                     AQSubmit = 0;
#ifndef ST_OSLINUX
    STEVT_SubscriberID_t    SubscriberID;
#endif



   #if !defined(MODULE) && defined(PTV)
       Device_p = (stblit_Device_t*)(((PK_ThreadData *)data_p)->param);
   #else
       Device_p = (stblit_Device_t*) data_p;
   #endif /* !MODULE && PTV */

#ifdef BLIT_DEBUG
    PrintTrace(("Entering stblit_BlitterErrorCheckProcess()...........\n"));
#endif

    /* Get the physic value of 0, to be used later in this function */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
            if ( PhysicJobBlitNodeBuffer_p != 0 )
            {
                NullValue = (U32) Device_p->JobBlitNodeBuffer_p - (U32) PhysicJobBlitNodeBuffer_p;
            }

            else
            {
                NullValue = (U32) Device_p->SingleBlitNodeBuffer_p - (U32) PhysicSingleBlitNodeBuffer_p;
            }

#else
            NullValue = (U32)stblit_DeviceToCpu( NullValue, &(Device_p->VirtualMapping));
#endif

    while(TRUE)
    {
        /* Wait for PostSubmitMessage notification */
        STOS_SemaphoreWait(Device_p->BlitterErrorCheck_SubmissionSemaphore);

        if (Device_p->TaskTerminate == TRUE)
        {
            break;
        }

        /* Take access control */
        STOS_SemaphoreWait(Device_p->BlitterErrorCheck_AccessControlSemaphore);

        /* Start interrupt waiting */
        Device_p->BlitterErrorCheck_WaitingForInterrupt = TRUE;
        Device_p->BlitterErrorCheck_GotOneInterrupt     = FALSE;

#ifdef BLIT_DEBUG
        PrintTrace(("\n>Proc_GotSubNot"));
#endif

        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_AccessControlSemaphore);

        TimeOut = STOS_time_plus( STOS_time_now(), ST_GetClocksPerSecond() * STBLIT_HW_BLITTER_RESET_CHECK_TIME_OUT );

        /* Wait for interrupt notification */
        Err = STOS_SemaphoreWaitTimeOut(Device_p->BlitterErrorCheck_InterruptSemaphore, &TimeOut);

        /* Detect blitter error */
        if (Err != ST_NO_ERROR)
        {
            /* Lock the access to the STBLIT_NUM_AQS Application Queue list(s), After this point, no other process can insert nodes into AQ(s)*/
            for(AQSubmit = 0; AQSubmit < STBLIT_NUM_AQS; AQSubmit++)
            {
                STOS_SemaphoreWait(Device_p->AQInsertionSemaphore[AQSubmit]);
            }

#ifdef BLIT_DEBUG
            PrintTrace(("\n>Proc_Reset"));
#endif

            /* Reset Blitter Hw */
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL), BLT_CTL_RESET_SEQ1);
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_CTL), BLT_CTL_RESET_SEQ2);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
            /* Un-install interrupt */
#ifdef PTV
            ioctl( Device_p->BLIT_DevNum, STBLIT_IOCTL_UNINSTALL_INT, NULL);
#else
            ioctl( fileno(Device_p->BLIT_DevFile), STBLIT_IOCTL_UNINSTALL_INT, NULL);
#endif
            /* install interrupt */
#ifdef PTV
            if (Err = ioctl( Device_p->BLIT_DevNum, STBLIT_IOCTL_INSTALL_INT_ONLY, NULL) != ST_NO_ERROR )
#else
            if ( Err = ioctl( fileno(Device_p->BLIT_DevFile), STBLIT_IOCTL_INSTALL_INT_ONLY, NULL) != ST_NO_ERROR )
#endif
            {
                Err = ST_ERROR_INTERRUPT_INSTALL;
            }

#else /* !STBLIT_LINUX_FULL_USER_VERSION */

#ifdef ST_OSLINUX
            Err = STOS_InterruptUninstall( Device_p->BlitInterruptNumber, Device_p->BlitInterruptLevel, (void *)Device_p);
#endif
            /* install interrupt */
            Err = STOS_InterruptInstall( Device_p->BlitInterruptNumber,
                        Device_p->BlitInterruptLevel,
                        stblit_BlitterInterruptHandler,
                        "BLITTER",
                        (void *) Device_p );
            if ( Err )
            {
                Err = ST_ERROR_INTERRUPT_INSTALL;
            }
#endif

            for(AQSubmit = 0; AQSubmit < STBLIT_NUM_AQS; AQSubmit++)
            {
                /* Take access control */
                STOS_SemaphoreWait(Device_p->AccessControl);

                /* Get the first node in the AQ[i] */
                FirstNode_p = Device_p->FirstNodeTab[AQSubmit];

                /* Release accesss control */
                STOS_SemaphoreSignal(Device_p->AccessControl);

                /* Test if the list AQSubmit contain pendig nodes*/
                if(FirstNode_p)
                {
                    /* Find the first continuation node in the AQSubmit list without counting the first Node, start finding from the second node in the list */
                    NodeHandle = (stblit_NodeHandle_t) FirstNode_p;
                    JobHandle = ((stblit_Node_t*)NodeHandle)->JobHandle;

                    do
                    {

#ifdef STBLIT_LINUX_FULL_USER_VERSION

                        /* Get the virtual adress of BLT_NIP*/
                        if (JobHandle == STBLIT_NO_JOB_HANDLE)
                        {
                            NodeHandle = (U32)Device_p->SingleBlitNodeBuffer_p + (U32)(((stblit_Node_t *)NodeHandle)->HWNode.BLT_NIP) - (U32)PhysicSingleBlitNodeBuffer_p;
                        }
                        else
                        {
                            NodeHandle = (U32) Device_p->JobBlitNodeBuffer_p + (U32)(((stblit_Node_t *)NodeHandle)->HWNode.BLT_NIP) - (U32) PhysicJobBlitNodeBuffer_p;
                        }
#else
                        NodeHandle = (U32)stblit_DeviceToCpu(((stblit_Node_t *)NodeHandle)->HWNode.BLT_NIP, &(Device_p->VirtualMapping));
#endif
                     } while (((stblit_Node_t*)NodeHandle)->IsContinuationNode == FALSE);

                    /* Release Nodes if required */
                    stblit_ReleaseListNode(Device_p, ((stblit_Node_t *)NodeHandle)->PreviousNode);

                    /*Signal Event for Blitter error*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
                    /* Notify Blitter HW as required */
                    STEVT_Notify(  Device_p->EvtHandle,
                                Device_p->EventID[STBLIT_HARDWARE_ERROR_ID],
                                NULL );
#elif ST_OSLINUX
                    STEVT_Notify(  Device_p->EvtHandle,
                                    Device_p->EventID[STBLIT_HARDWARE_ERROR_ID],
                                    NULL );
#else
                    STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_HARDWARE_ERROR_ID], NULL, SubscriberID);
#endif

                    /*Start NodeInsertion if there is pending node(s) to run*/
                    /* Get the Phy adress of FirstContinuationNode_p->HWNode.BLT_NIP*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
                    JobHandle = ((stblit_Node_t*)NodeHandle)->JobHandle;
                    if (JobHandle == STBLIT_NO_JOB_HANDLE)
                    {
                        NIP_Value = (U32) Device_p->SingleBlitNodeBuffer_p + (U32)((stblit_Node_t*)NodeHandle)->HWNode.BLT_NIP - (U32) PhysicSingleBlitNodeBuffer_p;
                    }
                    else
                    {
                        NIP_Value = (U32) Device_p->JobBlitNodeBuffer_p + (U32)((stblit_Node_t*)NodeHandle)->HWNode.BLT_NIP - (U32) PhysicJobBlitNodeBuffer_p;
                    }
#else
                    NIP_Value = (U32)stblit_DeviceToCpu((((stblit_Node_t*)NodeHandle)->HWNode.BLT_NIP), &(Device_p->VirtualMapping));
#endif

                    if (  NIP_Value != NullValue )
                    {
                        /* There is node(s) for insertion */
                        Device_p->FirstNodeTab[AQSubmit] = (stblit_Node_t*)NodeHandle;
                        stblit_InsertNodesAtTheFront( Device_p, ((stblit_Node_t*)NodeHandle), (stblit_Node_t*)(Device_p->ContinuationNodeTab[AQSubmit]->PreviousNode), AQSubmit, FALSE );
                        stblit_WriteLastNodeAdress(Device_p,(stblit_Node_t*)(Device_p->ContinuationNodeTab[AQSubmit]->PreviousNode), AQSubmit );
                    }
                    else
                    {
                        /* Release Nodes if required */
                        stblit_ReleaseListNode(Device_p, NodeHandle);
                        Device_p->ContinuationNodeTab[AQSubmit]     = NULL;
                        Device_p->AQFirstInsertion[AQSubmit]        = TRUE;
                        Device_p->FirstNodeTab[AQSubmit]            = NULL;
                    }
                }
            }

            /* After this point, the other process can insert nodes into AQ1*/
            for(AQSubmit = 0; AQSubmit < STBLIT_NUM_AQS; AQSubmit++)
            {
                    STOS_SemaphoreSignal(Device_p->AQInsertionSemaphore[AQSubmit]);
            }
        }

        /* There isn't a Blitter error */
        else
        {
#ifdef BLIT_DEBUG
            PrintTrace(("\n>Proc_GotInterruptNot"));
#endif
        }

        /* Take access control */
        STOS_SemaphoreWait(Device_p->BlitterErrorCheck_AccessControlSemaphore);

        /* Finish interrupt waiting */
        Device_p->BlitterErrorCheck_WaitingForInterrupt = FALSE;

        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_AccessControlSemaphore);
    }
}
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */


#ifdef ST_OSWINCE
/*******************************************************************************
Name        : stblit_PostSubmitMessage
Description : SUBMIT list nodes in the application queue
Parameters  : The structure associated with the device , the address of the first
              node of the list to insert and the last node of the list to submit.
Assumptions : Valid params (non null pointer)
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_PostSubmitMessage(stblit_Device_t * const Device_p,
                                        void* FirstNode_p, void* LastNode_p,
                                        BOOL HightPriorityNodes,
                                        BOOL LockBeforeFirstNode )
{
    ST_ErrorCode_t Err = ST_NO_ERROR;
    stblit_Node_t* LastNodeAddress;        // written to AQ1_LNA
    stblit_Node_t* NextContinuationNode_p; // pointed to by AQ1_LNA->NIP
    U32            AQSubmit = 0;

    U32 Tmp;

#ifndef STBLIT_SYNCHRONE_MODE
        STOS_SemaphoreWait(Device_p->IPSemaphore);
#endif

#ifdef STBLIT_DEBUG_GET_STATISTICS
    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

    /* Update statistics structure */
    Device_p->GenerationEndTime = STOS_time_now();

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */


#ifndef ST_OSLINUX
    if (0)
        stblit_DumpRegistersAndNodes(Device_p, FirstNode_p,LastNode_p);
#endif

// JLX begin
    if (DumpNodes)
        STBLIT_DumpNodes(FirstNode_p,LastNode_p);
// JLX end

    if (   ((stblit_Node_t*)FirstNode_p)->JobHandle != STBLIT_NO_JOB_HANDLE /* it's a job */
        || Device_p->ContinuationNodeTab[AQSubmit] == NULL /* Single Blit, start of AQ */
       )
    {
        // Allocate next continuation node
        Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) &NextContinuationNode_p);
        if (Err != ST_NO_ERROR)
            return STBLIT_ERROR_MAX_SINGLE_BLIT_NODE;
    }
    else
        NextContinuationNode_p = (stblit_Node_t*)FirstNode_p; // user-provided single-blit node

    // Begin programming the BDisp2
    if (Device_p->ContinuationNodeTab[AQSubmit] == NULL)
    {
        // first programmation of AQ1
        U32 CTL;

        // Point BLT_AQ1_IP to the first node
#ifdef STBLIT_LINUX_FULL_USER_VERSION
            Tmp = (U32)stblit_CpuToDevice((void*)FirstNode_p);
#ifdef STBLIT_DEBUG
            PrintTrace(("Upate Next Node of the first node : 0x%x\n", Tmp ));
            PrintTrace(("Address AQ1_IP : 0x%x\n", (U32)Device_p->BaseAddress_p + BLT_AQ1_IP ));
#endif
#else
            Tmp = (U32)stblit_CpuToDevice((void*)FirstNode_p,&(Device_p->VirtualMapping));
#endif
        BLITTER_WRITE_REGISTER((void*)((U32)Device_p->BaseAddress_p + BLT_AQ1_IP), Tmp);

        /* Calculate BLT_CTL register value */
        CTL=0;
        CTL|=((U32)( (COMPLETED_INT_ENA )  & STBLIT_AQCTL_IRQ_MASK      ) << STBLIT_AQCTL_IRQ_SHIFT      );
        CTL|=((U32)( QUEUE_EN           & STBLIT_AQCTL_QUEUE_EN_MASK ) << STBLIT_AQCTL_QUEUE_EN_SHIFT );
        CTL|=((U32)( PRIORITY_1         & STBLIT_AQCTL_PRIORITY_MASK ) << STBLIT_AQCTL_PRIORITY_SHIFT );
        /*-- Note : THis modification causes interruption problems --*/
        /*CTL|=((U32)( ABORT_CUURENT_NODE & STBLIT_EVENT_BEHAV_MASK    ) << STBLIT_EVENT_BEHAV_SHIFT    );*/
        /* Write AQ1 Trigger control */
        BLITTER_WRITE_REGISTER((void*)((U32)Device_p->BaseAddress_p + BLT_AQ1_CTL), CTL);

        // The LNA register will contain the last of the specified nodes
        LastNodeAddress = (stblit_Node_t*)LastNode_p;
    }
    else
    {
        // Append nodes to end of queue but don t overwrite PreviousNode
        stblit_Node_t* previous = Device_p->ContinuationNodeTab[AQSubmit]->PreviousNode;
        memcpy(Device_p->ContinuationNodeTab[AQSubmit], FirstNode_p, sizeof(stblit_Node_t));
        Device_p->ContinuationNodeTab[AQSubmit]->PreviousNode = previous;

        /* Change previous address of node 2 of a "single" blit, if it exists */
        if (((stblit_Node_t*)FirstNode_p)->JobHandle == STBLIT_NO_JOB_HANDLE
            && FirstNode_p != LastNode_p)
        {
            stblit_Node_t* SecondNode_p = stblit_DeviceToCpu((((stblit_Node_t*)FirstNode_p)->HWNode.BLT_NIP), &(Device_p->VirtualMapping) );
            SecondNode_p->PreviousNode = Device_p->ContinuationNodeTab[AQSubmit];
        }

        /* Compute the value of the LNA register */
        if (FirstNode_p == LastNode_p)
            LastNodeAddress = Device_p->ContinuationNodeTab[AQSubmit];
        else
            LastNodeAddress = (stblit_Node_t*)LastNode_p;
    }

    /* Connect the next continuation node to the added nodes */
    if (((stblit_Node_t*)FirstNode_p)->JobHandle != STBLIT_NO_JOB_HANDLE)
        /* job nodes */
        NextContinuationNode_p->PreviousNode = Device_p->ContinuationNodeTab[AQSubmit];
    else
        // single blit nodes
        NextContinuationNode_p->PreviousNode = LastNodeAddress;

    // Make the last node point to the next continuation node
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp = (U32)stblit_CpuToDevice((void*)NextContinuationNode_p);
#else
    Tmp = (U32)stblit_CpuToDevice((void*)NextContinuationNode_p,&(Device_p->VirtualMapping));
#endif
    STSYS_WriteRegMem32LE(&(LastNodeAddress->HWNode.BLT_NIP), Tmp);

    // There must a memory barrier here so the previous write is committed before the AQ is started
    Tmp = STSYS_ReadRegMem32LE(&(LastNodeAddress->HWNode.BLT_NIP));

    // Store continuation node for next submit
    Device_p->ContinuationNodeTab[AQSubmit] = NextContinuationNode_p;

    // program LNA to start executing the nodes
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp = (U32)stblit_CpuToDevice(LastNodeAddress);
#else
    Tmp = (U32)stblit_CpuToDevice(LastNodeAddress, &(Device_p->VirtualMapping));
#endif
    BLITTER_WRITE_REGISTER((void*)((U32)Device_p->BaseAddress_p + BLT_AQ1_LNA), Tmp);

#ifndef STBLIT_SYNCHRONE_MODE
    STOS_SemaphoreSignal(Device_p->IPSemaphore);
#endif

#ifdef STBLIT_DEBUG_GET_STATISTICS
    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

    /* Update statistics structure */
    Device_p->ExecutionStartTime = STOS_time_now();

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */
    return (Err);
}


/*
--------------------------------------------------------------------------------
Submit Job API function
--------------------------------------------------------------------------------
*/
static ST_ErrorCode_t STBLIT_SubmitJob(STBLIT_Handle_t Handle, STBLIT_JobHandle_t JobHandle)
{
    stblit_Unit_t*      Unit_p   = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p;
    stblit_Job_t*       Job_p    = (stblit_Job_t*) JobHandle;
    stblit_Node_t*      FirstNode_p;
    stblit_Node_t*      LastNode_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
        return ST_ERROR_INVALID_HANDLE;

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Job Validity */
    if (JobHandle == STBLIT_NO_JOB_HANDLE)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
        return STBLIT_ERROR_INVALID_JOB_HANDLE;

    /* Check Handles mismatch */
    if (Job_p->Handle != Handle)
        return STBLIT_ERROR_JOB_HANDLE_MISMATCH;

    /* Check if Job empty */
    if (Job_p->NbNodes == 0)
        return STBLIT_ERROR_JOB_EMPTY;

    /* Check if lock pending */
    if (Job_p->PendingLock)
        return STBLIT_ERROR_JOB_PENDING_LOCK;

    Job_p->Closed = TRUE;
    FirstNode_p = (stblit_Node_t*)(Job_p->FirstNodeHandle);
    LastNode_p  = (stblit_Node_t*)(Job_p->LastNodeHandle);

    /* Post Message */
    #ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Unit_p->Device_p, (void*)FirstNode_p, (void*)LastNode_p, FALSE, Job_p->LockBeforeFirstNode, 0);
    #endif

    return ST_NO_ERROR;
}


/*
--------------------------------------------------------------------------------
Submit Job at Front API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SubmitJobFront(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    return STBLIT_SubmitJob(Handle, JobHandle);
}

/*
--------------------------------------------------------------------------------
Submit Job at Back API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SubmitJobBack(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    return STBLIT_SubmitJob(Handle, JobHandle);
}

#else /* !WinCE */

/*******************************************************************************
Name        : stblit_PostSubmitMessage
Description : SUBMIT list nodes in the application queue


Parameters  : The structure associated with the device, the address of the first
              node of the list to insert and the last node of the list to submit.
Assumptions : Valid params (non null pointer)
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_PostSubmitMessage(stblit_Device_t * const Device_p,
                                        void* FirstNode_p, void* LastNode_p,
                                        BOOL HightPriorityNodes,
                                        BOOL LockBeforeFirstNode,
                                        U32  NumberNodes)
{
    ST_ErrorCode_t                      Err = ST_NO_ERROR;
    stblit_Node_t*                      LastNodeAddress;                    /* written to AQ1_LNA */
    stblit_Node_t*                      NextContinuationNode_p;             /* pointed to by AQ1_LNA->NIP */
    U32                                 AQSubmit = 0;                       /* The AQ Number*/
#ifdef STBLIT_DUMP_NODES_LIST
	BOOL                                DumpRegistersAndNodes  = TRUE;     /* =TRUE to dump register */
#else
	BOOL                                DumpRegistersAndNodes  = FALSE;     /* =TRUE to dump register */
#endif /* STBLIT_DUMP_NODES_LIST */

    BOOL                                InsertionAtTheFront;                 /* Insertion at the front of AQ*/
    stblit_Node_t*                      previous;                           /* Store temporary a node*/
    stblit_Node_t*                      SecondNode_p;                       /* Used to store temporary a node*/

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    STBLIT_JobHandle_t                  JobHandle;
#endif
    U32 Tmp;

#ifdef STBLIT_ENABLE_BASIC_TRACE
    U32 Blit_Width, Blit_Height;
#endif


    UNUSED_PARAMETER(NumberNodes);

    STOS_SemaphoreWait(Device_p->IPSemaphore);

   /* Init AQSubmit value by the required AQ number*/

#ifdef STBLIT_DEBUG_GET_STATISTICS
    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

    /* Update statistics structure */
    Device_p->GenerationEndTime = STOS_time_now();

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */


    if(HightPriorityNodes)
    {
       AQSubmit = STBLIT_NUM_AQS - 1;
    }

    /* After this point, no other process can insert nodes into AQ*/
    STOS_SemaphoreWait(Device_p->AQInsertionSemaphore[AQSubmit]);

#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    /* Provoke a blitter error(crash) by programming a bad node(node values=0)*/
#ifdef STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION
    Device_p->NbSubmission++;
    if(Device_p->NbSubmission == STBLIT_HARDWARE_ERROR_SIMULATION_MAX_VALUE)
    {
        /* Reset counter */
        Device_p->NbSubmission = 0;

        /* Set last node to 0 to force blitter error(crash) */
        memset(LastNode_p,0,sizeof(stblit_Node_t));
    }
#endif /* STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION */
#endif

/*Step A- Compute the NextContinuation Node*/
    if(Device_p->AQFirstInsertion[AQSubmit])
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&NextContinuationNode_p;

		Device_p->AQFirstInsertion[AQSubmit] = FALSE;

        /*A.1- If first insertion, allocate a continuation node*/
        /* Single blit node */
        if (((stblit_Node_t*)FirstNode_p)->JobHandle == STBLIT_NO_JOB_HANDLE)
        {

            Err = stblit_GetElement(&(Device_p->SBlitNodeDataPool), (void**) TempAddress);
        }

        /* Job blit Node */
        else
        {
            Err = stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
        }

        /* If there is no much node in the DataPool structure */
        if (Err != ST_NO_ERROR)
	    {
	        if(LastNode_p)
	        {
	             stblit_ReleaseListNode(Device_p, (stblit_NodeHandle_t)LastNode_p);
	        }
            STOS_SemaphoreSignal(Device_p->AQInsertionSemaphore[AQSubmit]);

	        return STBLIT_ERROR_MAX_SINGLE_BLIT_NODE;
	    }

        /*First Insertion at the front*/
        InsertionAtTheFront = TRUE;
    }

    else if(Device_p->ContinuationNodeTab[AQSubmit]->PreviousNode == STBLIT_NO_NODE_HANDLE)
    {
        /* A.2- If AQ is empty, re-use the stored continuation node as a next continuation node*/
        NextContinuationNode_p = Device_p->ContinuationNodeTab[AQSubmit];
        InsertionAtTheFront = TRUE;
    }

    else
    {
        /* A.3- The AQ isn't empty, use provided first node as a NextContinuation node*/
        NextContinuationNode_p = (stblit_Node_t *)FirstNode_p;
        InsertionAtTheFront = FALSE;
    }

    /* Mark this node as a continuation node, this information is used to release bad nodes when the blitter make an error*/
    NextContinuationNode_p->IsContinuationNode     = TRUE;

    if(DumpRegistersAndNodes)
    {
        stblit_DumpRegistersAndNodes(Device_p, FirstNode_p, LastNode_p);
    }

/*Step B: programming the BDisp*/
    if (InsertionAtTheFront)
    {

#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
        /* Store the first node in the current AQ list, used to walk the AQ in case of Blitter error */
        Device_p->FirstNodeTab[AQSubmit] = FirstNode_p;
#endif
        /*B.1- Insertion at the front of the exist list*/
        stblit_InsertNodesAtTheFront( Device_p, FirstNode_p, LastNode_p, AQSubmit, LockBeforeFirstNode );

        /* The LNA register will contain the last of the specified nodes*/
        LastNodeAddress = (stblit_Node_t*)LastNode_p;
    }

    else
    {
        /*B.2- Insertion at the end of the exist list*/
        /* Append nodes to end of queue but don' t overwrite PreviousNode */
        /* copy first node to prev continuation node, put first node as new continuation node at end of list */
        previous = (stblit_Node_t *)Device_p->ContinuationNodeTab[AQSubmit]->PreviousNode;
        memcpy(Device_p->ContinuationNodeTab[AQSubmit], FirstNode_p, sizeof(stblit_Node_t));
        Device_p->ContinuationNodeTab[AQSubmit]->PreviousNode           = (stblit_NodeHandle_t)previous;

        /* Change previous address of node 2 of the new submitted list, if it exists */
        if ( FirstNode_p != LastNode_p )
        {
#ifdef STBLIT_LINUX_FULL_USER_VERSION
            /* Get the node JobHandle: Single or Job*/
            JobHandle = ((stblit_Node_t*)FirstNode_p)->JobHandle;

            /* Get the virtual adress of BLT_NIP which is the first node at the current list */
            if (JobHandle == STBLIT_NO_JOB_HANDLE)
            {
                SecondNode_p = (stblit_Node_t*)((U32) Device_p->SingleBlitNodeBuffer_p + (U32)(((stblit_Node_t*)FirstNode_p)->HWNode.BLT_NIP) - (U32) PhysicSingleBlitNodeBuffer_p);
            }
            else
            {
                SecondNode_p = (stblit_Node_t*)((U32) Device_p->JobBlitNodeBuffer_p + (U32)(((stblit_Node_t*)FirstNode_p)->HWNode.BLT_NIP) - (U32) PhysicJobBlitNodeBuffer_p);
            }
#else
            SecondNode_p = stblit_DeviceToCpu((((stblit_Node_t*)FirstNode_p)->HWNode.BLT_NIP), &(Device_p->VirtualMapping) );
#endif
            SecondNode_p->PreviousNode = (stblit_NodeHandle_t)Device_p->ContinuationNodeTab[AQSubmit];
        }

        /* Compute the value of the LNA register */
        if (FirstNode_p == LastNode_p)
        {
            LastNodeAddress = Device_p->ContinuationNodeTab[AQSubmit];
        }
        else
        {
            LastNodeAddress = (stblit_Node_t*)LastNode_p;
        }
    }

/*Step C: Add Continuation Node at the end of the list and store it for the next submission */
   /* C.1- Connect the next continuation node to the added nodes */
    stblit_Connect2Nodes(Device_p, LastNodeAddress, NextContinuationNode_p);

    /* C.2- There must a memory barrier here so the previous write is committed before the AQ is started */
    Tmp = STSYS_ReadRegMem32LE(&(LastNodeAddress->HWNode.BLT_NIP));

    /* C.3- Store continuation node for next submit */
    Device_p->ContinuationNodeTab[AQSubmit] = NextContinuationNode_p;


/*Step D: Launch the IP Blitter */
    /* D.1- program LNA to start executing the nodes*/
    stblit_WriteLastNodeAdress(Device_p, LastNodeAddress, AQSubmit);


/* this part of code concern the Blitter error simulation*/
#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    /* Take access control */
    STOS_SemaphoreWait(Device_p->BlitterErrorCheck_AccessControlSemaphore);

    if(!Device_p->BlitterErrorCheck_WaitingForInterrupt)
    {
        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_AccessControlSemaphore);

        /* Notify stblit_BlitterErrorCheckProcess process */
        STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_SubmissionSemaphore);
    }
    else
    {
        /* Release accesss control */
        STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_AccessControlSemaphore);
    }
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */

#ifdef STBLIT_ENABLE_BASIC_TRACE
    Blit_Width  = (((((stblit_Node_t*)LastNodeAddress)->HWNode.BLT_T_S1_SZ) >> STBLIT_T_S1_SZ_WIDTH_SHIFT) & STBLIT_T_S1_SZ_WIDTH_MASK);
    Blit_Height = (((((stblit_Node_t*)LastNodeAddress)->HWNode.BLT_T_S1_SZ) >> STBLIT_T_S1_SZ_HEIGHT_SHIFT) & STBLIT_T_S1_SZ_HEIGHT_MASK);

    STBLIT_TRACE("\r\n[P%d] U=%s H=%x F=%d N=%d W=%d H=%d", ++Device_p->NbBltSub, LastNodeAddress->UserTag_p,
                                                            (U32)LastNodeAddress, InsertionAtTheFront, NumberNodes, Blit_Width, Blit_Height);
#endif

    /* After this point, the other process can insert nodes into AQ1*/
    STOS_SemaphoreSignal(Device_p->AQInsertionSemaphore[AQSubmit]);

    STOS_SemaphoreSignal(Device_p->IPSemaphore);
#ifdef STBLIT_DEBUG_GET_STATISTICS
    /* Get AccessControl */
    STOS_SemaphoreWait(Device_p->StatisticsAccessControl_p);

    /* Update statistics structure */
    Device_p->ExecutionStartTime = STOS_time_now();

    /* Release AccessControl */
    STOS_SemaphoreSignal(Device_p->StatisticsAccessControl_p);
#endif /* STBLIT_DEBUG_GET_STATISTICS */

    return (Err);
}

/*******************************************************************************

Name        : stblit_WriteLastNodeAdress
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stblit_WriteLastNodeAdress(stblit_Device_t * const Device_p,
                                        void* LastNode_p, U32 AQSubmit)

 {
    U32             Last;
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif



    /*C- Write last node address into AQ1_LNA */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if ( PhysicJobBlitNodeBuffer_p != 0 )
    {
        Last = (U32) LastNode_p - (U32) Device_p->JobBlitNodeBuffer_p + (U32) PhysicJobBlitNodeBuffer_p;
    }
    else
    {
        Last = (U32) LastNode_p - (U32) Device_p->SingleBlitNodeBuffer_p + (U32) PhysicSingleBlitNodeBuffer_p;
    }

#else
    Last = (U32)stblit_CpuToDevice((void*)LastNode_p,&(Device_p->VirtualMapping));
#endif

#if defined(STBLIT_SINGLE_BLIT_DCACHE_NODES_ALLOCATION)
    stblit_SoftCacheFlushData();
#elif defined(STBLIT_JOB_DCACHE_NODES_ALLOCATION)
    {
        STBLIT_JobHandle_t  JobHandle;
        JobHandle = ((stblit_Node_t*)LastNode_p)->JobHandle;

        if (JobHandle != STBLIT_NO_JOB_HANDLE)
        {
            stblit_SoftCacheFlushData();
        }
    }
#endif


#if defined (DVD_SECURED_CHIP)
    MemoryStatus = STMES_IsMemorySecure((void *)Last, sizeof(stblit_Node_t), 0);
    if(MemoryStatus == SECURE_REGION)
    {
        BLITTER_WRITE_REGISTER((void*)Device_p->AQ_LNA_ADDRESS[AQSubmit], ( STBLIT_SECURE_ON | Last ));
    }
    else
    {
        BLITTER_WRITE_REGISTER((void*)Device_p->AQ_LNA_ADDRESS[AQSubmit], Last);
    }

#else
    BLITTER_WRITE_REGISTER((void*)Device_p->AQ_LNA_ADDRESS[AQSubmit], Last );
#endif

 }

/*******************************************************************************
Name        : stblit_InsertNodesAtTheFront
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void stblit_InsertNodesAtTheFront(stblit_Device_t * const Device_p,
                                        void* FirstNode_p, void* LastNode_p,
                                        U32 AQSubmit,
                                        BOOL LockBeforeFirstNode)
 {
    U32             CTL;
    U32             First;
#if defined (DVD_SECURED_CHIP)
    U8 MemoryStatus;
#endif

    UNUSED_PARAMETER(LastNode_p);
    UNUSED_PARAMETER(LockBeforeFirstNode);


    /* Calculate BLT_CTL register value */
    CTL=0;
    CTL|=((U32)( (COMPLETED_INT_ENA )  & STBLIT_AQCTL_IRQ_MASK      ) << STBLIT_AQCTL_IRQ_SHIFT      );
    CTL|=((U32)( QUEUE_EN           & STBLIT_AQCTL_QUEUE_EN_MASK ) << STBLIT_AQCTL_QUEUE_EN_SHIFT );
    CTL|=((U32)( AQSubmit           & STBLIT_AQCTL_PRIORITY_MASK ) << STBLIT_AQCTL_PRIORITY_SHIFT );


    /*-- Note : THis modification causes interruption problems --*/
    /*CTL|=((U32)( ABORT_CUURENT_NODE & STBLIT_EVENT_BEHAV_MASK    ) << STBLIT_EVENT_BEHAV_SHIFT    );*/

    /* Upate Next Node of the first node*/
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if ( PhysicJobBlitNodeBuffer_p != 0 )
    {
        First =  (U32) FirstNode_p - (U32) Device_p->JobBlitNodeBuffer_p + (U32) PhysicJobBlitNodeBuffer_p;
    }
    else
    {
        First  = (U32) FirstNode_p - (U32) Device_p->SingleBlitNodeBuffer_p + (U32) PhysicSingleBlitNodeBuffer_p;
    }
#else
    {
        First = (U32)stblit_CpuToDevice((void*)FirstNode_p,&(Device_p->VirtualMapping));
    }
#endif

#if defined (DVD_SECURED_CHIP)
    MemoryStatus = STMES_IsMemorySecure((void *)First, sizeof(stblit_Node_t), 0);
    if(MemoryStatus == SECURE_REGION)
    {
        BLITTER_WRITE_REGISTER((void*)(Device_p->AQ_IP_ADDRESS[AQSubmit]), (STBLIT_SECURE_ON | First));
    }
    else
    {
        BLITTER_WRITE_REGISTER((void*)(Device_p->AQ_IP_ADDRESS[AQSubmit]), First);
    }

#else
        BLITTER_WRITE_REGISTER((void*)(Device_p->AQ_IP_ADDRESS[AQSubmit]), First);
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
#ifdef BLIT_DEBUG
    PrintTrace(("OLD :Upate Next Node of the first node : 0x%x\n", FirstNode_p ));
    PrintTrace(("NEW :Upate Next Node of the first node : 0x%x\n", First ));
    PrintTrace(("Address BLT_AQ1_IP : 0x%x\n", Device_p->AQ_IP_ADDRESS[AQSubmit] ));
    PrintTrace(("Address BLT_AQ1_CTL : 0x%x\n", Device_p->AQ_CTL_ADDRESS[AQSubmit] ));
    PrintTrace(("CTL DATA : 0x%x\n", (U32)CTL ));
#endif
#endif

    /* Write AQ1 Trigger control */
    BLITTER_WRITE_REGISTER((void*)(Device_p->AQ_CTL_ADDRESS[AQSubmit]), CTL);

 }

/*
Submit Job at Front API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SubmitJobFront(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t      Err      = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p   = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p ;
    stblit_Job_t*       Job_p    = (stblit_Job_t*) JobHandle;
    stblit_Node_t*      FirstNode_p;
    stblit_Node_t*      LastNode_p;
    stblit_NodeHandle_t  NodeHandle, LeftHandle, RightHandle, CurrentNodeHandle;


    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Job Validity */
    if (JobHandle == STBLIT_NO_JOB_HANDLE)
    {
        return(STBLIT_ERROR_INVALID_JOB_HANDLE);
    }
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
    {
        return(STBLIT_ERROR_INVALID_JOB_HANDLE);
    }

    /* Check Handles mismatch */
    if (Job_p->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
    }

    /* Check if Job empty */
    if (Job_p->NbNodes == 0)
    {
        return(STBLIT_ERROR_JOB_EMPTY);
    }

    /* Check if lock pending */
    if (Job_p->PendingLock == TRUE)
    {
        return(STBLIT_ERROR_JOB_PENDING_LOCK);
    }

    /* The original Job is not submitted to the hw. It is in fact its nodes copy which is submitted */

    /* Start proctection for pool access control*/
    STOS_SemaphoreWait(Device_p->AccessControl);

    /* Make sure there is enough nodes in the pool for the copy */
    if (Job_p->NbNodes > Device_p->JBlitNodeDataPool.NbFreeElem)
    {
        /* Stop protection */
        STOS_SemaphoreSignal(Device_p->AccessControl);

        return(STBLIT_ERROR_MAX_JOB_NODE);
    }

    if(Job_p->NbNodes == 1)
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&NodeHandle;

        /* Get a Node */
        stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);

        /* Copy The node */
        *((stblit_Node_t*)NodeHandle) =   *((stblit_Node_t*)Job_p->FirstNodeHandle);

        FirstNode_p = (stblit_Node_t*)NodeHandle;
        LastNode_p = (stblit_Node_t*) NodeHandle;
    }
    else /* At least 2 nodes */
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&RightHandle;

		/* Get a Node */
        stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
        /* Copy The node */
        *((stblit_Node_t*)RightHandle) =  *((stblit_Node_t*)Job_p->LastNodeHandle);

        LastNode_p = (stblit_Node_t*)RightHandle;
        CurrentNodeHandle = Job_p->LastNodeHandle;

        while(CurrentNodeHandle != Job_p->FirstNodeHandle) /* There is at least 1 node on the left side of the current node */
        {
			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&LeftHandle;

			/* Get a Node */
            stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
            /* Copy The node */
            *((stblit_Node_t*)LeftHandle) =  *((stblit_Node_t*)(((stblit_Node_t*)CurrentNodeHandle)->PreviousNode));

            stblit_Connect2Nodes(Device_p,(stblit_Node_t*)LeftHandle,(stblit_Node_t*) RightHandle);

            CurrentNodeHandle = ((stblit_Node_t*)CurrentNodeHandle)->PreviousNode;
            RightHandle = LeftHandle;
        }
        FirstNode_p = (stblit_Node_t*)LeftHandle;
    }

    /* Stop proctection */
    STOS_SemaphoreSignal(Device_p->AccessControl);

    Job_p->Closed = TRUE;

    /* Post Message */
    #ifndef STBLIT_TEST_FRONTEND
    stblit_PostSubmitMessage(Unit_p->Device_p, (void*)FirstNode_p, (void*)LastNode_p, TRUE, Job_p->LockBeforeFirstNode, 0);
    #endif

    return(Err);
}

/*
--------------------------------------------------------------------------------
Submit Job at Back API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_SubmitJobBack(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    ST_ErrorCode_t      Err      = ST_NO_ERROR;
    stblit_Unit_t*      Unit_p   = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p;
    stblit_Job_t*       Job_p    = (stblit_Job_t*) JobHandle;
    stblit_Node_t*      FirstNode_p;
    stblit_Node_t*      LastNode_p;
    stblit_NodeHandle_t  NodeHandle, LeftHandle, RightHandle, CurrentNodeHandle;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check Job Validity */
    if (JobHandle == STBLIT_NO_JOB_HANDLE)
    {
        return(STBLIT_ERROR_INVALID_JOB_HANDLE);
    }
    if (Job_p->JobValidity != STBLIT_VALID_JOB)
    {
        return(STBLIT_ERROR_INVALID_JOB_HANDLE);
    }

    /* Check Handles mismatch */
    if (Job_p->Handle != Handle)
    {
        return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
    }

    /* Check if Job empty */
    if (Job_p->NbNodes == 0)
    {
        return(STBLIT_ERROR_JOB_EMPTY);
    }

    /* Check if lock pending */
    if (Job_p->PendingLock == TRUE)
    {
        return(STBLIT_ERROR_JOB_PENDING_LOCK);
    }


    /* The original Job is not submitted to the hw. It is in fact its nodes copy which is submitted */

    /* Start proctection for access control*/
    STOS_SemaphoreWait(Device_p->AccessControl);

    /* Make sure there is enough nodes in the pool for the copy */
    if (Job_p->NbNodes > Device_p->JBlitNodeDataPool.NbFreeElem)
    {
        /* Stop proctection */
        STOS_SemaphoreSignal(Device_p->AccessControl);

        return(STBLIT_ERROR_MAX_JOB_NODE);
    }

    if(Job_p->NbNodes == 1)
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&NodeHandle;

		/* Get a Node */
        stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);

        /* Copy The node */
        *((stblit_Node_t*)NodeHandle) =   *((stblit_Node_t*)Job_p->FirstNodeHandle);

        FirstNode_p = (stblit_Node_t*)NodeHandle;
        LastNode_p = (stblit_Node_t*) NodeHandle;
    }
    else /* At least 2 nodes */
    {
		U32 TempAddress;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&RightHandle;

        /* Get a Node */
        stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
        /* Copy The node */
        *((stblit_Node_t*)RightHandle) =  *((stblit_Node_t*)Job_p->LastNodeHandle);

        LastNode_p = (stblit_Node_t*)RightHandle;
        CurrentNodeHandle = Job_p->LastNodeHandle;

        while(CurrentNodeHandle != Job_p->FirstNodeHandle) /* There is at least 1 node on the left side of the current node */
        {
			/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
			TempAddress = (U32)&LeftHandle;

			/* Get a Node */
            stblit_GetElement(&(Device_p->JBlitNodeDataPool), (void**) TempAddress);
            /* Copy The node */
            *((stblit_Node_t*)LeftHandle) =  *((stblit_Node_t*)(((stblit_Node_t*)CurrentNodeHandle)->PreviousNode));

            stblit_Connect2Nodes(Device_p,(stblit_Node_t*)LeftHandle,(stblit_Node_t*) RightHandle);

            CurrentNodeHandle = ((stblit_Node_t*)CurrentNodeHandle)->PreviousNode;
            RightHandle = LeftHandle;
        }
        FirstNode_p = (stblit_Node_t*)LeftHandle;
    }

    /* Stop proctection */
    STOS_SemaphoreSignal(Device_p->AccessControl);

    Job_p->Closed = TRUE;

    /* Post Message */
    #ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Unit_p->Device_p, (void*)FirstNode_p, (void*)LastNode_p, FALSE, Job_p->LockBeforeFirstNode, 0);
    #endif

    return(Err);
}

#endif /* !ST_OSWINCE */

/*
--------------------------------------------------------------------------------
Lock API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Lock(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(JobHandle);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*
--------------------------------------------------------------------------------
Unlock API function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Unlock(STBLIT_Handle_t  Handle, STBLIT_JobHandle_t JobHandle)
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(JobHandle);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/* End of bdisp_be.c */

