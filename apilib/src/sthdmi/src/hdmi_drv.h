/************************************************************************

File Name   : hdmi_drv.h

Description : HDMI driver internal exported types and functions.

COPYRIGHT (C) STMicroelectronics 2005

Date                     Modification                          Name
-----                    -------------                         ------
28 feb 2005              Created                               AC
***********************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HDMI_DRV_H
#define __HDMI_DRV_H

/* Includes --------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif        /* ST_OSLINUX */

#include "stddefs.h"
#include "sthdmi.h"
#include "stvmix.h"



#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */
/* Below is a 'magic number', unique for each driver.
Proposed syntax: 0xXY0ZY0ZX, where XYZ is the driver ID in hexa
  ex: id=21  decimal (0x015) ->  0x01051050
  ex: id=142 decimal (0x08e) ->  0x080e80e0
  ex: id=333 decimal (0x14d) ->  0x140d40d1
This should be an uncommon number, without 00's.
This number enables to detect validity of the handle, supposing
there will never be this number by chance in memory.
Note: Drivers could use the same syntax for handles of internal modules, replacing the 0's
by a module ID like this: 0xXYnZYnZX, where n is the module ID. */
#define STHDMI_VALID_UNIT      0x140d40d1
#define STHDMI_VENDORNAME_LENGTH  8
#define STHDMI_PRODUCTDSC_LENGTH  16
#define STHDMI_VSPAYLOAD_LENGTH   25

#define    CEC_MESSAGES_BUFFER_SIZE    16

#define STHDMI_MAX_DEVICE  2     /* Max number of Init() allowed */
#define STHDMI_MAX_OPEN    2      /* Max number of Open() allowed per Init() */
#define STHDMI_MAX_UNIT    (STHDMI_MAX_OPEN * STHDMI_MAX_DEVICE)

#ifndef STHDMI_TASK_CEC_PRIORITY
#if defined (ST_OS20)
#define STHDMI_TASK_CEC_PRIORITY   7
#endif
#if defined (ST_OS21)
#define STHDMI_TASK_CEC_PRIORITY   125
#endif
#if defined (ST_OSLINUX)
#define STHDMI_TASK_CEC_PRIORITY   12
#endif
#endif

#define CEC_NOTIFY_CLEAR_ALL            0x00
#define CEC_NOTIFY_MESSAGE              0x01
#define CEC_NOTIFY_HPD                  0x02
#define CEC_NOTIFY_EDID                 0x04
#define CEC_NOTIFY_LOGIC_ADDRESS        0x08


#ifdef STHDMI_CEC
enum
{
  HDMI_CEC_COMMAND_EVT_ID,
  HDMI_NB_REGISTERED_EVENTS_IDS
};
#endif

typedef enum InfoFrameType_e
{
    INFO_FRAME_NONE,
    INFO_FRAME_VER_ONE,
    INFO_FRAME_VER_TWO
}InfoFrameType_t;

/* Exported Types ----------------------------------------------------------- */
#ifdef STHDMI_CEC
typedef struct HDMI_Task_s {

    task_t*                         Task_p;
    void*                           TaskStack;
    tdesc_t                         TaskDesc;
    BOOL    IsRunning;        /* TRUE if task is running, FALSE otherwise */
    BOOL    ToBeDeleted;      /* Set TRUE to ask task to end in order to delete it */
} HDMI_Task_t;
typedef struct
{
    BOOL                    IsCaseFree;
    STVOUT_CECMessage_t     Message;
    STHDMI_CEC_Command_t    Command;
}HDMI_CMD_MSG_t;

typedef struct HDMI_CECStruct_s
{
    U8                              Notify;
    STVOUT_CECMessageInfo_t         CurrentReceivedMessageInfo;
    HDMI_CMD_MSG_t                  CEC_Cmd_Msg_Buffer[CEC_MESSAGES_BUFFER_SIZE];
    BOOL                            IsEDIDRetrived;
    BOOL                            IsPhysicalAddressValid;
    STHDMI_CEC_PhysicalAddress_t    PhysicalAddress;
    STVOUT_CECDevice_t              CECDevice;
}HDMI_CECStruct_t;
#endif

typedef struct
{
    STHDMI_DeviceType_t            DeviceType;
    ST_DeviceName_t                DeviceName;
    ST_Partition_t*                CPUPartition_p;
    U32                            MaxOpen;
    STVOUT_Handle_t                VoutHandle;
    ST_DeviceName_t                VoutName;
    STVTG_Handle_t                 VtgHandle;
    ST_DeviceName_t                VtgName;
    STEVT_Handle_t                 EvtHandle;
    ST_DeviceName_t                EvtName;
    STVOUT_OutputType_t            OutputType;
    STHDMI_AVIInfoFrameFormat_t    AVIType;
    STHDMI_SPDInfoFrameFormat_t    SPDType;
    STHDMI_MSInfoFrameFormat_t     MSType;
    STHDMI_AUDIOInfoFrameFormat_t  AUDIOType;
    STHDMI_VSInfoFrameFormat_t     VSType;
    STVOUT_ColorSpace_t            Colorimetry;
    STAUD_DACDataPrecision_t       SampleSize;
    U32                            LevelShift;
    U32                            BitRate;
    BOOL                           DownmixInhibit;
    STAUD_Object_t                 DecoderObject;
    char                           VendorName[STHDMI_VENDORNAME_LENGTH];
    char                           ProductDesc[STHDMI_PRODUCTDSC_LENGTH];
    STHDMI_SPDDeviceInfo_t         SrcDeviceInfo;
    STGXOBJ_AspectRatio_t          AspectRatio; /* Code of Aspect Ratio will be set by application */
    STGXOBJ_AspectRatio_t          ActiveAspectRatio;
    STHDMI_PictureScaling_t        PictureScaling;
    U32                            ChannelCount;
    STAUD_StreamContent_t          CodingType;
    U32                            SamplingFrequency;
    U32                            ChannelAlloc;
    STVID_SequenceInfo_t           SequenceInfo;
    STVID_PictureInfos_t           PictureInfo;
    STHDMI_OutputWindows_t         OutputWindows;
    U32                            MPEGBitRate;
    BOOL                           IsFieldRepeated;
    STVID_MPEGFrame_t              MepgFrameType;
    U32                            FrameLength;
    U32                            RegistrationId;
    char                           VSPayload[STHDMI_VSPAYLOAD_LENGTH];  /* 28 bytes of packet word allowed by HW without tacking
                                                   into account of header bytes. 25= 28-3 bytes of registration ID */
   STHDMI_EDIDProdDescription_t   EDIDProductDesc;
   char                           IDManufactureName[3];
   U8*                            MonitorData_p;
   /* For Memory alocation management */
   STHDMI_ShortVideoDescriptor_t * VideoDescriptor_p;
   U8							   VideoLength;
   STHDMI_ShortAudioDescriptor_t*  AudioDescriptor_p;
   U8                              AudioLength;
   U8*  						   VendorData_p;
   U8                              VendorLength;

   U32                             EDIDExtensionFlag;
   STHDMI_EDIDBlockExtension_t*    EDIDExtension_p;
#if defined(STHDMI_CEC)
   HDMI_Task_t                     CEC_Task;
   semaphore_t*                    CEC_Sem_p;
   semaphore_t*                    CECStruct_Access_Sem_p;
   BOOL                            CEC_TaskStarted;
   HDMI_CECStruct_t                CEC_Params;
   STEVT_EventID_t                 RegisteredEventsID[HDMI_NB_REGISTERED_EVENTS_IDS];
#endif
} sthdmi_Device_t;

typedef struct
{
    sthdmi_Device_t*        Device_p;
    U32                     UnitValidity; /* Only the value XXX_VALID_UNIT means the unit is valid */
} sthdmi_Unit_t;

/* Exported Variables ----------------------------------------------------- */
extern sthdmi_Unit_t* sthdmi_UnitArray;


/* Exported Macros -------------------------------------------------------- */
#define ERROR(err)      (((err) != ST_NO_ERROR)? STHDMI_ERROR_NOT_AVAILABLE : (err))
#define OK(err)          ((err) == ST_NO_ERROR)
/* Passing a (STHDMI_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((sthdmi_Unit_t *) (Handle)) >= sthdmi_UnitArray) &&                    \
                               (((sthdmi_Unit_t *) (Handle)) < (sthdmi_UnitArray + STHDMI_MAX_UNIT)) &&  \
                               (((sthdmi_Unit_t *) (Handle))->UnitValidity == STHDMI_VALID_UNIT))


/* Exported Functions ----------------------------------------------------- */
void sthdmi_Report( char *stringfunction, ST_ErrorCode_t Error);
ST_ErrorCode_t  InitInternalStruct  (sthdmi_Device_t * const Device_p);
#if defined(STHDMI_CEC)
ST_ErrorCode_t  sthdmi_StartCECTask (sthdmi_Device_t * const Device_p);
ST_ErrorCode_t sthdmi_StopCECTask(sthdmi_Device_t * const Device_p);
ST_ErrorCode_t  sthdmi_Register_Subscribe_Events(sthdmi_Device_t * const Device_p);
ST_ErrorCode_t sthdmi_UnRegister_UnSubscribe_Events(sthdmi_Device_t * const Device_p);
#endif
ST_ErrorCode_t  TermInternalStruct(sthdmi_Device_t * const Device_p);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HDMI_DRV_H */

/* End of hdmi_drv.h */


