/*******************************************************************************

File name   : stdvm.c

Description : Digital Versatile manager API

COPYRIGHT (C) 2005 STMicroelectronics

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dvmint.h"
#include "dvmfs.h"
#include "dvmindex.h"
#include "handle.h"
#include "event.h"
#include "dvmtask.h"
#ifdef STDVM_ENABLE_CRYPT
 #include "dvmcrypt.h"
#endif
#ifdef STNET_CONTROLLED_BY_STDVM
 #include "stnet.h"
#endif

/* Private Types ------------------------------------------------------------ */
typedef struct STDVMi_Connection_s
{
    BOOL                        InUse;
    STDVM_ConnectionType_t      ConnectionType;
    ST_DeviceName_t             PRM_ConnectionName;

#ifdef STNET_CONTROLLED_BY_STDVM
    STNET_Handle_t              NETHandle;
    char                       *URI;                    /* Uniform resource Identifier Ex. rtp://224.10.10.1:1234 */
#endif
} STDVMi_Connection_t;

/* Private Constants -------------------------------------------------------- */
#define STDVM_REVISION  "STDVM-REL_3.3.1A0"

#define STDVM_PLAY_SPEED_AUDIO_2X    (STDVM_PLAY_SPEED_ONE*2)

/* Private Variables -------------------------------------------------------- */
static  ST_DeviceName_t     STDVMi_DeviceName;

static  ST_DeviceName_t     STDVMi_PRMDeviceName = "PRM";

static  ST_DeviceName_t     STDVMi_MP3PBDeviceName = "MP3";

static STDVMi_Connection_t  STDVMi_Connections[STDVM_MAX_CONNECTIONS];

/* Private Macros ----------------------------------------------------------- */
#define STDVMi_INTERNAL_HANDLES     4

#ifdef STNET_CONTROLLED_BY_STDVM
#define STNETi_UDP_DEFAULT_PORT         1234
#define STNETi_RTP_DEFAULT_PORT         1234
#endif

/* Private functions prototypes --------------------------------------------- */

/* Global Variables --------------------------------------------------------- */
partition_t                *STDVMi_CachePartition,
                           *STDVMi_NCachePartition;

/* Public functions --------------------------------------------------------- */

#ifdef STNET_CONTROLLED_BY_STDVM
ST_ErrorCode_t STDVMi_GetProducer(void *Handle, U32 **Producer)
{
    return STNET_ReceiverGetWritePointer((STNET_Handle_t)(U32)Handle, (void **)Producer);
}

ST_ErrorCode_t STDVMi_SetConsumer(void *Handle, U32  *Consumer)
{
    return STNET_ReceiverSetReadPointer((STNET_Handle_t)(U32)Handle, Consumer);
}

ST_ErrorCode_t STDVMi_TCPIPInjectTS(void *Handle, U8 *Buffer, U32 Size)
{
    STNET_StartParams_t StartParams;

    StartParams.u.TransmitterParams.BufferAddress = Buffer;
    StartParams.u.TransmitterParams.BufferSize    = Size;
    StartParams.u.TransmitterParams.IsBlocking    = TRUE;

    return STNET_Start((STNET_Handle_t)(U32)Handle,&StartParams);
}
#endif

/*******************************************************************************
Name         : STDVM_Init()

Description  : Initializes STDVM. Creates an instance of STDVM
                and initializes the hardware

Parameters   : ST_DeviceName_t      DeviceName     STDVM name
               STDVM_InitParams_t  *InitParams_p   Pointer to the params structure

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_Init(ST_DeviceName_t DeviceName, STDVM_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t      ErrCode;
    STPRM_InitParams_t  STPRM_InitParams;
#ifdef ENABLE_MP3_PLAYBACK
    MP3PB_InitParams_t  MP3PB_InitParams;
#endif
    S32                 i;


    STDVMi_CachePartition  = InitParams_p->CachePartition;
    STDVMi_NCachePartition = InitParams_p->NCachePartition;

    ErrCode = STDVMi_AllocateHandles(STDVM_MAX_INSTANCES+STDVMi_INTERNAL_HANDLES);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_AllocateHandles : %08X\n", ErrCode));
        return ErrCode;
    }

    STPRM_InitParams.CachePartition  = InitParams_p->CachePartition;
    STPRM_InitParams.NCachePartition = InitParams_p->NCachePartition;
    strcpy(STPRM_InitParams.EVTDeviceName, InitParams_p->EVTDeviceName);
    strcpy(STPRM_InitParams.DMADeviceName, InitParams_p->DMADeviceName);

    ErrCode = STPRM_Init(STDVMi_PRMDeviceName, &STPRM_InitParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_Init : %08X\n", ErrCode));
        return ErrCode;
    }

    /* add connections */
    for(i = 0; i < InitParams_p->NbOfConnections; i++)
    {
        ErrCode = STDVM_AddConnection(STDVMi_DeviceName, &InitParams_p->Connections[i]);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_AddConnection Failed : %08X\n", ErrCode));
            return ErrCode;
        }
    }

#ifdef ENABLE_MP3_PLAYBACK
    MP3PB_InitParams.CachePartition  = InitParams_p->CachePartition;
    MP3PB_InitParams.NCachePartition = InitParams_p->NCachePartition;
    strcpy(MP3PB_InitParams.EVTDeviceName, InitParams_p->EVTDeviceName);

    ErrCode = MP3PB_Init(STDVMi_MP3PBDeviceName, &MP3PB_InitParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Init : %08X\n", ErrCode));
        return ErrCode;
    }
#endif

    ErrCode = STDVMi_EventInit(DeviceName, STDVMi_PRMDeviceName, STDVMi_MP3PBDeviceName, InitParams_p);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_EventInit : %08X\n", ErrCode));
        return ErrCode;
    }

    ErrCode = STDVMi_ServiceTaskInit();
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_ServiceTaskInit : %08X\n", ErrCode));
        return ErrCode;
    }

#ifdef STDVM_ENABLE_CRYPT
    ErrCode = STDVMi_CryptInit();
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_CryptInit : %08X\n", ErrCode));
        return ErrCode;
    }
#endif

    /* store Device name for verifying */
    strcpy(STDVMi_DeviceName, DeviceName);

    /* store Nache partition for further use */
    STDVMi_NCachePartition = InitParams_p->NCachePartition;

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_GetRevision()

Description  : Revision number string

Parameters   : None

Return Value : Revision string
*******************************************************************************/
ST_Revision_t  STDVM_GetRevision(void)
{
    return ((ST_Revision_t) STDVM_REVISION);
}


/*******************************************************************************
Name         : STDVM_AddConnection()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_AddConnection(ST_DeviceName_t DeviceName, STDVM_Connection_t *Connection_p)
{
    ST_ErrorCode_t      ErrCode;
    STPRM_Connection_t  STPRM_Connection;
    U32                 i;

    if(strcmp(STDVMi_DeviceName, DeviceName) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid Device name %s\n", DeviceName));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    for(i=0; i < STDVM_MAX_CONNECTIONS; i++)
    {
        if(STDVMi_Connections[i].InUse == FALSE)
            break;
    }
    if(i == STDVM_MAX_CONNECTIONS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "No Free Connections\n"));
        return ST_ERROR_NO_MEMORY;
    }
    STDVMi_Connections[i].InUse = TRUE;
    STDVMi_Connections[i].ConnectionType = Connection_p->Type;

    switch(Connection_p->Type)
    {
        case STDVM_CONNECTION_SRC_DEMUX:
            STPRM_Connection.Type = STPRM_CONNECTION_SRC_DEMUX;
            strcpy(STPRM_Connection.Description.Demux.PTIDeviceName, Connection_p->Description.Demux.PTIDeviceName);
            STPRM_Connection.Description.Demux.PTIHandle = Connection_p->Description.Demux.PTIHandle;
            STPRM_Connection.Description.Demux.CLKHandle = Connection_p->Description.Demux.CLKHandle;
            strcpy(STDVMi_Connections[i].PRM_ConnectionName, Connection_p->Description.Demux.PTIDeviceName);
            break;

        case STDVM_CONNECTION_DST_SWTS:
            STPRM_Connection.Type = STPRM_CONNECTION_DST_SWTS;
            strcpy(STPRM_Connection.Description.Swts.PTIDeviceName, Connection_p->Description.Swts.PTIDeviceName);
            STPRM_Connection.Description.Swts.PTIHandle         = Connection_p->Description.Swts.PTIHandle;
            STPRM_Connection.Description.Swts.SWTSBaseAddress   = Connection_p->Description.Swts.SWTSBaseAddress;
            STPRM_Connection.Description.Swts.SWTSRequestSignal = Connection_p->Description.Swts.SWTSRequestSignal;
            strcpy(STDVMi_Connections[i].PRM_ConnectionName, Connection_p->Description.Swts.PTIDeviceName);
            break;

        case STDVM_CONNECTION_DST_NULL:
            STPRM_Connection.Type = STPRM_CONNECTION_DST_NULL;
            strcpy(STDVMi_Connections[i].PRM_ConnectionName, "NULL");
            break;

        case STDVM_CONNECTION_SRC_NETWORK:
#ifdef STNET_CONTROLLED_BY_STDVM
            {
                STNET_OpenParams_t  STNET_OpenParams;
                U32                 j;
                U8                  k,
                                    URILength;
                char               *URIPtr,
                                    TempString[256];
                U16                 PortNumber;


                for(j=0; j < STDVM_MAX_CONNECTIONS; j++)
                {
                    if((STDVMi_Connections[j].InUse == TRUE) &&
                       (STDVMi_Connections[j].URI != NULL) &&
                       (strcmp(STDVMi_Connections[j].URI, Connection_p->Description.Network.URI) == 0) &&
                       (STDVMi_Connections[j].ConnectionType == Connection_p->Type))
                    {
                        memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Connection Already Exist\n"));
                        return ST_ERROR_BAD_PARAMETER;
                    }
                }

                STDVMi_Connections[i].URI = memory_allocate(STDVMi_CachePartition, strlen(Connection_p->Description.Network.URI)+1);
                if(STDVMi_Connections[i].URI == NULL)
                {
                    memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "memory_allocate failed\n"));
                    return ST_ERROR_NO_MEMORY;
                }

                strcpy(STDVMi_Connections[i].URI, Connection_p->Description.Network.URI);
                sprintf(STDVMi_Connections[i].PRM_ConnectionName, "NET%d", i);

                URIPtr = (char *)Connection_p->Description.Network.URI;
                URILength = strlen(URIPtr);

                /* Find protocol */
                for(k = 0; k < (URILength - 3); k++)
                {
                    if((URIPtr[0] == ':') && (URIPtr[1] == '/') && (URIPtr[2] == '/'))
                    {
                        URIPtr += 3;
                        break;
                    }
                    TempString[k] = *URIPtr++;
                }
                TempString[k] = '\0';
                if(strcmp(TempString, "udp") == 0)
                {
                    PortNumber = STNETi_UDP_DEFAULT_PORT;
                }
                else if (strcmp(TempString, "rtp") == 0)
                {
                    PortNumber = STNETi_RTP_DEFAULT_PORT;
                }
                URILength -= k + 3;

                /* Find IP address */
                for(k = 0; k < URILength; k++)
                {
                    if(URIPtr[0] == ':')
                    {
                        URIPtr += 1;
                        break;
                    }
                    TempString[k] = *URIPtr++;
                }
                TempString[k] = '\0';

                /* Find Port number */
                if(k < URILength)   /* Port number is present */
                {
                    PortNumber = atoi(URIPtr);
                }

                STNET_OpenParams.TransmissionProtocol = STNET_PROTOCOL_RTP_TS;/*STNET_PROTOCOL_TS*/


                STNET_OpenParams.Connection.ReceiverPort = PortNumber;
                strcpy(STNET_OpenParams.Connection.ReceiverIPAddress, (char *)TempString);

                ErrCode = STNET_Open(Connection_p->Description.Network.NETDeviceName,
                                     &STNET_OpenParams,
                                     &STDVMi_Connections[i].NETHandle);
                if(ErrCode != ST_NO_ERROR)
                {
                    memory_deallocate(STDVMi_CachePartition, STDVMi_Connections[i].URI);
                    memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STNET_Open : %08X\n", ErrCode));
                    return ErrCode;
                }

                /* Add connection to STPRM */
                STPRM_Connection.Type = STPRM_CONNECTION_SRC_BUFFER;
                STPRM_Connection.Description.Buffer.BufferBase  = 0;
                STPRM_Connection.Description.Buffer.BufferSize  = 0;
                STPRM_Connection.Description.Buffer.UserHandle  = (void *)(U32)STDVMi_Connections[i].NETHandle;
                STPRM_Connection.Description.Buffer.GetProducer = STDVMi_GetProducer;
                STPRM_Connection.Description.Buffer.SetConsumer = STDVMi_SetConsumer;
            }
            break;
#else
            memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Connection type STDVM_CONNECTION_SRC_NETWORK not enabled\n"));
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

        case STDVM_CONNECTION_DST_NETWORK:
#ifdef STNET_CONTROLLED_BY_STDVM
            {
                STNET_OpenParams_t  STNET_OpenParams;
                U32                 j;
                U8                  URILength;

                for(j=0; j < STDVM_MAX_CONNECTIONS; j++)
                {
                    if((STDVMi_Connections[j].InUse == TRUE) &&
                       (STDVMi_Connections[j].URI != NULL) &&
                       (strcmp(STDVMi_Connections[j].URI, Connection_p->Description.Network.URI) == 0) &&
                       (STDVMi_Connections[j].ConnectionType == Connection_p->Type))
                    {
                        memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Connection Already Exist\n"));
                        return ST_ERROR_BAD_PARAMETER;
                    }
                }

                STDVMi_Connections[i].URI = memory_allocate(STDVMi_CachePartition, strlen(Connection_p->Description.Network.URI)+1);
                if(STDVMi_Connections[i].URI == NULL)
                {
                    memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "memory_allocate failed\n"));
                    return ST_ERROR_NO_MEMORY;
                }

                strcpy(STDVMi_Connections[i].URI, Connection_p->Description.Network.URI);
                sprintf(STDVMi_Connections[i].PRM_ConnectionName, "NET%d", i);

                STNET_OpenParams.TransmissionProtocol = STNET_PROTOCOL_RTP_TS;/*STNET_PROTOCOL_TS*/
                URILength = strlen(STDVMi_Connections[i].URI);
                /*strncpy(STNET_OpenParams.Connection.ReceiverPort, STDVMi_Connections[i].URI + (URILength-4), 4);*/
                STNET_OpenParams.Connection.ReceiverPort = 1234;
                if(strncmp(STDVMi_Connections[i].URI, "rtp://", 6) == 0)
                {
                    strncpy(STNET_OpenParams.Connection.ReceiverIPAddress, STDVMi_Connections[i].URI+6, URILength-10);
                }
                else if (strncmp(STDVMi_Connections[i].URI, "rtsp://", 7) == 0)
                {
                    strncpy(STNET_OpenParams.Connection.ReceiverIPAddress, STDVMi_Connections[i].URI+7, URILength-11);
                }
#if 0
                ErrCode = STNET_Open(Connection_p->Description.Network.NETDeviceName,
                                     &STNET_OpenParams,
                                     &STDVMi_Connections[i].NETHandle);
                if(ErrCode != ST_NO_ERROR)
                {
                    memory_deallocate(STDVMi_CachePartition, STDVMi_Connections[i].URI);
                    memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STNET_Open : %08X\n", ErrCode));
                    return ErrCode;
                }
#endif
                /* Add connection to STPRM */
                STPRM_Connection.Type = STPRM_CONNECTION_DST_TCPIP;
                STPRM_Connection.Description.TCPIP.TCPIPHandle   = (void *)(U32)STDVMi_Connections[i].NETHandle;
                STPRM_Connection.Description.TCPIP.TCPIPInjectTS = STDVMi_TCPIPInjectTS;
            }
            break;
#else
            memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Connection type STDVM_CONNECTION_DST_NETWORK not enabled\n"));
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

        default:
            memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid connection type : %08X\n", Connection_p->Type));
            return ST_ERROR_BAD_PARAMETER;
    }

    ErrCode = STPRM_SetConnection(STDVMi_PRMDeviceName, STDVMi_Connections[i].PRM_ConnectionName, &STPRM_Connection);
    if(ErrCode != ST_NO_ERROR)
    {
#ifdef STNET_CONTROLLED_BY_STDVM
        if((Connection_p->Type == STDVM_CONNECTION_SRC_NETWORK) || (Connection_p->Type == STDVM_CONNECTION_DST_NETWORK))
        {
            STNET_Close(STDVMi_Connections[i].NETHandle);
            memory_deallocate(STDVMi_CachePartition, STDVMi_Connections[i].URI);
        }
#endif
        memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_SetConnection : %08X\n", ErrCode));
        return ErrCode;
    }

#ifdef STNET_CONTROLLED_BY_STDVM
    if(Connection_p->Type == STDVM_CONNECTION_SRC_NETWORK)
    {
        STNET_StartParams_t StartParams;

        StartParams.u.ReceiverParams.BufferAddress = (U8 *)STPRM_Connection.Description.Buffer.BufferBase;
        StartParams.u.ReceiverParams.BufferSize    = STPRM_Connection.Description.Buffer.BufferSize;
        StartParams.u.ReceiverParams.AllowOverflow = TRUE;

        ErrCode = STNET_Start(STDVMi_Connections[i].NETHandle, &StartParams);
        if(ErrCode != ST_NO_ERROR)
        {
            STPRM_SetConnection(STDVMi_PRMDeviceName, STDVMi_Connections[i].PRM_ConnectionName, NULL);
            memory_deallocate(STDVMi_CachePartition, STDVMi_Connections[i].URI);
            STNET_Close(STDVMi_Connections[i].NETHandle);
            memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STNET_ReceiverStart : %08X\n", ErrCode));
            return ErrCode;
        }
    }
#endif

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RemoveConnection()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_RemoveConnection(ST_DeviceName_t DeviceName, STDVM_Connection_t *Connection_p)
{
    ST_ErrorCode_t      ErrCode;
    U32                 i;


    if(strcmp(STDVMi_DeviceName, DeviceName) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid Device name %s\n", DeviceName));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    switch(Connection_p->Type)
    {
        case STDVM_CONNECTION_SRC_DEMUX:

            for(i=0; i < STDVM_MAX_CONNECTIONS; i++)
            {
                if((STDVMi_Connections[i].InUse == TRUE) &&
                   (strcmp(STDVMi_Connections[i].PRM_ConnectionName, Connection_p->Description.Demux.PTIDeviceName) == 0))
                    break;
            }
            break;

        case STDVM_CONNECTION_DST_SWTS:

            for(i=0; i < STDVM_MAX_CONNECTIONS; i++)
            {
                if((STDVMi_Connections[i].InUse == TRUE) &&
                   (strcmp(STDVMi_Connections[i].PRM_ConnectionName, Connection_p->Description.Swts.PTIDeviceName) == 0))
                    break;
            }
            break;

        case STDVM_CONNECTION_DST_NULL:
            for(i=0; i < STDVM_MAX_CONNECTIONS; i++)
            {
                if((STDVMi_Connections[i].InUse == TRUE) && (STDVMi_Connections[i].ConnectionType == Connection_p->Type))
                    break;
            }
            break;

        case STDVM_CONNECTION_SRC_NETWORK:
#ifdef STNET_CONTROLLED_BY_STDVM
            for(i=0; i < STDVM_MAX_CONNECTIONS; i++)
            {
                if((STDVMi_Connections[i].InUse == TRUE) &&
                   (STDVMi_Connections[i].URI != NULL) &&
                   (strcmp(STDVMi_Connections[i].URI, Connection_p->Description.Network.URI) == 0) &&
                   (STDVMi_Connections[i].ConnectionType == Connection_p->Type))
                {
                    ErrCode = STNET_Stop(STDVMi_Connections[i].NETHandle);
                    if(ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STNET_Stop : %08X\n", ErrCode));
                        return ErrCode;
                    }
                    break;
                }
            }
#else
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Connection type STDVM_CONNECTION_SRC_NETWORK not enabled\n"));
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif
            break;

#ifdef STNET_CONTROLLED_BY_STDVM
            for(i=0;i< STDVM_MAX_CONNECTIONS;i++)
            {
                if((STDVMi_Connections[i].InUse == TRUE) &&
                   (STDVMi_Connections[i].URI != NULL) &&
                   (strcmp(STDVMi_Connections[i].URI, Connection_p->Description.Network.URI) == 0) &&
                   (STDVMi_Connections[i].ConnectionType == Connection_p->Type))
                {
                    break;
                }
            }
#else
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Connection type STDVM_CONNECTION_SRC_NETWORK not enabled\n"));
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif
            break;

        default:
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid connection type : %08X\n", Connection_p->Type));
            return ST_ERROR_BAD_PARAMETER;
    }

    if(i == STDVM_MAX_CONNECTIONS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "connection does not exist : %08X\n", Connection_p->Type));
        return ST_ERROR_BAD_PARAMETER;
    }

    ErrCode = STPRM_SetConnection(STDVMi_PRMDeviceName, STDVMi_Connections[i].PRM_ConnectionName, NULL);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_SetConnection : %08X\n", ErrCode));
        return ErrCode;
    }

#ifdef STNET_CONTROLLED_BY_STDVM
    if((Connection_p->Type == STDVM_CONNECTION_SRC_NETWORK) || (Connection_p->Type == STDVM_CONNECTION_DST_NETWORK))
    {
        ErrCode = STNET_Close(STDVMi_Connections[i].NETHandle);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STNET_Close : %08X\n", ErrCode));
            return ErrCode;
        }

        memory_deallocate(STDVMi_CachePartition, STDVMi_Connections[i].URI);
    }
#endif

    memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_Term()

Description  : Terms STDVM ans underlying STPRM

Parameters   : ST_DeviceName_t      DeviceName     STDVM name
               STDVM_TermParams_t  *TermParams_p   Pointer to the term params structure

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_Term(ST_DeviceName_t DeviceName, STDVM_TermParams_t *TermParams_p)
{
    ST_ErrorCode_t      ErrCode;
    STPRM_TermParams_t  STPRM_TermParams;
#ifdef ENABLE_MP3_PLAYBACK
    MP3PB_TermParams_t  MP3PB_TermParams;
#endif
    U32                 i;


    if(strcmp(STDVMi_DeviceName, DeviceName) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid Device name %s\n", DeviceName));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* check if any handle is still open */
    if(STDVMi_IsAnyHandleOpen() == TRUE)
    {
        if(!TermParams_p->ForceTerminate)           /* Unforced Closure */
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DVM handles are still open\n"));
            return( ST_ERROR_OPEN_HANDLE );
        }
        else                                        /* Forced Closure is not yet supported */
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Force terminate not supported\n"));
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }

#ifdef STDVM_ENABLE_CRYPT
    ErrCode = STDVMi_CryptTerm();
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_CryptTerm : %08X\n", ErrCode));
        return ErrCode;
    }
#endif

    ErrCode = STDVMi_ServiceTaskTerm();
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_ServiceTaskTerm : %08X\n", ErrCode));
        return ErrCode;
    }

    ErrCode = STDVMi_EventTerm(DeviceName, STDVMi_PRMDeviceName, STDVMi_MP3PBDeviceName, TermParams_p->ForceTerminate);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_EventTerm : %08X\n", ErrCode));
        return ErrCode;
    }

#ifdef ENABLE_MP3_PLAYBACK
    MP3PB_TermParams.ForceTerminate = TermParams_p->ForceTerminate;

    ErrCode = MP3PB_Term(STDVMi_MP3PBDeviceName, &MP3PB_TermParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Term : %08X\n", ErrCode));
        return ErrCode;
    }
    memset(STDVMi_MP3PBDeviceName, 0, sizeof(STDVMi_MP3PBDeviceName));
#endif

    /* remove connections */
    for(i=0; i < STDVM_MAX_CONNECTIONS; i++)
    {
        if(STDVMi_Connections[i].InUse == TRUE)
        {
#ifdef STNET_CONTROLLED_BY_STDVM
            if(STDVMi_Connections[i].ConnectionType == STDVM_CONNECTION_SRC_NETWORK)
            {
                ErrCode = STNET_Stop(STDVMi_Connections[i].NETHandle);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STNET_Stop : %08X\n", ErrCode));
                    return ErrCode;
                }
            }
#endif
            ErrCode = STPRM_SetConnection(STDVMi_PRMDeviceName, STDVMi_Connections[i].PRM_ConnectionName, NULL);
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_SetConnection : %08X\n", ErrCode));
                return ErrCode;
            }

#ifdef STNET_CONTROLLED_BY_STDVM
            if((STDVMi_Connections[i].ConnectionType == STDVM_CONNECTION_SRC_NETWORK) ||
               (STDVMi_Connections[i].ConnectionType == STDVM_CONNECTION_DST_NETWORK))
            {
                ErrCode = STNET_Close(STDVMi_Connections[i].NETHandle);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STNET_Close : %08X\n", ErrCode));
                    return ErrCode;
                }

                memory_deallocate(STDVMi_CachePartition, STDVMi_Connections[i].URI);
            }
#endif
            memset(&STDVMi_Connections[i], 0, sizeof(STDVMi_Connection_t));
        }
    }

    STPRM_TermParams.ForceTerminate = FALSE;

    ErrCode = STPRM_Term(STDVMi_PRMDeviceName, &STPRM_TermParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_Term : %08X\n", ErrCode));
        return ErrCode;
    }

    ErrCode = STDVMi_DeallocateHandles();
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_DeallocateHandles : %08X\n", ErrCode));
        return ErrCode;
    }

    /* clear the device name when term is successful */
    memset(STDVMi_DeviceName, 0, sizeof(STDVMi_DeviceName));

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_Open()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_Open(ST_DeviceName_t DeviceName, STDVM_OpenParams_t *OpenParams_p, STDVM_Handle_t *Handle)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p;
    STPRM_OpenParams_t  STPRM_OpenParams;
#ifdef ENABLE_MP3_PLAYBACK
    MP3PB_OpenParams_t  MP3PB_OpenParams;
#endif


    if(strcmp(STDVMi_DeviceName, DeviceName) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid Device name %s\n", DeviceName));
        return ST_ERROR_BAD_PARAMETER;
    }

    Handle_p = STDVMi_GetFreeHandle();
    if(Handle_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_GetFreeHandle : No memory\n"));
        return ST_ERROR_NO_FREE_HANDLES;
    }

    switch(OpenParams_p->Type)
    {
        case STDVM_OBJECT_PLAYBACK:

            STPRM_OpenParams.Type = STPRM_OBJECT_PLAYBACK;

            STPRM_OpenParams.Params.OpenParamsPlay.VIDHandle        = OpenParams_p->Params.OpenParamsPlay.VIDHandle;
            STPRM_OpenParams.Params.OpenParamsPlay.AUDHandle        = OpenParams_p->Params.OpenParamsPlay.AUDHandle;
            STPRM_OpenParams.Params.OpenParamsPlay.TTXHandle        = OpenParams_p->Params.OpenParamsPlay.TTXHandle;
            STPRM_OpenParams.Params.OpenParamsPlay.SUBHandle        = OpenParams_p->Params.OpenParamsPlay.SUBHandle;
            STPRM_OpenParams.Params.OpenParamsPlay.CLKHandle        = OpenParams_p->Params.OpenParamsPlay.CLKHandle;
            STPRM_OpenParams.Params.OpenParamsPlay.VTGHandle        = OpenParams_p->Params.OpenParamsPlay.VTGHandle;
            STPRM_OpenParams.Params.OpenParamsPlay.AUDClkrvSource   = OpenParams_p->Params.OpenParamsPlay.AUDClkrvSource;

            memcpy(STPRM_OpenParams.Params.OpenParamsPlay.VIEWHandle,
                   OpenParams_p->Params.OpenParamsPlay.VIEWHandle,
                   sizeof(STVID_ViewPortHandle_t) * STDVM_MAX_VIEWPORTS);

            strcpy(STPRM_OpenParams.Params.OpenParamsPlay.VIDDeviceName, OpenParams_p->Params.OpenParamsPlay.VIDDeviceName);
            strcpy(STPRM_OpenParams.Params.OpenParamsPlay.AUDDeviceName, OpenParams_p->Params.OpenParamsPlay.AUDDeviceName);
            strcpy(STPRM_OpenParams.Params.OpenParamsPlay.EVTDeviceName, OpenParams_p->Params.OpenParamsPlay.EVTDeviceName);

            STPRM_OpenParams.Params.OpenParamsPlay.CALL_PlayStart   = STDVMi_PlayCallbackStart;
            STPRM_OpenParams.Params.OpenParamsPlay.CALL_PlayStop    = STDVMi_PlayCallbackStop;
            STPRM_OpenParams.Params.OpenParamsPlay.CALL_PlayJump    = STDVMi_PlayCallbackJump;
            STPRM_OpenParams.Params.OpenParamsPlay.CALL_PlayPacket  = STDVMi_PlayCallbackPacket;
            STPRM_OpenParams.Params.OpenParamsPlay.CALL_PlayGetTime = STDVMi_PlayCallbackGetTime;

            Handle_p->ObjectType  = STDVM_OBJECT_PLAYBACK;

#ifdef ENABLE_MP3_PLAYBACK
            MP3PB_OpenParams.AUDHandle     = OpenParams_p->Params.OpenParamsPlay.AUDHandle;
            strcpy(MP3PB_OpenParams.AUDDeviceName, OpenParams_p->Params.OpenParamsPlay.AUDDeviceName);

            ErrCode = MP3PB_Open(STDVMi_MP3PBDeviceName, &MP3PB_OpenParams, &Handle_p->MP3_Handle);
            if(ErrCode != ST_NO_ERROR)
            {
                STDVMi_ReleaseHandle(Handle_p);

                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Open : %08X\n", ErrCode));
                return ErrCode;
            }
#endif
            break;

        case STDVM_OBJECT_RECORD:

            STPRM_OpenParams.Type = STPRM_OBJECT_RECORD;

            strcpy(STPRM_OpenParams.Params.OpenParamsRecord.EVTDeviceName, OpenParams_p->Params.OpenParamsRecord.EVTDeviceName);

            STPRM_OpenParams.Params.OpenParamsRecord.CALL_RecordStart   = STDVMi_RecordCallbackStart;
            STPRM_OpenParams.Params.OpenParamsRecord.CALL_RecordStop    = STDVMi_RecordCallbackStop;
            STPRM_OpenParams.Params.OpenParamsRecord.CALL_RecordPacket  = STDVMi_RecordCallbackPacket;
            STPRM_OpenParams.Params.OpenParamsRecord.CALL_RecordGetTime = STDVMi_RecordCallbackGetTime;

            Handle_p->ObjectType  = STDVM_OBJECT_RECORD;
            break;

        default:
            STDVMi_ReleaseHandle(Handle_p);

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid object type : %d\n", OpenParams_p->Type));
            return ST_ERROR_BAD_PARAMETER;
    }


    STPRM_OpenParams.FSInterface.CALL_Open  = STDVMi_FileOpen;
    STPRM_OpenParams.FSInterface.CALL_Read  = STDVMi_FileRead;
    STPRM_OpenParams.FSInterface.CALL_Write = STDVMi_FileWrite;
    STPRM_OpenParams.FSInterface.CALL_Seek  = STDVMi_FileSeek;
    STPRM_OpenParams.FSInterface.CALL_Close = STDVMi_FileClose;

    ErrCode = STPRM_Open(STDVMi_PRMDeviceName, &STPRM_OpenParams, &Handle_p->PRM_Handle);
    if(ErrCode != ST_NO_ERROR)
    {
        STDVMi_ReleaseHandle(Handle_p);

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_Open : %08X\n", ErrCode));
        return ErrCode;
    }

    *Handle = (STDVM_Handle_t)Handle_p;

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayStart()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlayStart(STDVM_Handle_t PlayHandle, STDVM_PlayStartParams_t *PlayStartParams_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;
    BOOL                SrcIsFile;
    BOOL                AudioOnlyProg;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

#ifdef ENABLE_MP3_PLAYBACK
    if((PlayStartParams_p->PidsNum == 1) && (PlayStartParams_p->Pids[0].Type == STDVM_STREAMTYPE_MP3))
    {
        MP3PB_PlayStartParams_t MP3PB_PlayStartParams;

        strcpy(MP3PB_PlayStartParams.SourceName,      PlayStartParams_p->SourceName);

        ErrCode = MP3PB_PlayStart(Handle_p->MP3_Handle, &MP3PB_PlayStartParams);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : %08X\n", ErrCode));
            return ErrCode;
        }

        Handle_p->PlayStartByPRM = FALSE;
        Handle_p->EnableCrypt    = FALSE;
    }
    else
#endif
    {
        STPRM_PlayStartParams_t STPRM_PlayStartParams;
        S32                     i;

        if(PlayStartParams_p->EnableCrypt == TRUE)
        {
            ErrCode = STDVMi_CheckKey(Handle_p, PlayStartParams_p->SourceName, PlayStartParams_p->CryptKey);
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_CheckKey : %08X\n", ErrCode));
                return ErrCode;
            }

            /* Store Decrypt Params */
            Handle_p->EnableCrypt = PlayStartParams_p->EnableCrypt;
            memcpy(Handle_p->CryptKey, PlayStartParams_p->CryptKey, sizeof(STDVM_Key_t));
        }

        /* Find PRM source connection name */
        for(i = 0; i < STDVM_MAX_CONNECTIONS; i++)
        {
#ifdef STNET_CONTROLLED_BY_STDVM
            if((STDVMi_Connections[i].InUse == TRUE) &&
               ((strcmp(STDVMi_Connections[i].PRM_ConnectionName, PlayStartParams_p->SourceName) == 0) ||
                ((STDVMi_Connections[i].URI != NULL) && (strcmp(STDVMi_Connections[i].URI, PlayStartParams_p->SourceName) == 0))))
#else
            if((STDVMi_Connections[i].InUse == TRUE) &&
               (strcmp(STDVMi_Connections[i].PRM_ConnectionName, PlayStartParams_p->SourceName) == 0))
#endif
            {
                strcpy(STPRM_PlayStartParams.SourceName, STDVMi_Connections[i].PRM_ConnectionName);
                SrcIsFile = FALSE;
                break;
            }
        }
        if(i == STDVM_MAX_CONNECTIONS)
        {
            strcpy(STPRM_PlayStartParams.SourceName, PlayStartParams_p->SourceName);
            SrcIsFile = TRUE;
        }

#ifdef STNET_CONTROLLED_BY_STDVM
        /* Find PRM destination connection name */
        for(i = 0; i < STDVM_MAX_CONNECTIONS; i++)
        {
            if((STDVMi_Connections[i].InUse == TRUE) &&
               ((strcmp(STDVMi_Connections[i].PRM_ConnectionName, PlayStartParams_p->DestinationName) == 0) ||
                ((STDVMi_Connections[i].URI != NULL)&&(strcmp(STDVMi_Connections[i].URI,PlayStartParams_p->DestinationName) == 0))))
            {
                strcpy(STPRM_PlayStartParams.DestinationName, STDVMi_Connections[i].PRM_ConnectionName);
                break;
            }
        }
        /* this is a invalid destination STPRM will return error in this case */
        if(i == STDVM_MAX_CONNECTIONS)
#endif
        {
            strcpy(STPRM_PlayStartParams.DestinationName, PlayStartParams_p->DestinationName);
        }

        /* consider AudioOnlyProg only for HDD playback streams where trickmodes are possible */
        AudioOnlyProg = SrcIsFile;

        for(i = 0; i < PlayStartParams_p->PidsNum; i++)
        {
            STPRM_PlayStartParams.Pids[i].Type = (PlayStartParams_p->Pids[i].Type <= STPRM_STREAMTYPE_OTHER) ? \
                                                    PlayStartParams_p->Pids[i].Type : STPRM_STREAMTYPE_OTHER;
            STPRM_PlayStartParams.Pids[i].Pid  = PlayStartParams_p->Pids[i].Pid;

            if((PlayStartParams_p->Pids[i].Type == STPRM_STREAMTYPE_MP1V) || \
               (PlayStartParams_p->Pids[i].Type == STPRM_STREAMTYPE_MP2V) || \
               (PlayStartParams_p->Pids[i].Type == STPRM_STREAMTYPE_MP4V) || \
               (PlayStartParams_p->Pids[i].Type == STPRM_STREAMTYPE_H264) || \
               (PlayStartParams_p->Pids[i].Type == STPRM_STREAMTYPE_DIVX) || \
               (PlayStartParams_p->Pids[i].Type == STPRM_STREAMTYPE_VC1))
            {
                AudioOnlyProg = FALSE;
            }
        }

        STPRM_PlayStartParams.PidsNum = PlayStartParams_p->PidsNum;

        STPRM_PlayStartParams.VideoEnableDisplay    = PlayStartParams_p->VideoEnableDisplay;
        STPRM_PlayStartParams.VideoDecodeOnce       = PlayStartParams_p->VideoDecodeOnce;
        STPRM_PlayStartParams.AudioEnableOutput     = PlayStartParams_p->AudioEnableOutput;
        STPRM_PlayStartParams.StartPositionInMs     = PlayStartParams_p->StartPositionInMs;
        STPRM_PlayStartParams.StartSpeed            = PlayStartParams_p->StartSpeed;    /* Setting StartSpeed other than 200 for audio only channels does not affect */

        ErrCode = STPRM_PlayStart(Handle_p->PRM_Handle, &STPRM_PlayStartParams);
        if(ErrCode != ST_NO_ERROR)
        {
            /* reset variables */
            Handle_p->EnableCrypt = FALSE;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayStart : %08X\n", ErrCode));
            return ErrCode;
        }

        Handle_p->AudioOnlyProg = AudioOnlyProg;

        if(Handle_p->AudioOnlyProg == TRUE)
        {
            AUDIO_TRICK_ACCESS_MUTEX_CREATE(Handle_p);
#if defined(STDVM_AUDIO_TRICK_MODES)
            if((PlayStartParams_p->StartSpeed != STDVM_PLAY_SPEED_ONE) &&
               (PlayStartParams_p->StartSpeed != STDVM_PLAY_SPEED_AUDIO_2X))
#else
            if(PlayStartParams_p->StartSpeed != STDVM_PLAY_SPEED_ONE)
#endif
            {
                ErrCode = STPRM_PlayPause(Handle_p->PRM_Handle);
                if(ErrCode != ST_NO_ERROR)
                {
                    /* reset variables */
                    Handle_p->EnableCrypt = FALSE;
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayPause : %08X\n", ErrCode));
                    return ErrCode;
                }

                AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

                Handle_p->AudioInTrickMode    = TRUE;
                Handle_p->AudioTrickStartTime = time_now();
                Handle_p->CurrentTime         = PlayStartParams_p->StartPositionInMs;
                Handle_p->CurrentSpeed        = PlayStartParams_p->StartSpeed;
                Handle_p->AudioInPause        = FALSE;
                Handle_p->AudioTrickEndOfFile = FALSE;

                AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
            }
            else
            {
                AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

                Handle_p->AudioInTrickMode    = FALSE;
                Handle_p->AudioTrickStartTime = 0;
                Handle_p->CurrentTime         = PlayStartParams_p->StartPositionInMs;
                Handle_p->CurrentSpeed        = PlayStartParams_p->StartSpeed;
                Handle_p->AudioInPause        = FALSE;
                Handle_p->AudioTrickEndOfFile = FALSE;

                AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
            }
        }
#ifdef ENABLE_MP3_PLAYBACK
        Handle_p->PlayStartByPRM = TRUE;
#endif

        /* Backward GetTime WA */
        if(PlayStartParams_p->StartSpeed < 0)
        {
            Handle_p->BackwardTrick  = TRUE;
            Handle_p->BackwardPrevTimeInMs = 0;
        }
        else
        {
            Handle_p->BackwardTrick  = FALSE;
            Handle_p->BackwardPrevTimeInMs = 0;
        }
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayStop()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlayStop(STDVM_Handle_t PlayHandle, STDVM_PlayStopParams_t *PlayStopParams_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    /* handling Audio only program variable here */
    if(Handle_p->AudioOnlyProg == TRUE)
    {
        Handle_p->AudioOnlyProg = FALSE;
        AUDIO_TRICK_ACCESS_MUTEX_DELETE(Handle_p);
    }

#ifdef ENABLE_MP3_PLAYBACK
    if(Handle_p->PlayStartByPRM == FALSE)
    {
        MP3PB_PlayStopParams_t  MP3PB_PlayStopParams;

        MP3PB_PlayStopParams.AudioStopMethod        = PlayStopParams_p->AudioStopMethod;
        MP3PB_PlayStopParams.AudioFadingMethod      = PlayStopParams_p->AudioFadingMethod;

        ErrCode = MP3PB_PlayStop(Handle_p->MP3_Handle, &MP3PB_PlayStopParams);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayStop : %08X\n", ErrCode));
            return ErrCode;
        }
    }
    else
#endif
    {
        STPRM_PlayStopParams_t  STPRM_PlayStopParams;

        STPRM_PlayStopParams.VideoDisableDisplay    = PlayStopParams_p->VideoDisableDisplay;
        STPRM_PlayStopParams.VideoStopMethod        = PlayStopParams_p->VideoStopMethod;
        STPRM_PlayStopParams.VideoFreezeMethod      = PlayStopParams_p->VideoFreezeMethod;
        STPRM_PlayStopParams.AudioStopMethod        = PlayStopParams_p->AudioStopMethod;
        STPRM_PlayStopParams.AudioFadingMethod      = PlayStopParams_p->AudioFadingMethod;

        ErrCode = STPRM_PlayStop(Handle_p->PRM_Handle, &STPRM_PlayStopParams);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayStop : %08X\n", ErrCode));
            return ErrCode;
        }
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlaySeek()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlaySeek(STDVM_Handle_t PlayHandle, S32 TimeInMs, STDVM_SeekFlags_t Flags)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;
    U32                 StartTimeInMs, CurrentTimeInMs, EndTimeInMs;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if((Handle_p->AudioOnlyProg == TRUE) && (Handle_p->AudioInTrickMode == TRUE))
    {
        ErrCode = STPRM_PlayGetTime(Handle_p->PRM_Handle, &StartTimeInMs, &CurrentTimeInMs, &EndTimeInMs);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayGetTime : %08X\n", ErrCode));
            return ErrCode;
        }

        if((TimeInMs < StartTimeInMs) || (TimeInMs > EndTimeInMs))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_PlaySeek : Time[%u] ExceedsLimits[%u, %u]\n",
                    TimeInMs, StartTimeInMs, EndTimeInMs ));
            return ST_ERROR_BAD_PARAMETER;
        }

        AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

        Handle_p->CurrentTime         = TimeInMs;
        Handle_p->AudioTrickStartTime = time_now();
        Handle_p->AudioTrickEndOfFile = FALSE;

        AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
    }

    ErrCode = STPRM_PlaySeek(Handle_p->PRM_Handle, TimeInMs, (STPRM_SeekFlags_t)Flags);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlaySeek : %08X\n", ErrCode));
        return ErrCode;
    }

    /* Backward GetTime WA */
    if(Handle_p->BackwardTrick == TRUE)
    {
        Handle_p->BackwardPrevTimeInMs = 0;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlaySetSpeed()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlaySetSpeed(STDVM_Handle_t PlayHandle, S32 Speed)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;
    U32                 StartTimeInMs, CurrentTimeInMs, EndTimeInMs;
    S32                 AudioTrickTime;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Handle_p->AudioOnlyProg == TRUE)
    {
        AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

#if defined(STDVM_AUDIO_TRICK_MODES)
        if((Speed != STDVM_PLAY_SPEED_ONE) && (Speed !=STDVM_PLAY_SPEED_AUDIO_2X))
#else
        if(Speed != STDVM_PLAY_SPEED_ONE)
#endif
        {
            if(Handle_p->AudioInTrickMode == FALSE)
            {
                if(Handle_p->AudioInPause == FALSE)
                {
                    ErrCode = STPRM_PlayPause(Handle_p->PRM_Handle);
                    if(ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayPause : %08X\n", ErrCode));
                        AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
                        return ErrCode;
                    }
                }

                ErrCode = STPRM_PlayGetTime(Handle_p->PRM_Handle, &StartTimeInMs, &CurrentTimeInMs, &EndTimeInMs);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayGetTime : %08X\n", ErrCode));

                    /* Resume play if Pause n the previous code block */
                    if(Handle_p->AudioInPause == FALSE)
                    {
                        ST_ErrorCode_t      ErrCode1;

                        ErrCode1 = STPRM_PlayResume(Handle_p->PRM_Handle);
                        if(ErrCode1 != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayResume : %08X\n", ErrCode));
                            AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ErrCode1;
                        }
                    }

                    AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ErrCode;
                }

                Handle_p->AudioInTrickMode      = TRUE;
                Handle_p->CurrentTime           = CurrentTimeInMs;
                Handle_p->AudioTrickStartTime   = time_now();
                Handle_p->CurrentSpeed          = Speed;
            }
            else
            {
                if(Handle_p->AudioTrickEndOfFile == TRUE)
                {
                    if(((Handle_p->CurrentSpeed > 0) && (Speed > 0)) ||
                       ((Handle_p->CurrentSpeed < 0) && (Speed < 0)))
                    {
                        Handle_p->CurrentSpeed = Speed;
                        return ST_NO_ERROR;
                    }
                    Handle_p->AudioTrickEndOfFile = FALSE;
                }

                if(Handle_p->AudioTrickStartTime != 0)
                {
                    AudioTrickTime         = (S32)(((float)time_minus(time_now(), Handle_p->AudioTrickStartTime) * \
                                                Handle_p->CurrentSpeed * 10) / ST_GetClocksPerSecond());
                    Handle_p->CurrentTime +=  AudioTrickTime;
                }
                Handle_p->AudioTrickStartTime   =  time_now();
                Handle_p->CurrentSpeed          =  Speed;
            }
        }
        else if(Handle_p->AudioInTrickMode == TRUE)
        {
            if(Handle_p->AudioTrickStartTime != 0)
            {
                AudioTrickTime         = (S32)(((float)time_minus(time_now(), Handle_p->AudioTrickStartTime) * \
                                            Handle_p->CurrentSpeed * 10) / (S32)ST_GetClocksPerSecond());
                Handle_p->CurrentTime +=  AudioTrickTime;
            }
            Handle_p->CurrentSpeed = Speed;

            ErrCode = STPRM_PlaySeek(Handle_p->PRM_Handle, Handle_p->CurrentTime, STPRM_PLAY_SEEK_SET);
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlaySeek : %08X\n", ErrCode));
                AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
                return ErrCode;
            }

            if(Handle_p->AudioInPause == FALSE)
            {
                ErrCode = STPRM_PlayResume(Handle_p->PRM_Handle);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayResume : %08X\n", ErrCode));
                    AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ErrCode;
                }
            }

            Handle_p->AudioInTrickMode    = FALSE;
            Handle_p->AudioTrickEndOfFile = FALSE;
        }

        AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
    }

    if((Handle_p->AudioOnlyProg == FALSE) || (Handle_p->AudioInTrickMode == FALSE))
    {
        ErrCode = STPRM_PlaySetSpeed(Handle_p->PRM_Handle, Speed);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlaySetSpeed : %08X\n", ErrCode));
            return ErrCode;
        }
    }

    /* Backward GetTime WA */
    if(Speed < 0)
    {
        if(Handle_p->BackwardTrick == FALSE)
        {
            Handle_p->BackwardTrick = TRUE;
            Handle_p->BackwardPrevTimeInMs = 0;
        }
    }
    else if(Handle_p->BackwardTrick == TRUE)
    {
        Handle_p->BackwardTrick = FALSE;
        Handle_p->BackwardPrevTimeInMs = 0;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayGetSpeed()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlayGetSpeed(STDVM_Handle_t  PlayHandle, S32 *Speed_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if((Handle_p->AudioOnlyProg == TRUE) && (Handle_p->AudioInTrickMode == TRUE))
    {
        *Speed_p = Handle_p->CurrentSpeed;
    }
    else
    {
        ErrCode = STPRM_PlayGetSpeed(Handle_p->PRM_Handle, Speed_p);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_PlayGetSpeed : %08X\n", ErrCode));
            return ErrCode;
        }
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayPause()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlayPause(STDVM_Handle_t PlayHandle)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;
    S32                 AudioTrickTime;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Handle_p->AudioOnlyProg == TRUE)
    {
        AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

        if(Handle_p->AudioInPause == TRUE)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_PlayPause: Already in PAUSE mode\n"));
            AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        if(Handle_p->AudioTrickEndOfFile == TRUE)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_PlayPause: Already in EOF\n"));
            AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        Handle_p->AudioInPause = TRUE;

        if(Handle_p->AudioInTrickMode == TRUE)
        {
            if(Handle_p->AudioTrickStartTime != 0)
            {
                AudioTrickTime         = (S32)(((float)time_minus(time_now(), Handle_p->AudioTrickStartTime) * \
                                            Handle_p->CurrentSpeed * 10) / ST_GetClocksPerSecond());
                Handle_p->CurrentTime +=  AudioTrickTime;
            }
            Handle_p->AudioTrickStartTime = 0;

            AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_NO_ERROR;
        }

        AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
    }

    ErrCode = STPRM_PlayPause(Handle_p->PRM_Handle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayPause : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayStep()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlayStep(STDVM_Handle_t PlayHandle)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if((Handle_p->AudioOnlyProg == TRUE) && (Handle_p->AudioInTrickMode == TRUE))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_PlayStep: !!!NOT POSSIBLE!!! for Audio Only Program\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    ErrCode = STPRM_PlayStep(Handle_p->PRM_Handle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayStep : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayResume()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlayResume(STDVM_Handle_t PlayHandle)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Handle_p->AudioOnlyProg == TRUE)
    {
        AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

        if(Handle_p->AudioInPause == FALSE)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_PlayResume: STDVM Already in PLAY mode\n"));
            AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        if(Handle_p->AudioTrickEndOfFile == TRUE)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_PlayResume: STDVM in END OF FILE\n"));
            AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        Handle_p->AudioInPause = FALSE;

        if(Handle_p->AudioInTrickMode == TRUE)
        {
            Handle_p->AudioTrickStartTime = time_now();

            AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_NO_ERROR;
        }

        AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
    }

    ErrCode = STPRM_PlayResume(Handle_p->PRM_Handle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayResume : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayGetStatus()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_PlayGetStatus(STDVM_Handle_t PlayHandle, STDVM_PlayStatus_t *PlayStatus_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;
    STPRM_PlayStatus_t  STPRM_PlayStatus;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_PlayGetStatus(Handle_p->PRM_Handle, &STPRM_PlayStatus);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayGetStatus : %08X\n", ErrCode));
        return ErrCode;
    }

    strcpy(PlayStatus_p->SourceName, STPRM_PlayStatus.SourceName);
    strcpy(PlayStatus_p->DestinationName, STPRM_PlayStatus.DestinationName);

    PlayStatus_p->PidsNum = STPRM_PlayStatus.PidsNum;

    memcpy(PlayStatus_p->Pids, STPRM_PlayStatus.Pids, sizeof(STPRM_StreamData_t) * PlayStatus_p->PidsNum);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayGetTime()

Description  : Get playback times

Parameters   :

Return Value :

Notes        :

*******************************************************************************/
ST_ErrorCode_t STDVM_PlayGetTime(STDVM_Handle_t PlayHandle, U32 *StartTimeInMs_p, U32 *CurrentTimeInMs_p, U32 *EndTimeInMs_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;
    S32                 AudioTrickTime;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_PlayGetTime(Handle_p->PRM_Handle, StartTimeInMs_p, CurrentTimeInMs_p, EndTimeInMs_p);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayGetTime : %08X\n", ErrCode));
        return ErrCode;
    }

    if((Handle_p->AudioOnlyProg == TRUE) && (Handle_p->AudioInTrickMode == TRUE))
    {
        AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

        if(Handle_p->AudioTrickStartTime != 0)
        {
            AudioTrickTime = (S32)(((float)time_minus(time_now(), Handle_p->AudioTrickStartTime) * \
                                Handle_p->CurrentSpeed * 10) / ST_GetClocksPerSecond());
        }
        else
        {
            AudioTrickTime = 0;
        }

        *CurrentTimeInMs_p = Handle_p->CurrentTime + AudioTrickTime;

        AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);
    }

    /* Backward GetTime WA */
    if(Handle_p->BackwardTrick == TRUE)
    {
        if((Handle_p->BackwardPrevTimeInMs != 0) && (*CurrentTimeInMs_p > Handle_p->BackwardPrevTimeInMs))
        {
            *CurrentTimeInMs_p = Handle_p->BackwardPrevTimeInMs;
        }
        else
        {
            Handle_p->BackwardPrevTimeInMs = *CurrentTimeInMs_p;
        }
    }

    return ST_NO_ERROR;
}

/*******************************************************************************
Name         : STDVM_PlayGetParams()

Description  : Get playback times

Parameters   :

Return Value :

Notes        :

*******************************************************************************/
ST_ErrorCode_t STDVM_PlayGetParams(STDVM_Handle_t PlayHandle, STDVM_PlayParams_t *PlayParams_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;
    STPRM_PlayParams_t  STPRM_PlayParams;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    memset(PlayParams_p, 0, sizeof(STDVM_PlayParams_t));

    ErrCode = STPRM_PlayGetParams(Handle_p->PRM_Handle, &STPRM_PlayParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayGetParams : %08X\n", ErrCode));
        return ErrCode;
    }

    PlayParams_p->VideoPid = STPRM_PlayParams.VideoPid;
    PlayParams_p->VideoSlot = STPRM_PlayParams.VideoSlot;
    PlayParams_p->AudioPid = STPRM_PlayParams.AudioPid;
    PlayParams_p->AudioSlot = STPRM_PlayParams.AudioSlot;
    PlayParams_p->PcrPid = STPRM_PlayParams.PcrPid;
    PlayParams_p->PcrSlot = STPRM_PlayParams.PcrSlot;

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayChangePids()

Description  :

Parameters   :

Return Value :

Notes        : PIDs always present in the stream

*******************************************************************************/
ST_ErrorCode_t STDVM_PlayChangePids(STDVM_Handle_t PlayHandle, U32 NumberOfPids, STDVM_StreamData_t *Pids_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p      = (STDVMi_Handle_t *)PlayHandle;
    STPRM_StreamData_t *STPRM_Pids_p  = (STPRM_StreamData_t *)Pids_p;
    BOOL                AudioOnlyProg = TRUE;
    S32                 i,
                        Speed;
    U32                 StartTimeInMs, CurrentTimeInMs, EndTimeInMs;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_PlayChangePids(Handle_p->PRM_Handle, NumberOfPids, STPRM_Pids_p);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayChangePids : %08X\n", ErrCode));
        return ErrCode;
    }

    for(i = 0; i < NumberOfPids; i++)
    {
        if((Pids_p[i].Type == STPRM_STREAMTYPE_MP1V) || \
           (Pids_p[i].Type == STPRM_STREAMTYPE_MP2V) || \
           (Pids_p[i].Type == STPRM_STREAMTYPE_MP4V) || \
           (Pids_p[i].Type == STPRM_STREAMTYPE_H264) || \
           (Pids_p[i].Type == STPRM_STREAMTYPE_DIVX) || \
           (Pids_p[i].Type == STPRM_STREAMTYPE_VC1))
        {
            AudioOnlyProg = FALSE;
        }
    }

    if(Handle_p->AudioOnlyProg != AudioOnlyProg)
    {
        if(AudioOnlyProg == TRUE)
        {
            ErrCode = STPRM_PlayGetSpeed(Handle_p->PRM_Handle, &Speed);
            if(ErrCode != ST_NO_ERROR)
            {
                /* TODO Handle live channels, GetSpeed returns below error for Live channels */
                if(ErrCode == STPRM_ERROR_PLAYBACK_SPEED_FAILED)
                {
                    return ST_NO_ERROR;
                }

                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM_PlayGetSpeed : %08X\n", ErrCode));
                return ErrCode;
            }

            AUDIO_TRICK_ACCESS_MUTEX_CREATE(Handle_p);


#if defined(STDVM_AUDIO_TRICK_MODES)
            if((Speed != STDVM_PLAY_SPEED_ONE) && (Speed !=STDVM_PLAY_SPEED_AUDIO_2X))
#else
            if(Speed != STDVM_PLAY_SPEED_ONE)
#endif
             {
                ErrCode = STPRM_PlayGetTime(Handle_p->PRM_Handle, &StartTimeInMs, &CurrentTimeInMs, &EndTimeInMs);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayGetTime : %08X\n", ErrCode));
                    AUDIO_TRICK_ACCESS_MUTEX_DELETE(Handle_p);
                    return ErrCode;
                }

                AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

                Handle_p->AudioInTrickMode    = TRUE;
                Handle_p->AudioTrickStartTime = time_now();
                Handle_p->CurrentTime         = CurrentTimeInMs;
                Handle_p->CurrentSpeed        = Speed;
                Handle_p->AudioInPause        = FALSE;
                Handle_p->AudioTrickEndOfFile = FALSE;

                AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);

                /* instead of using a flag we consider error condition is because of PRM is already in pause state */
                ErrCode = STPRM_PlayPause(Handle_p->PRM_Handle);
                if(ErrCode != ST_NO_ERROR)
                {
                    Handle_p->AudioInPause = TRUE;
                }
            }
            else
            {
                AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

                Handle_p->AudioInTrickMode    = FALSE;
                Handle_p->AudioTrickStartTime = 0;
                Handle_p->AudioTrickEndOfFile = FALSE;

                AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);

                /* instead of using a flag we consider error condition is because of PRM is already in pause state */
                ErrCode = STPRM_PlayPause(Handle_p->PRM_Handle);
                if(ErrCode != ST_NO_ERROR)
                {
                    Handle_p->AudioInPause  = TRUE;
                }
                else
                {
                    Handle_p->AudioInPause  = FALSE;

                    ErrCode = STPRM_PlayResume(Handle_p->PRM_Handle);
                    if(ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayResume : %08X\n", ErrCode));
                        AUDIO_TRICK_ACCESS_MUTEX_DELETE(Handle_p);
                        return ErrCode;
                    }
                }
            }
        }
        else
        {
            AUDIO_TRICK_ACCESS_MUTEX_DELETE(Handle_p);

            if((Handle_p->AudioInTrickMode == TRUE) && (Handle_p->AudioInPause != TRUE))
            {
                ErrCode = STPRM_PlayResume(Handle_p->PRM_Handle);
                if(ErrCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayResume : %08X\n", ErrCode));
                    return ErrCode;
                }
            }
        }

        Handle_p->AudioOnlyProg = AudioOnlyProg;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RecordStart()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_RecordStart(STDVM_Handle_t RecordHandle, STDVM_RecordStartParams_t *RecordStartParams_p)
{
    ST_ErrorCode_t              ErrCode;
    STDVMi_Handle_t            *Handle_p = (STDVMi_Handle_t *)RecordHandle;
    STPRM_RecordStartParams_t   STPRM_RecordStartParams;
    STDVM_StreamInfo_t          StreamInfo;
    S32                         i;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    strcpy(StreamInfo.ChannelName, RecordStartParams_p->ChannelName);
    strcpy(StreamInfo.Description, RecordStartParams_p->Description);
    StreamInfo.RecordDateTime = RecordStartParams_p->RecordDateTime;
    memcpy(StreamInfo.Pids, RecordStartParams_p->Pids, sizeof(STDVM_StreamData_t) * RecordStartParams_p->PidsNum);
    StreamInfo.PidsNum = RecordStartParams_p->PidsNum;

    ErrCode = STDVMi_SetStreamInfo(Handle_p, RecordStartParams_p->DestinationName, &StreamInfo);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_SetStreamInfo : %08X\n", ErrCode));
        return ErrCode;
    }

    /* Store Encrypt Params */
    Handle_p->EnableCrypt = RecordStartParams_p->EnableCrypt;
    memcpy(Handle_p->CryptKey, RecordStartParams_p->CryptKey, sizeof(STDVM_Key_t));

#ifdef STNET_CONTROLLED_BY_STDVM
    /* Find PRM source connection name */
    for(i = 0; i < STDVM_MAX_CONNECTIONS; i++)
    {
        if((STDVMi_Connections[i].InUse == TRUE) &&
           ((strcmp(STDVMi_Connections[i].PRM_ConnectionName, RecordStartParams_p->SourceName) == 0) ||
            ((STDVMi_Connections[i].URI != NULL) && (strcmp(STDVMi_Connections[i].URI, RecordStartParams_p->SourceName) == 0))))
        {
            strcpy(STPRM_RecordStartParams.SourceName, STDVMi_Connections[i].PRM_ConnectionName);
            break;
        }
    }
    if(i == STDVM_MAX_CONNECTIONS)
#endif
    {
        strcpy(STPRM_RecordStartParams.SourceName,      RecordStartParams_p->SourceName);
    }

#ifdef STNET_CONTROLLED_BY_STDVM
    /* Find PRM destination connection name */
    for(i = 0; i < STDVM_MAX_CONNECTIONS; i++)
    {
        if((STDVMi_Connections[i].InUse == TRUE) &&
           ((strcmp(STDVMi_Connections[i].PRM_ConnectionName, RecordStartParams_p->DestinationName) == 0) ||
            ((STDVMi_Connections[i].URI != NULL)&&(strcmp(STDVMi_Connections[i].URI,RecordStartParams_p->DestinationName) == 0))))
        {
            strcpy(STPRM_RecordStartParams.DestinationName, STDVMi_Connections[i].PRM_ConnectionName);
            break;
        }
    }
    /* this is a invalid destination STPRM will return error in this case */
    if(i == STDVM_MAX_CONNECTIONS)
#endif
    {
        strcpy(STPRM_RecordStartParams.DestinationName, RecordStartParams_p->DestinationName);
    }

    for(i = 0; i < RecordStartParams_p->PidsNum; i++)
    {
        STPRM_RecordStartParams.Pids[i].Type = (RecordStartParams_p->Pids[i].Type <= STPRM_STREAMTYPE_OTHER) ? \
                                                    RecordStartParams_p->Pids[i].Type : STPRM_STREAMTYPE_OTHER;
        STPRM_RecordStartParams.Pids[i].Pid  = RecordStartParams_p->Pids[i].Pid;
    }
    STPRM_RecordStartParams.PidsNum = RecordStartParams_p->PidsNum;

    STPRM_RecordStartParams.FileSizeInPackets       = RecordStartParams_p->FileSizeInPackets;
    STPRM_RecordStartParams.CircularFile            = RecordStartParams_p->CircularFile;
    STPRM_RecordStartParams.FlushThresholdInPackets = 0; /* TODO: to be modified for net mode */

    ErrCode = STPRM_RecordStart(Handle_p->PRM_Handle, &STPRM_RecordStartParams);
    if(ErrCode != ST_NO_ERROR)
    {
        /* reset params */
        Handle_p->EnableCrypt = FALSE;
        STDVMi_RemoveStreamInfo(Handle_p, RecordStartParams_p->DestinationName);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordStart : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RecordStop()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_RecordStop(STDVM_Handle_t RecordHandle, STDVM_RecordStopParams_t  *RecordStopParams_p)
{
    ST_ErrorCode_t              ErrCode;
    STDVMi_Handle_t            *Handle_p = (STDVMi_Handle_t *)RecordHandle;
    STPRM_RecordStopParams_t    STPRM_RecordStopParams = { 0 };


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_RecordStop(Handle_p->PRM_Handle, &STPRM_RecordStopParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordStop : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RecordStop()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_RecordGetStatus(STDVM_Handle_t RecordHandle, STDVM_RecordStatus_t *RecordStatus_p)
{
    ST_ErrorCode_t          ErrCode;
    STDVMi_Handle_t        *Handle_p = (STDVMi_Handle_t *)RecordHandle;
    STPRM_RecordStatus_t    STPRM_RecordStatus;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_RecordGetStatus(Handle_p->PRM_Handle, &STPRM_RecordStatus);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordGetStatus : %08X\n", ErrCode));
        return ErrCode;
    }

    strcpy(RecordStatus_p->SourceName, STPRM_RecordStatus.SourceName);
    strcpy(RecordStatus_p->DestinationName, STPRM_RecordStatus.DestinationName);

    RecordStatus_p->PidsNum = STPRM_RecordStatus.PidsNum;

    memcpy(RecordStatus_p->Pids, STPRM_RecordStatus.Pids, sizeof(STPRM_StreamData_t) * RecordStatus_p->PidsNum);

    RecordStatus_p->FileSizeInPackets = STPRM_RecordStatus.FileSizeInPackets;
    RecordStatus_p->CircularFile      = STPRM_RecordStatus.CircularFile;

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RecordGetTime()

Description  : Get record times

Parameters   :

Return Value :

Notes        :

*******************************************************************************/
ST_ErrorCode_t STDVM_RecordGetTime(STDVM_Handle_t RecordHandle, U32 *StartTimeInMs_p, U32 *EndTimeInMs_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)RecordHandle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_RecordGetTime(Handle_p->PRM_Handle, StartTimeInMs_p, EndTimeInMs_p);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordGetTime : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RecordGetParams()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_RecordGetParams(STDVM_Handle_t RecordHandle, STDVM_RecordParams_t *RecordParams_p)
{
    ST_ErrorCode_t          ErrCode;
    STDVMi_Handle_t        *Handle_p = (STDVMi_Handle_t *)RecordHandle;
    STPRM_RecordParams_t    STPRM_RecordParams;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    memset(RecordParams_p, 0, sizeof(STDVM_RecordParams_t));

    ErrCode = STPRM_RecordGetParams(Handle_p->PRM_Handle, &STPRM_RecordParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordGetParams : %08X\n", ErrCode));
        return ErrCode;
    }

    RecordParams_p->PidsNum = STPRM_RecordParams.PidsNum;
    memcpy(RecordParams_p->Pids, STPRM_RecordParams.Pids, sizeof(STPTI_Pid_t) * RecordParams_p->PidsNum);
    memcpy(RecordParams_p->Slots, STPRM_RecordParams.Slots, sizeof(STPTI_Slot_t) * RecordParams_p->PidsNum);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RecordPause()

Description  : Pause recording

Parameters   :

Return Value :

Notes        :

*******************************************************************************/
ST_ErrorCode_t STDVM_RecordPause(STDVM_Handle_t RecordHandle)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)RecordHandle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_RecordPause(Handle_p->PRM_Handle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordPause : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RecordResume()

Description  : Resume recording

Parameters   :

Return Value :

Notes        :

*******************************************************************************/
ST_ErrorCode_t STDVM_RecordResume(STDVM_Handle_t RecordHandle)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)RecordHandle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_RecordResume(Handle_p->PRM_Handle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordResume : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_RecordChangePids()

Description  :

Parameters   :

Return Value :

Notes        : PIDs always present in the stream

*******************************************************************************/
ST_ErrorCode_t STDVM_RecordChangePids(STDVM_Handle_t RecordHandle, U32 NumberOfPids, STDVM_StreamData_t *Pids_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)RecordHandle;
    STPRM_StreamData_t  Pids[STPRM_MAX_PIDS];
    U32                 i;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    for(i = 0; i < NumberOfPids; i++)
    {
        Pids[i].Type = (Pids_p[i].Type <= STPRM_STREAMTYPE_OTHER) ? Pids_p[i].Type : STPRM_STREAMTYPE_OTHER;
        Pids[i].Pid  = Pids_p[i].Pid;
    }

    ErrCode = STPRM_RecordChangePids(Handle_p->PRM_Handle, NumberOfPids, Pids);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordChangePids : %08X\n", ErrCode));
        return ErrCode;
    }

    ErrCode = STDVMi_UpdateStreamChangePIDs(Handle_p, NumberOfPids, Pids_p);

    return ErrCode;
}


/*******************************************************************************
Name         : STDVM_RecordChangePidsAndSource()

Description  :

Parameters   :

Return Value :

Notes        : PIDs always present in the stream

*******************************************************************************/
ST_ErrorCode_t STDVM_RecordChangePidsAndSource(STDVM_Handle_t RecordHandle, char *NewSourceName, U32 NumberOfPids, STDVM_StreamData_t *Pids_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)RecordHandle;
    STPRM_StreamData_t  Pids[STPRM_MAX_PIDS];
    U32                 i;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    for(i = 0; i < NumberOfPids; i++)
    {
        Pids[i].Type = (Pids_p[i].Type <= STPRM_STREAMTYPE_OTHER) ? Pids_p[i].Type : STPRM_STREAMTYPE_OTHER;
        Pids[i].Pid  = Pids_p[i].Pid;
    }

    ErrCode = STPRM_RecordChangePidsAndSource(Handle_p->PRM_Handle, NewSourceName, NumberOfPids, Pids);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordChangePidsAndSource : %08X\n", ErrCode));
        return ErrCode;
    }

    ErrCode = STDVMi_UpdateStreamChangePIDs(Handle_p, NumberOfPids, Pids_p);

    return ErrCode;
}


/*******************************************************************************
Name         : STDVM_RecordInsertPacket()

Description  :

Parameters   :

Return Value :

Notes        :

*******************************************************************************/
ST_ErrorCode_t STDVM_RecordInsertPacket(STDVM_Handle_t  RecordHandle, U8 *Buffer, U8 NbOfPackets)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)RecordHandle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STPRM_RecordInsertPacket(Handle_p->PRM_Handle, Buffer, NbOfPackets);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordInsertPacket : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_PlayPauseRecord()

Description  :

Parameters   :

Return Value :

Notes        :

*******************************************************************************/
ST_ErrorCode_t STDVM_PlayPauseRecord(STDVM_Handle_t  PlayHandle, STDVM_Handle_t RecordHandle, STDVM_RecordStartParams_t *RecordStartParams_p)
{
    ST_ErrorCode_t              ErrCode;
    STDVMi_Handle_t            *PlayHandle_p = (STDVMi_Handle_t *)PlayHandle;
    STDVMi_Handle_t            *RecordHandle_p = (STDVMi_Handle_t *)RecordHandle;
    STPRM_RecordStartParams_t   STPRM_RecordStartParams;
    STDVM_StreamInfo_t          StreamInfo;
    S32                         i;
    BOOL                        TempPlayEnableCrypt,
                                TempRecEnableCrypt;
    STDVM_Key_t                 TempPlayCryptKey,
                                TempRecCryptKey;

    if(STDVMi_IsHandleValid(PlayHandle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Play Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(STDVMi_IsHandleValid(RecordHandle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Record Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    strcpy(StreamInfo.ChannelName, RecordStartParams_p->ChannelName);
    strcpy(StreamInfo.Description, RecordStartParams_p->Description);
    memcpy(StreamInfo.Pids, RecordStartParams_p->Pids, sizeof(STDVM_StreamData_t) * RecordStartParams_p->PidsNum);
    StreamInfo.PidsNum = RecordStartParams_p->PidsNum;

    ErrCode = STDVMi_SetStreamInfo(RecordHandle_p, RecordStartParams_p->DestinationName, &StreamInfo);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_SetStreamInfo : %08X\n", ErrCode));
        return ErrCode;
    }

#ifdef STNET_CONTROLLED_BY_STDVM
    /* Find PRM source connection name */
    for(i = 0; i < STDVM_MAX_CONNECTIONS; i++)
    {
        if((STDVMi_Connections[i].InUse == TRUE) &&
           ((strcmp(STDVMi_Connections[i].PRM_ConnectionName, RecordStartParams_p->SourceName) == 0) ||
            ((STDVMi_Connections[i].URI != NULL) && (strcmp(STDVMi_Connections[i].URI, RecordStartParams_p->SourceName) == 0))))
        {
            strcpy(STPRM_RecordStartParams.SourceName, STDVMi_Connections[i].PRM_ConnectionName);
            break;
        }
    }
    if(i == STDVM_MAX_CONNECTIONS)
#endif
    {
        strcpy(STPRM_RecordStartParams.SourceName,      RecordStartParams_p->SourceName);
    }

#ifdef STNET_CONTROLLED_BY_STDVM
    /* Find PRM destination connection name */
    for(i = 0; i < STDVM_MAX_CONNECTIONS; i++)
    {
        if((STDVMi_Connections[i].InUse == TRUE) &&
           ((strcmp(STDVMi_Connections[i].PRM_ConnectionName, RecordStartParams_p->DestinationName) == 0) ||
            ((STDVMi_Connections[i].URI != NULL)&&(strcmp(STDVMi_Connections[i].URI,RecordStartParams_p->DestinationName) == 0))))
        {
            strcpy(STPRM_RecordStartParams.DestinationName, STDVMi_Connections[i].PRM_ConnectionName);
            break;
        }
    }
    /* this is a invalid destination STPRM will return error in this case */
    if(i == STDVM_MAX_CONNECTIONS)
#endif
    {
        strcpy(STPRM_RecordStartParams.DestinationName, RecordStartParams_p->DestinationName);
    }

    for(i = 0; i < RecordStartParams_p->PidsNum; i++)
    {
        STPRM_RecordStartParams.Pids[i].Type = (RecordStartParams_p->Pids[i].Type <= STPRM_STREAMTYPE_OTHER) ? \
                                                    RecordStartParams_p->Pids[i].Type : STPRM_STREAMTYPE_OTHER;
        STPRM_RecordStartParams.Pids[i].Pid  = RecordStartParams_p->Pids[i].Pid;
    }
    STPRM_RecordStartParams.PidsNum = RecordStartParams_p->PidsNum;

    STPRM_RecordStartParams.FileSizeInPackets       = RecordStartParams_p->FileSizeInPackets;
    STPRM_RecordStartParams.CircularFile            = RecordStartParams_p->CircularFile;
    STPRM_RecordStartParams.FlushThresholdInPackets = 0; /* TODO: to be modified for net mode */

    /* store the present variables to restore on failure */
    TempPlayEnableCrypt = PlayHandle_p->EnableCrypt;
    memcpy(TempPlayCryptKey, PlayHandle_p->CryptKey, sizeof(STDVM_Key_t));

    TempRecEnableCrypt = RecordHandle_p->EnableCrypt;
    memcpy(TempRecCryptKey, RecordHandle_p->CryptKey, sizeof(STDVM_Key_t));

    /* set Decrypt Params */
    PlayHandle_p->EnableCrypt = RecordStartParams_p->EnableCrypt;
    memcpy(PlayHandle_p->CryptKey, RecordStartParams_p->CryptKey, sizeof(STDVM_Key_t));

    /* set Encrypt Params */
    RecordHandle_p->EnableCrypt = RecordStartParams_p->EnableCrypt;
    memcpy(RecordHandle_p->CryptKey, RecordStartParams_p->CryptKey, sizeof(STDVM_Key_t));

    ErrCode = STPRM_PlayPauseRecord(PlayHandle_p->PRM_Handle, RecordHandle_p->PRM_Handle, &STPRM_RecordStartParams);
    if(ErrCode != ST_NO_ERROR)
    {
        /* restore the variables */
        PlayHandle_p->EnableCrypt = TempPlayEnableCrypt;
        memcpy(PlayHandle_p->CryptKey, TempPlayCryptKey, sizeof(STDVM_Key_t));

        RecordHandle_p->EnableCrypt = TempRecEnableCrypt;
        memcpy(RecordHandle_p->CryptKey, TempRecCryptKey, sizeof(STDVM_Key_t));

        STDVMi_RemoveStreamInfo(RecordHandle_p, RecordStartParams_p->DestinationName);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayPauseRecord : %08X\n", ErrCode));
        return ErrCode;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVM_GetStreamInfo()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_GetStreamInfo(STDVM_Handle_t Handle, char *StreamName, STDVM_StreamInfo_t *StreamInfo_p)
{
    ST_ErrorCode_t      ErrCode;


    if(STDVMi_IsHandleValid((STDVMi_Handle_t *)Handle) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STDVMi_GetStreamInfo((STDVMi_Handle_t *)Handle, StreamName, StreamInfo_p);

    return ErrCode;
}


/*******************************************************************************
Name         : STDVM_RemoveStream()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_RemoveStream(STDVM_Handle_t Handle, char *StreamName)
{
    ST_ErrorCode_t      ErrCode;


    if(STDVMi_IsHandleValid((STDVMi_Handle_t *)Handle) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STDVMi_RemoveStream((STDVMi_Handle_t *)Handle, StreamName);

    return ErrCode;
}


/*******************************************************************************
Name         : STDVM_CopyStream()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_CopyStream(STDVM_Handle_t Handle, char *OldStreamName, char *NewStreamName, U32 StartTimeInMs, U32 EndTimeInMs)
{
    ST_ErrorCode_t      ErrCode;


    if(STDVMi_IsHandleValid((STDVMi_Handle_t *)Handle) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STDVMi_CopyStream((STDVMi_Handle_t *)Handle, OldStreamName, StartTimeInMs, EndTimeInMs, NewStreamName);

    return ErrCode;
}


/*******************************************************************************
Name         : STDVM_CropStream()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_CropStream(STDVM_Handle_t Handle, char *StreamName, U32 NewStartTimeInMs, U32 NewEndTimeInMs)
{
    ST_ErrorCode_t      ErrCode;


    if(STDVMi_IsHandleValid((STDVMi_Handle_t *)Handle) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STDVMi_CropStream((STDVMi_Handle_t *)Handle, StreamName, NewStartTimeInMs, NewEndTimeInMs);

    return ErrCode;
}


/*******************************************************************************
Name         : STDVM_SwitchCircularToLinear()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_SwitchCircularToLinear(STDVM_Handle_t  RecordHandle)
{
    ST_ErrorCode_t      ErrCode;


    if(STDVMi_IsHandleValid((STDVMi_Handle_t *)RecordHandle) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    ErrCode = STDVMi_SwitchCircularToLinear((STDVMi_Handle_t *)RecordHandle);

    return ErrCode;
}


/*******************************************************************************
Name         : STDVM_Close()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_Close(STDVM_Handle_t Handle)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)Handle;


    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

#ifdef ENABLE_MP3_PLAYBACK
    if(Handle_p->MP3_Handle != 0)
    {
        ErrCode = MP3PB_Close(Handle_p->MP3_Handle);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Close : %08X\n", ErrCode));
            return ErrCode;
        }
    }
#endif

    ErrCode = STPRM_Close(Handle_p->PRM_Handle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_Close : %08X\n", ErrCode));
        return ErrCode;
    }

    ErrCode = STDVMi_ReleaseHandle(Handle_p);

    return ErrCode;
}


/*******************************************************************************
Name         : STDVM_GetKey()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVM_GetKey(ST_DeviceName_t DeviceName, const void *Buffer, STDVM_Key_t *Key)
{
    STDVMi_Handle_t    *Handle_p;


    if(strcmp(STDVMi_DeviceName, DeviceName) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid Device name %s\n", DeviceName));
        return ST_ERROR_BAD_PARAMETER;
    }

    Handle_p = STDVMi_GetHandleFromBuffer((void *)Buffer);
    if((Handle_p == NULL) || (Handle_p->EnableCrypt != TRUE))
    {
        /* Not a TS Buffer OR Data not to be EnCrypted/DeCrypted */
        return ST_ERROR_BAD_PARAMETER;
    }

    /* copy the Key */
    memcpy(Key, Handle_p->CryptKey, sizeof(STDVM_Key_t));

    return ST_NO_ERROR;
}

#ifdef STDVM_DEBUG

/*******************************************************************************
Name         : STDVM_GetPRMHandle()
Description  : STDVM_GetPRMHandle

Parameters   :

Return Value :

Notes        :

*******************************************************************************/
ST_ErrorCode_t STDVM_GetPRMHandle(STDVM_Handle_t PlayHandle,STPRM_Handle_t *PRMHandle)
{
    STDVMi_Handle_t    *Handle_p = (STDVMi_Handle_t *)PlayHandle;
    if(STDVMi_IsHandleValid(Handle_p) == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVM Invalid Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    *PRMHandle = Handle_p->PRM_Handle;

    return ST_NO_ERROR;
}
#endif

/* EOF ---------------------------------------------------------------------- */

