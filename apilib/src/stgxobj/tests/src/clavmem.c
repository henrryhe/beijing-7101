/*******************************************************************************

File name   : clavmem.c

Description : STAVMEM initialization source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
09 Apr 2002        Created                                          HSdLM
28 Oct 2002        Hardware or software bug for mb290               BS
20 Nov 2002        Add support of 5517                              HSdLM
17 Apr 2003        Multi-partitions support                         HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>

#define USE_AS_FRONTEND
#include "genadd.h"
#include "stdevice.h"

#include "stddefs.h"
#include "startup.h"
#include "testcfg.h"

#include "stboot.h"
#include "clboot.h" /* for DCacheMap */
/*#if !defined(ST_OSLINUX)  */
#include "sttbx.h"
/*#endif  */
#include "stavmem.h"
#include "clavmem.h"
#include "testtool.h"

#ifdef USE_GPDMA
#include "clgpdma.h"
#endif /*#ifdef USE_GPDMA*/

#ifdef USE_FDMA
#include "clfdma.h"
#endif /*#ifdef USE_FDMA*/


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

#ifdef STAVMEM_DEBUG_MEMORY_STATUS
static BOOL TTAVMEM_GetMemoryStatus(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
#endif
STAVMEM_PartitionHandle_t AvmemPartitionHandle[MAX_PARTITIONS+1];
STAVMEM_MemoryRange_t     SecuredSysMemoryRange;

#if defined(mb390)|| defined(mb400)|| defined(mb457)|| defined(mb428)|| defined(maly3s)|| defined(mb436)|| defined(DTT5107) || defined(SAT5107)
static S32 PartitionsCreated = 1;
#else
static S32 PartitionsCreated = MAX_PARTITIONS;
#endif


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t Term(U32 NumberOfPartitions);
static BOOL Init(U32 NumberOfPartitions);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : TTAVMEM_GetMemoryStatus
Description : Call  STAVMEM_GetMemoryStatus()
Parameters  :
Assumptions :
Limitations :
Returns     : False if success
*******************************************************************************/
#ifdef STAVMEM_DEBUG_MEMORY_STATUS
static BOOL TTAVMEM_GetMemoryStatus(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    S32 LVAR;
    U8 PartitionIndex;
    char MyMsg[128]; /* text for trace */

    UNUSED_PARAMETER(ResultSymbol_p);

    RetErr = STTST_GetInteger( Parse_p, 0, &LVAR);

    if ( (RetErr) || (LVAR < 0) || (LVAR > MAX_PARTITIONS) )
    {
        sprintf(MyMsg, "Expected partition index (default 0, Max value is %d)", (MAX_PARTITIONS-1));
        STTST_TagCurrentLine( Parse_p, MyMsg);
        return(TRUE);
    }
    PartitionIndex = LVAR;

    ErrorCode = STAVMEM_GetMemoryStatus(STAVMEM_DEVICE_NAME, NULL, NULL, PartitionIndex );
     
     #ifdef ST_OSWINCE
        ErrorCode = STAVMEM_GetMemoryStatus(STAVMEM_DEVICE_NAME, NULL, NULL);
     #else	
        ErrorCode = STAVMEM_GetMemoryStatus(STAVMEM_DEVICE_NAME, NULL, NULL, 0 );
    #endif
    if (ErrorCode == ST_NO_ERROR)
    {
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
 }
#endif

/*******************************************************************************
Name        : TTAVMEM_Init
Description : Call  AVMEM_Init()
Parameters  :
Assumptions :
Limitations :
Returns     : False if success
*******************************************************************************/
static BOOL TTAVMEM_Init(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    BOOL RetErr;
    BOOL bOK;
    S32 LVAR;
    UNUSED_PARAMETER(ResultSymbol_p);

#if defined(mb390) || defined(mb400) || defined(mb457) || defined(mb428)|| defined(mb436) || defined(DTT5107) || defined(SAT5107)
    /* With mb390 platform, by default we initialize only one partition.
     * A second partition is only initialised to test STVID_Setup function */
    RetErr = STTST_GetInteger( Parse_p, 1, &LVAR);
#else
    RetErr = STTST_GetInteger( Parse_p, MAX_PARTITIONS, &LVAR);
#endif

    if ( (RetErr) || (LVAR < 0))
    {
        STTST_TagCurrentLine( Parse_p, "Expected number of partitions (default Max partitions)" );
        return(TRUE);
    }
    bOK = Init(LVAR);
    if (bOK)
    {
        PartitionsCreated = LVAR;
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}

/*******************************************************************************
Name        : TTAVMEM_Term
Description : Call  AVMEM_Term()
Parameters  :
Assumptions :
Limitations :
Returns     : False if success
*******************************************************************************/
static BOOL TTAVMEM_Term(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    ST_ErrorCode_t ErrorCode;
    UNUSED_PARAMETER(ResultSymbol_p);
    UNUSED_PARAMETER(Parse_p);

    ErrorCode = Term(PartitionsCreated);
    if (ErrorCode == ST_NO_ERROR)
    {
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
 }

/*******************************************************************************
Name        : AVMEM_Init
Description : Initialise and open AVMEM
Parameters  : Number Of Partitions To Be Created
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL AVMEM_Init()
{
#if defined(mb390) || defined(mb400) || defined(mb457) || defined(mb428)|| defined(mb436) || defined(DTT5107) || defined(SAT5107)
    /* With mb390 platform, by default we initialize only 1 partition.
     * A second partition is only initialised to test STVID_Setup function */
    return(Init(1));
#else
    return(Init(MAX_PARTITIONS));
#endif
} /* end AVMEM_Init() */

/*******************************************************************************
Name        : Init
Description : Initialise and open AVMEM
Parameters  : Number Of Partitions To Be Created
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL Init(U32 NumberOfPartitions)
{
    STAVMEM_InitParams_t InitAvmem;
    STAVMEM_CreatePartitionParams_t CreateParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STAVMEM_MemoryRange_t PartitionMapTable[MAX_PARTITIONS+1];
    U32 i;
    BOOL RetOk = TRUE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (NumberOfPartitions)
    {
        case 1 :
            PartitionMapTable[0].StartAddr_p = (void *) (PARTITION0_START);
            PartitionMapTable[0].StopAddr_p  = (void *) (PARTITION0_STOP);
            break;

        case 2 :
            PartitionMapTable[0].StartAddr_p = (void *) (PARTITION0_START);
            PartitionMapTable[0].StopAddr_p  = (void *) (PARTITION0_STOP);
            #if ((defined(mb314) || defined(mb382) || defined(mb376)) && defined(ST_7020)) \
                || defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
                PartitionMapTable[1].StartAddr_p = (void *) (PARTITION1_START);
                PartitionMapTable[1].StopAddr_p  = (void *) (PARTITION1_STOP);
                
            #elif (defined(mb390)|| defined(mb400)|| defined(mb457)|| defined(mb428)|| defined(mb436)|| defined(DTT5107) || defined(SAT5107)) /* Used to test VID_Setup */
                PartitionMapTable[0].StartAddr_p = (void *) (PARTITION0_START);
                PartitionMapTable[0].StopAddr_p  = (void *) (PARTITION1_START - 1);
                PartitionMapTable[1].StartAddr_p = (void *) (PARTITION1_START);
                PartitionMapTable[1].StopAddr_p  = (void *) (PARTITION0_STOP);
            #endif
            break;
            #if defined(DVD_SECURED_CHIP) && defined(ST_7109)
        case 3 :
                PartitionMapTable[0].StartAddr_p = (void *) (PARTITION0_START);
                PartitionMapTable[0].StopAddr_p  = (void *) (PARTITION0_STOP);
                PartitionMapTable[1].StartAddr_p = (void *) (PARTITION1_START);
                PartitionMapTable[1].StopAddr_p  = (void *) (PARTITION1_STOP);
                PartitionMapTable[2].StartAddr_p = (void *) (PARTITION2_START);
                PartitionMapTable[2].StopAddr_p  = (void *) (PARTITION2_STOP);
                SecuredSysMemoryRange.StartAddr_p = (void *) PARTITION0_START;
                SecuredSysMemoryRange.StopAddr_p  = (void *) PARTITION0_STOP;
            break;
            #endif  /* DVD_SECURED_CHIP && ST_7109 */
        default :

            return(FALSE);

            break;

    }



    VirtualMapping.PhysicalAddressSeenFromCPU_p    = (void *)PHYSICAL_ADDRESS_SEEN_FROM_CPU;
    VirtualMapping.PhysicalAddressSeenFromDevice_p = (void *)PHYSICAL_ADDRESS_SEEN_FROM_DEVICE;
    VirtualMapping.PhysicalAddressSeenFromDevice2_p= (void *)PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2;
    VirtualMapping.VirtualBaseAddress_p            = (void *)VIRTUAL_BASE_ADDRESS;
    VirtualMapping.VirtualSize                     = VIRTUAL_SIZE;
    VirtualMapping.VirtualWindowOffset             = 0;
    VirtualMapping.VirtualWindowSize               = VIRTUAL_WINDOW_SIZE;

    InitAvmem.MaxPartitions                 = NumberOfPartitions;
    InitAvmem.MaxBlocks                     = 100;    /* video:       1 bits buffer + 4 frame buffers
                                                        tests video: 2 dest buffer
                                                        audio:       1 bits buffer
                                                        osd :        50 */
    InitAvmem.MaxForbiddenRanges            = 3 /* =0 without OSD */;   /* forbidden ranges not used */
    InitAvmem.MaxForbiddenBorders           = 3 /* =1 without OSD */;  /* video will use one forbidden border */
    InitAvmem.CPUPartition_p                = SystemPartition_p;
    InitAvmem.NCachePartition_p             = NCachePartition_p;
    InitAvmem.MaxNumberOfMemoryMapRanges    = NumberOfPartitions; /* because each partition has a single range here */
    InitAvmem.BlockMoveDmaBaseAddr_p        = (void *) BM_BASE_ADDRESS;
    InitAvmem.CacheBaseAddr_p               = (void *) CACHE_BASE_ADDRESS;
    InitAvmem.VideoBaseAddr_p               = (void *) VIDEO_BASE_ADDRESS;
    InitAvmem.SDRAMBaseAddr_p               = (void *) MPEG_SDRAM_BASE_ADDRESS;
    InitAvmem.SDRAMSize                     = MPEG_SDRAM_SIZE;
#ifndef DCacheMapSize
    InitAvmem.NumberOfDCachedRanges         = 0;
    InitAvmem.DCachedRanges_p               = NULL;
#else
    InitAvmem.NumberOfDCachedRanges         = (DCacheMapSize / sizeof(STBOOT_DCache_Area_t)) - 1;
    InitAvmem.DCachedRanges_p               = (STAVMEM_MemoryRange_t *) DCacheMap_p;
#endif
    InitAvmem.DeviceType                    = STAVMEM_DEVICE_TYPE_VIRTUAL;
    InitAvmem.SharedMemoryVirtualMapping_p  = &VirtualMapping;
    InitAvmem.OptimisedMemAccessStrategy_p  = NULL;
#ifdef USE_GPDMA
    strcpy(InitAvmem.GpdmaName, STGPDMA_DEVICE_NAME);
#endif /*#ifdef USE_GPDMA*/

    if (STAVMEM_Init(STAVMEM_DEVICE_NAME, &InitAvmem) != ST_NO_ERROR)
    {
        STTBX_Print(("AVMEM_Init() failed !\n"));
        RetOk = FALSE;
    }
    else
    {
        for (i=0; (i < NumberOfPartitions) && RetOk; i++)
        {
            CreateParams.NumberOfPartitionRanges = (sizeof(PartitionMapTable[i]) / sizeof(STAVMEM_MemoryRange_t));
            CreateParams.PartitionRanges_p       = &PartitionMapTable[i];
            ErrorCode = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &CreateParams, &AvmemPartitionHandle[i]);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("AVMEM_CreatePartition() #%d failed !\n", i));
                RetOk = FALSE;
            }
            switch (i)
            {
                case 0 :
                    STTST_AssignInteger("AVMEM_NC_PARTITION",  AvmemPartitionHandle[i], TRUE);
                    break;

                default :
                    STTST_AssignInteger("AVMEM_C_PARTITION",  AvmemPartitionHandle[i], TRUE);
                    break;
            }
        }
        if (RetOk)
        {
            /* Init and CreatePartition successed */
            STTBX_Print(("STAVMEM initialized,\trevision=%s\n",STAVMEM_GetRevision() ));
            for (i=0; (i < NumberOfPartitions); i++)
            {
                STTBX_Print(("\t\tPartition %d Handle=0x%x Start 0x%x Stop 0x%x\n", i, AvmemPartitionHandle[i],
                        PartitionMapTable[i].StartAddr_p, PartitionMapTable[i].StopAddr_p));
            }
        }
    }
    return(RetOk);
} /* end Init() */


/*******************************************************************************
Name        : STAVMEM_Command
Description : Definition of testtool commands
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
BOOL AVMEM_Command(void)
{
    BOOL RetErr = FALSE;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */
#ifdef STAVMEM_DEBUG_MEMORY_STATUS
    RetErr |= ((STTST_RegisterCommand("AVMEM_GetMemoryStatus", TTAVMEM_GetMemoryStatus, "Get the memory status (with 'devicetype') ")));
#endif
    RetErr |= ((STTST_RegisterCommand("AVMEM_Init", TTAVMEM_Init, "Initialise AVMEM (with 'devicetype') and create partition.")));
    RetErr |= ((STTST_RegisterCommand("AVMEM_Term", TTAVMEM_Term, "Delete partition and terminate AVMEM")));
    if ( !RetErr )
    {
        STTBX_Print(( "AVMEM_Command()     \t: ok           %s\n", STAVMEM_GetRevision()  ));
    }
    else
    {
        STTBX_Print(( "AVMEM_Command()     \t: failed !\n" ));
    }
    /* We have to return TRUE if no error occurs */
    return(!RetErr);
} /* end STAVMEM_Command */

/*******************************************************************************
Name        : AVMEM_Term
Description : Terminate AVMEM
Parameters  : Number Of Partitions To Be Deleted
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void AVMEM_Term()
{
    Term(PartitionsCreated);
} /* end AVMEM_Term() */

/*******************************************************************************
Name        : Term
Description : Terminate AVMEM
Parameters  : Number Of Partitions To Be Deleted
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t Term(U32 NumberOfPartitions)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    STAVMEM_TermParams_t            TermParams;
    STAVMEM_DeletePartitionParams_t DeleteParams;
    U32 i;

    DeleteParams.ForceDelete =FALSE;
    for (i=0; i < NumberOfPartitions; i++)
    {
        ErrorCode = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &AvmemPartitionHandle[i]);
        if (ErrorCode == STAVMEM_ERROR_ALLOCATED_BLOCK)
        {
            STTBX_Print(("STAVMEM_DeletePartition(%s) #%d: Warning !! Still allocated block\n", STAVMEM_DEVICE_NAME, i));
            DeleteParams.ForceDelete = TRUE;
            STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &AvmemPartitionHandle[i]);
        }
    }
    TermParams.ForceTerminate = FALSE;
    ErrorCode = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermParams);
    if (ErrorCode == STAVMEM_ERROR_CREATED_PARTITION)
    {
        STTBX_Print(("STAVMEM_Term(%s): Warning !! Still created partition\n", STAVMEM_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("STAVMEM_Term(%s): ok\n", STAVMEM_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("STAVMEM_Term(%s): error 0x%0x\n", STAVMEM_DEVICE_NAME, ErrorCode));
    }
    return(ErrorCode);
} /* end Term() */

/* End of clavmem.c */
















