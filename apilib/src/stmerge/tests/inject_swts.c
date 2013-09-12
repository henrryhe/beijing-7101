/*****************************************************************************
File Name   : inject_swts.c

Description : SWTS file

History     : Ported to 7100 & 5301

Copyright (C) 2005 STMicroelectronics

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "inject_swts.h"

#if defined(USE_DMA_FOR_SWTS)
#if defined(ST_5528)
#include "stgpdma.h"
#else  /* 5100,7710,7100 etc */
#include "stfdma.h" 
#endif
#else
#include "stsys.h"
#endif 

#ifndef ST_OS21
#include "debug.h"
#endif
#include "stsys.h"

#if defined(STMERGE_DTV_PACKET)
    /* Originally tested with the below stream . contains 800 packets of SCID 0x10 */
    /*U8 StreamFilename[] = "../../streams/DTV/apg_stdd3.tps"; */

    /* Similarly for this stream we are setting SCID to 0x10 */
    U8 StreamFilename[] = "../../streams/DTV/allslot1.tps";
#else
    /* Similarly for this stream we are setting PID to 0x3C */
    U8 StreamFilename[] = "../../streams/DVB/canal10.trp";

#endif

#if defined (ST_5528)
#define SWTS_REG_EVENT_LINE0                    ST5528_GPDMA_SWTS0
#define SWTS_REG_EVENT_LINE1                    ST5528_GPDMA_SWTS1
#define STMERGE_SWTS0_DATA_REGISTER             (ST5528_TSMERGE_BASE_ADDRESS + 0x280)
#define STMERGE_SWTS1_DATA_REGISTER             (ST5528_TSMERGE_BASE_ADDRESS + 0x290)
#define STMERGE_SWTSCONFIG0_REGISTER            (ST5528_TSMERGE_BASE_ADDRESS + 0x288)
#define STMERGE_SWTSCONFIG1_REGISTER            (ST5528_TSMERGE_BASE_ADDRESS + 0x298)
#ifdef ST_OS21
#define SWTS_DTV_PTIA_POKE_ADDRESS              0xB91099FC
#define SWTS_DTV_PTIB_POKE_ADDRESS              0xB91199FC
#else
#define SWTS_DTV_PTIA_POKE_ADDRESS              0x191099FC
#define SWTS_DTV_PTIB_POKE_ADDRESS              0x191199FC
#endif

#elif defined(ST_5100)

#define STMERGE_SWTS0_DATA_REGISTER             (ST5100_TSMERGE2_BASE_ADDRESS + 0x280)
#define STMERGE_SWTSCONFIG0_REGISTER            (ST5100_TSMERGE_BASE_ADDRESS  + 0x2E0)
#define SWTS_DTV_PTI_POKE_ADDRESS                0x20E099FC

#elif defined(ST_7710)

#define STMERGE_SWTS0_DATA_REGISTER             (ST7710_TSMERGE2_BASE_ADDRESS + 0x280)
#define STMERGE_SWTSCONFIG0_REGISTER            (ST7710_TSMERGE_BASE_ADDRESS  + 0x2E0)
#define SWTS_DTV_PTI_POKE_ADDRESS                0x380199FC 

#elif defined(ST_7100)

#define STMERGE_SWTS0_DATA_REGISTER             (ST7100_TSMERGE2_BASE_ADDRESS + 0x0000)
#define STMERGE_SWTSCONFIG0_REGISTER            (ST7100_TSMERGE_BASE_ADDRESS  + 0x0600)
#define SWTS_DTV_PTI_POKE_ADDRESS                0xB92399FC  /*TBC for 7109*/

#elif defined(ST_7109)

#define STMERGE_SWTS0_DATA_REGISTER             (ST7109_TSMERGE2_BASE_ADDRESS + 0x0000)
#define STMERGE_SWTSCONFIG0_REGISTER            (ST7109_TSMERGE_BASE_ADDRESS  + 0x0600)
#define STMERGE_SWTS1_DATA_REGISTER             (ST7109_TSMERGE2_BASE_ADDRESS + 0x0020)
#define STMERGE_SWTSCONFIG1_REGISTER            (ST7109_TSMERGE_BASE_ADDRESS  + 0x0610)
#define STMERGE_SWTS2_DATA_REGISTER             (ST7109_TSMERGE2_BASE_ADDRESS + 0x0040)
#define STMERGE_SWTSCONFIG2_REGISTER            (ST7109_TSMERGE_BASE_ADDRESS  + 0x0620)
#define SWTS_DTV_PTI_POKE_ADDRESS                0xB92399FC  /*TBC for 7109*/

#elif defined(ST_5301)

#define STMERGE_SWTS0_DATA_REGISTER             (ST5301_TSMERGE2_BASE_ADDRESS + 0x280)
#define STMERGE_SWTSCONFIG0_REGISTER            (ST5301_TSMERGE_BASE_ADDRESS  + 0x2E0)
#define SWTS_DTV_PTI_POKE_ADDRESS                0x20E099FC 

#elif defined(ST_5525)

#define STMERGE_SWTS0_DATA_REGISTER            (ST5525_TSMERGE_2_BASE_ADDRESS + 0x0000)
#define STMERGE_SWTSCONFIG0_REGISTER           (ST5525_TSMERGE_BASE_ADDRESS   + 0x0600)
#define STMERGE_SWTS1_DATA_REGISTER            (ST5525_TSMERGE_2_BASE_ADDRESS + 0x0020)
#define STMERGE_SWTSCONFIG1_REGISTER           (ST5525_TSMERGE_BASE_ADDRESS   + 0x0610)
#define STMERGE_SWTS2_DATA_REGISTER            (ST5525_TSMERGE_2_BASE_ADDRESS + 0x0040)
#define STMERGE_SWTSCONFIG2_REGISTER           (ST5525_TSMERGE_BASE_ADDRESS   + 0x0620)
#define STMERGE_SWTS3_DATA_REGISTER            (ST5525_TSMERGE_2_BASE_ADDRESS + 0x0060)
#define STMERGE_SWTSCONFIG3_REGISTER           (ST5525_TSMERGE_BASE_ADDRESS   + 0x0630)
#define SWTS_DTV_PTI_POKE_ADDRESS               0x20E099FC  /*TBC for 5525*/

#endif

/* Private Variables --------------------------------------------------*/

BOOL   TermInject  = FALSE ;

#if defined(ST_5528) && defined(USE_DMA_FOR_SWTS)
static STGPDMA_DmaTransferId_t          GPDMATransferId ;
static STGPDMA_DmaTransferStatus_t      GPDMAStatus ;
#endif
static stmergefn_SWTSWriteControlBlock_t*   SWTSWriteData_p ;


/****************************************************************************
Function Name   : InjectStream

Description     : This subroutine is for the injection of the stream
                  using GPDMA and STMERGE operation.

Input param     : STMERGE_ObjectId_t

Return Value    : void
****************************************************************************/
void InjectStream (STMERGE_ObjectId_t SWTS_Object )
{
    ST_ErrorCode_t    Error = ST_NO_ERROR;
    task_t*           Task_p ;
#ifndef ST_OS21
    U32               TaskStatus ;
#endif

    /* Create and load swts task params */
    SWTSWriteData_p = ( stmergefn_SWTSWriteControlBlock_t *) memory_allocate_clear ( system_partition, 1,
                        sizeof (stmergefn_SWTSWriteControlBlock_t) ) ;

    if (SWTSWriteData_p == NULL)
    {
        STTBX_Print (("\n ------ Memory Allocation For SWTS Task Parameters Failed %x ------ \n", Error)) ;
        stmergetst_SetSuccessState(FALSE);
    }

#if defined(USE_DMA_FOR_SWTS)
    SWTSWriteData_p->SWTSValues_p = ( stmergefn_SWTS_t * ) memory_allocate_clear ( NcachePartition, 1,
                                    ( ( sizeof ( stmergefn_SWTS_t ) ) * BYTES_TO_READ/4 ) ) ;

    if (SWTSWriteData_p->SWTSValues_p == NULL)
    {
        STTBX_Print (("\n ------ Memory Allocation For SWTS Task Parameters Failed %x ------ \n", Error)) ;
        stmergetst_SetSuccessState(FALSE);
    }
#endif
    /* Load swts task data */
    SWTSWriteData_p->Filename_p     =   StreamFilename;
    SWTSWriteData_p->WordCount      =   BYTES_TO_READ/4;
    SWTSWriteData_p->Partition_p    =   system_partition;
    SWTSWriteData_p->SWTSObject     =   SWTS_Object;

#ifndef ST_OS21
    /* Init file read task, and start play to SWTS register  */
    TaskStatus = task_init ( stmerge_PlayFileToSWTSTask, (void *)SWTSWriteData_p, SWTSWriteData_p->TaskData.TaskStack,
                             2048, &SWTSWriteData_p->TaskData.TaskID, &SWTSWriteData_p->TaskData.TaskDesc,
                             10, "SWTSPlayTask",(task_flags_t)0 ) ;
                             
    if ( TaskStatus )
    {
        STTBX_Print (("\n ------ Play Task Creation Failed ------ \n")) ;
        stmergetst_SetSuccessState(FALSE);
    }
#else
    /* Init file read task, and start play to SWTS register  */
    Task_p = task_create ( stmerge_PlayFileToSWTSTask, (void *)SWTSWriteData_p,
                           20*1024,0, "SWTSPlayTask",(task_flags_t)0 ) ;
                             
    if ( Task_p == NULL )
    {
        STTBX_Print (("\n ------ Play Task Creation Failed ------ \n")) ;
        stmergetst_SetSuccessState(FALSE);
    }
#endif

#ifndef ST_OS21
    Task_p = &SWTSWriteData_p->TaskData.TaskID ;
#endif

    task_wait ( &Task_p, 1, TIMEOUT_INFINITY ) ;
    
#ifndef ST_OS21
    task_delete ( &SWTSWriteData_p->TaskData.TaskID ) ;
#else
    task_delete ( Task_p ) ;
#endif

    memory_deallocate ( SWTSWriteData_p->Partition_p, SWTSWriteData_p ) ;
#if defined(USE_DMA_FOR_SWTS)
    memory_deallocate ( SWTSWriteData_p->Partition_p, SWTSWriteData_p->SWTSValues_p);
#endif

} /* End of Inject Stream */


/****************************************************************************
Function Name   : stmerge_PlayFileToSWTSTask

Description     : Calls an SWTS handling function

Input param     : void SWTSDataHandle_p : Ptr to address of SWTS control block

Return Value    : None

****************************************************************************/

void stmerge_PlayFileToSWTSTask ( void* SWTSDataHandle_p )
{
#if defined(USE_DMA_FOR_SWTS)
    PlayFileBySWTSDMA ( SWTSDataHandle_p ) ;
#else
    PlayFileBySWTSWrite( SWTSDataHandle_p ) ;
#endif
} /* End of stmerge_PlayFileToSWTSTask*/

#if defined(USE_DMA_FOR_SWTS)
/****************************************************************************
Function Name    : PlayFileBySWTSDMA

Description      : Handles the reading of the specifiec file to the given array,
                   and the emptying of the array to the SWTS register by DMA

Input param      : void SWTSDataHandle_p : Ptr to address of SWTS control block

Return Value     : None

****************************************************************************/

void PlayFileBySWTSDMA ( void* SWTSDataHandle_p )
{
    stmergefn_SWTSWriteControlBlock_t           *SWTSData_p ;
    ST_ErrorCode_t                              Error = ST_NO_ERROR ;

#if defined(ST_5528)
    ST_DeviceName_t                             GPDMADeviceName ;
    STGPDMA_InitParams_t                        GPDMAInitParams ;
    STGPDMA_OpenParams_t                        GPDMAOpenParams ;
    STGPDMA_Handle_t                            GPDMAHandle ;
    STGPDMA_DmaTransfer_t                       GPDMATransfer ;
    STGPDMA_TermParams_t                        GPDMATermParams ;
#else /* 5100,7710 etc */
    ST_DeviceName_t                             FDMADeviceName = "FDMA2";
    STFDMA_InitParams_t                         FDMAInitParams ;
    STFDMA_ChannelId_t                          FDMAChannelId = STFDMA_USE_ANY_CHANNEL;
    STFDMA_TransferParams_t                     FDMATransferParams ;
    STFDMA_TransferId_t                         FDMATransferId = 1;
    STFDMA_Node_t                               *Node_p;
#endif

    FILE_TYPE                                   Infile ;
    long                                        Size ;
    U32                                         count = 0;

    SWTSData_p = ( stmergefn_SWTSWriteControlBlock_t* ) SWTSDataHandle_p ;

    /* Open file */
    Infile = DEBUGOPEN ( (char *) SWTSData_p->Filename_p, "rb" ) ;
    if ( Infile FILE_EXISTING_COMPARE)
    {
        STTBX_Print (("\n ------ Reading Transport Stream File Failed ------ \n")) ;
        stmergetst_SetSuccessState(FALSE);
    }
    else
    {

#if defined(ST_5528)
        STTBX_Print (("\n ------ Configuring GPDMA ------ \n")) ;

        /* shared event handler */
        strncpy ( GPDMAInitParams.EVTDeviceName, EVENT_HANDLER_NAME, sizeof ( ST_DeviceName_t ) ) ;
        GPDMAInitParams.DeviceType          =   STGPDMA_DEVICE_GPDMAC ;
        GPDMAInitParams.DriverPartition     =   NcachePartition ;
        GPDMAInitParams.BaseAddress         =   (U32 *)GPDMA_BASE_ADDRESS ;
        GPDMAInitParams.InterruptNumber     =   GPDMA_BASE_INTERRUPT+1 ;
        GPDMAInitParams.InterruptLevel      =   5 ;
        GPDMAInitParams.ChannelNumber       =   1 ;
        GPDMAInitParams.FreeListSize        =   1 ;
        GPDMAInitParams.MaxHandles          =   1 ;
        GPDMAInitParams.MaxTransfers        =   10 ;

        Error = STGPDMA_Init( "GPDMA" ,&GPDMAInitParams ) ;
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print (("\n ------ STGPDMA_Init Failed %x ------ \n", Error)) ;
        }


        Error = STGPDMA_Open ( "GPDMA", &GPDMAOpenParams, &GPDMAHandle ) ;
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print (("\n ------ STGPDMA_Open Failed %x ------ \n", Error)) ;
        }


        Error = STGPDMA_SetEventProtocol ( "GPDMA", SWTS_REG_EVENT_LINE0, STGPDMA_PROTOCOL_DREQ ) ;
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print (("\n ------ STGPDMA_SetEventProtocol Failed %x ------ \n", Error)) ;
        }


        if (Error == ST_NO_ERROR)
        {

            /* Configure dma transfer of data to SWTS */
            GPDMATransfer.TimingModel               =   STGPDMA_TIMING_PACED_DST ;
            GPDMATransfer.Source.TransferType       =   STGPDMA_TRANSFER_1D_INCREMENTING ;
            GPDMATransfer.Source.Address            =   (U32)&(SWTSData_p->SWTSValues_p[0].SWTSWord) ;
            GPDMATransfer.Source.UnitSize           =   4 ;
            GPDMATransfer.Source.Length             =   0 ; /* only required for a 2d transfer */
            GPDMATransfer.Source.Stride             =   0 ; /* only required for a 2d transfer */
            GPDMATransfer.Destination.TransferType  =   STGPDMA_TRANSFER_0D ;


            if (SWTSData_p->SWTSObject == STMERGE_SWTS_0)
            {
                GPDMATransfer.Destination.Address       =   (device_word_t)((device_word_t*)ST5528_TSMERGE_BASE_ADDRESS + (0x0280/4));
            }
            else if (SWTSData_p->SWTSObject == STMERGE_SWTS_1)
            {
                GPDMATransfer.Destination.Address       =   (device_word_t)((device_word_t*)ST5528_TSMERGE_BASE_ADDRESS + (0x0290/4));
            }

            GPDMATransfer.Destination.UnitSize      =   4 ;
            GPDMATransfer.Destination.Length        =   0 ; /* only required for a 2d transfer */
            GPDMATransfer.Destination.Stride        =   0 ; /* only required for a 2d transfer */
            GPDMATransfer.Count                     =   BYTES_TO_READ ;
            GPDMATransfer.Next                      =   NULL ;

            STTBX_Print (("\n ------ Commencing GPDMA Transfer To SWTS \n")) ;

            while (1)
            {
                
                Size = DEBUGREAD ( Infile, &SWTSData_p->SWTSValues_p[0].SWTSWord, (SWTSData_p->WordCount*4) ) ;
                /* if eof reached, repeat on file */
                if ((Size == 0) || (count == MAX_INJECTION_TIME))
                {
                    /*STTBX_Print(("\nFile has been read once\n"));*/
                    break;
                }
                count++;

                /* While terminate  */
                Error = STGPDMA_DmaStartChannel ( GPDMAHandle, STGPDMA_MODE_BLOCK, &GPDMATransfer,
                                                  0, SWTS_REG_EVENT_LINE0, 5000,
                                                  &GPDMATransferId, &GPDMAStatus ) ;

                if (Error != ST_NO_ERROR)
                {
                    STTBX_Print (("\n ------ STGPDMA_DmaStartChannel Failed, Error  = %x ------ \n",Error)) ;
                    break;
                }

            }/* End of While */

            /* Terminate gpdma */
            STTBX_Print (("\n ------ Terminating GPDMA ------ \n")) ;
            GPDMATermParams.ForceTerminate = TRUE ;
            Error = STGPDMA_Term ( GPDMADeviceName, &GPDMATermParams ) ;
        }


#else /* 5100,7710,7100 etc */

        STTBX_Print (("\n ------ Configuring FDMA ------ \n")) ;

        FDMAInitParams.DeviceType          = STFDMA_DEVICE_FDMA_2;
        FDMAInitParams.DriverPartition_p   = SystemPartition;
        FDMAInitParams.NCachePartition_p   = NcachePartition;
        FDMAInitParams.BaseAddress_p       = (U32 *)FDMA_BASE_ADDRESS;
        FDMAInitParams.InterruptNumber     = 34;
        FDMAInitParams.InterruptLevel      = 14;
        FDMAInitParams.NumberCallbackTasks = 0;
        FDMAInitParams.ClockTicksPerSecond = ST_GetClocksPerSecond();

        /* Initialise FDMA */
        Error= STFDMA_Init(FDMADeviceName, &FDMAInitParams);
        if ( Error != ST_NO_ERROR )
        {
            STTBX_Print (("\n ------ STFDMA_Init Failed %x ------ \n", Error)) ;
        }
        else
        {
            STTBX_Print (("\n ------ STFDMA_Init Succeeded %x ------ \n", Error)) ;
        }

        /* Lock a channel */
        Error = STFDMA_LockChannel(&FDMAChannelId);
        if ( Error != ST_NO_ERROR )
        {
            STTBX_Print (("\n ------ STFDMA_LockChannel Failed %x ------ \n", Error)) ;
        }
        else
        {
            STTBX_Print (("\n ------ STFDMA_LockChannel Succeeded %x ------ \n", Error)) ;
        }

        Node_p = (STFDMA_Node_t *) memory_allocate (NcachePartition,
		 (sizeof(STFDMA_Node_t)));

        /* Generic node config */
        Node_p->Next_p                           = NULL;
        Node_p->SourceStride                     = 0;
        Node_p->DestinationStride                = 0;
        Node_p->NumberBytes                      = 1024;
        Node_p->Length                           = 1024;
        Node_p->SourceAddress_p                  = (void *)&(SWTSData_p->SWTSValues_p[0].SWTSWord);
        Node_p->DestinationAddress_p             = (void *)STMERGE_SWTS0_DATA_REGISTER;
        Node_p->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
        Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
        Node_p->NodeControl.Reserved             = 0;
        Node_p->NodeControl.PaceSignal           = (U32)STFDMA_REQUEST_SIGNAL_SWTS;
        Node_p->NodeControl.NodeCompleteNotify   = 0;
        Node_p->NodeControl.NodeCompletePause    = 0;


        FDMATransferParams.ChannelId             = FDMAChannelId;
        FDMATransferParams.CallbackFunction      = NULL; /* Blocking mode */
        FDMATransferParams.NodeAddress_p         = Node_p;
        FDMATransferParams.BlockingTimeout       = 100;
        FDMATransferParams.ApplicationData_p     = NULL;

        while (1)
        {

            /* Copy file chunk to array */
            Size = DEBUGREAD ( Infile, &SWTSData_p->SWTSValues_p[0].SWTSWord, (SWTSData_p->WordCount*4) ) ;

            /* if eof reached, repeat on file */
            if ((Size == 0) || (count == MAX_INJECTION_TIME))
            {
                /*STTBX_Print(("\nFile has been read once\n"));*/
                break;
            }

            count++;

            Error = STFDMA_StartTransfer(&FDMATransferParams,&FDMATransferId);
            if ( Error != ST_NO_ERROR )
            {
                STTBX_Print (("\n ------ STFDMA_StartTransfer Failed %x ------ \n", Error)) ;
                break;
            }
            else
            {
                STTBX_Print (("\n ------ STFDMA_LockChannel Succeeded %x ------ \n", Error)) ;
            }

        }/* End of While */

        /* Terminate fdma */
        STTBX_Print (("\n ------ Terminating FDMA ------ \n")) ;
        Error = STFDMA_Term(FDMADeviceName, FALSE, NULL);
        if ( Error != ST_NO_ERROR )
        {
            STTBX_Print (("\n ------ STFDMA_Term Failed %x ------ \n", Error)) ;
        }

#endif
    }
    /* Close file, if opened successfully */
    if (Infile >= 0)
    {
        DEBUGCLOSE(Infile);
    }

} /* end of PlayFileBySWTSDMA */
#endif
/****************************************************************************
Name         : PlayFileBySWTSWrite

Description  : Handles the reading of the specifiec file to the given array,
               and the emptying of the array to the SWTS register by writing
               to the register from the array

Parameters   : void SWTSDataHandle_p : Ptr to address of SWTS control block

Return Value : None

****************************************************************************/

void PlayFileBySWTSWrite(void *SWTSDataHandle_p)
{

    stmergefn_SWTSWriteControlBlock_t *SWTSData_p;
    FILE_TYPE   Infile ;
    long Size;
    U32 i;
    U32 Test = 0;
    U32 count = 0;

    U8 TpBuffer[194];
    SWTSData_p = (stmergefn_SWTSWriteControlBlock_t *) SWTSDataHandle_p;

    /* Open file */
    Infile = DEBUGOPEN((char *) SWTSData_p->Filename_p, "rb");
    if (Infile FILE_EXISTING_COMPARE)     
    {
        STTBX_Print(("\n***SWTSWriteTask: ERROR: Cannot open file %s\n", SWTSData_p->Filename_p));
        stmergetst_SetSuccessState(FALSE);
    }
    else
    {
        /* While !terminate  */
        while (1)
        {

            U32*  TempBuf; 

            /* Copy file chunk to array */         
            Size = DEBUGREAD(Infile, TpBuffer, BYTES_TO_READ);
           
            /* if eof reached, repeat on file */
            if ((Size == 0) || (count == MAX_INJECTION_TIME))
            {
                STTBX_Print(("\nFile has been read once\n"));
                break;
            }
            count++;

#if defined(STMERGE_DTV_PACKET)
            memcpy(TempStore, TpBuffer, sizeof(TempStore)-1);       
            memcpy(TpBuffer+2, TempStore, sizeof(TempStore)-2);  /* move TpBuffer up one byte */
            TpBuffer[0] = 0x47; /* insert a 0x47 for syncronizing */
#endif
            TempBuf=(U32*)TpBuffer; 

            /* Drip array to SWTS register until array empty 4bytes at a time */
            for (i = 0; i < NUMBER_OF_WORDS_TO_WRITE;i++ )
            {
                if (SWTSData_p->SWTSObject == STMERGE_SWTS_0)
                {
                    *((U32*)STMERGE_SWTS0_DATA_REGISTER) = TempBuf[i];
                }
#if (NUMBER_OF_SWTS > 1)
                else if( SWTSData_p->SWTSObject == STMERGE_SWTS_1)
                {
                    *((U32*)STMERGE_SWTS1_DATA_REGISTER) = TempBuf[i];
                }
#endif
#if (NUMBER_OF_SWTS > 2)
                else if( SWTSData_p->SWTSObject == STMERGE_SWTS_2)
                {
                    *((U32*)STMERGE_SWTS2_DATA_REGISTER) = TempBuf[i];
                }
#endif
#if (NUMBER_OF_SWTS > 3)
                else if( SWTSData_p->SWTSObject == STMERGE_SWTS_3)
                {
                    *((U32*)STMERGE_SWTS3_DATA_REGISTER) = TempBuf[i];
                }
#endif
                do
                {
                    /* Wait until the transfer completes */
                    if (SWTSData_p->SWTSObject == STMERGE_SWTS_0)
                    {
                        Test = *((U32*)STMERGE_SWTSCONFIG0_REGISTER);
                    }
#if (NUMBER_OF_SWTS > 1)
                    else if (SWTSData_p->SWTSObject == STMERGE_SWTS_1)
                    {
                        Test = *((U32*)STMERGE_SWTSCONFIG1_REGISTER);
                    }
#endif
#if (NUMBER_OF_SWTS > 2)                   
                    else if (SWTSData_p->SWTSObject == STMERGE_SWTS_2)
                    {
                        Test = *((U32*)STMERGE_SWTSCONFIG2_REGISTER);
                    }
#endif
#if (NUMBER_OF_SWTS > 3)                   
                    else if (SWTSData_p->SWTSObject == STMERGE_SWTS_3)
                    {
                        Test = *((U32*)STMERGE_SWTSCONFIG3_REGISTER);
                    }
#endif

                    Test = Test & 0x80000000;
                    task_delay(10);

                }while (Test ==0);

            } /* end for */

        }/* endwhile */

    }

    /* Close file, if opened successfully */
    if (Infile >= 0)
    {   
        DEBUGCLOSE(Infile);
    }

} /* PlayFileBySWTSWrite */
/* End of inject_swts.c */

