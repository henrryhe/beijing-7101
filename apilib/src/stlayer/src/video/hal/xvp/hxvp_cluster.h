/*******************************************************************************

File name   : hxvp_cluster.h

Description : Cluster header file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Feb 2007        Created                                           JPB
                   
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HXVP_CLUSTER_H
#define __HXVP_CLUSTER_H

/* Includes ----------------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Constantes --------------------------------------------------------------- */

/****************************************** STBus PLUG registers */
#define	STBUS_PLUG_CONTROL_OFFSET           0x1000
#define	STBUS_PLUG_PAGE_SIZE_OFFSET         0x1004
#define	STBUS_PLUG_MIN_OPC_OFFSET           0x1008
#define	STBUS_PLUG_MAX_OPC_OFFSET           0x100c
#define	STBUS_PLUG_MAX_CHK_OFFSET           0x1010
#define	STBUS_PLUG_MAX_MSG_OFFSET           0x1014
#define	STBUS_PLUG_MIN_SPACE_OFFSET         0x1018

typedef void * ClusterHandle_t;

typedef struct XVP_clusterInit_s
{
    void                *StreamerBaseAddress;
    unsigned int         StreamerSize;
    void                *StbusRDPlugBaseAddress;
    unsigned int         StbusRDPlugSize;
    void                *StbusWRPlugBaseAddress;
    unsigned int         StbusWRPlugSize;
    const unsigned int  *MicrocodeRD_p;
    unsigned int         MicrocodeRDSize;
    const unsigned int  *MicrocodeWR_p;
    unsigned int         MicrocodeWRSize;
} XVP_clusterInit_t;

ST_ErrorCode_t hxvp_clusterInit(XVP_clusterInit_t   *InitParam);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __HXVP_CLUSTER */
