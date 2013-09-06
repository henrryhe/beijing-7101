/*
(c) Copyright changhong
Name:ch_hdmi.h
Description:head file of hdmi contol for changhong 7100 platform
Authors:yixiaoli
Date          Remark
2007-01-30    Created

Modify record: 


*/

typedef struct HDMI_Context_s
{
	Mutex_t                 MutexLock;
	STHDMI_Handle_t         HDMI_Handle;
	STEVT_Handle_t          EVT_Handle;
	STVOUT_Handle_t         VOUT_Handle;
	STVTG_Handle_t          VTG_Handle;  
	ST_DeviceName_t         AUD_DeviceName;
	ST_DeviceName_t         VTG_DeviceName;
	STVOUT_State_t          VOUT_State;
	BOOL                    HDMI_AutomaticEnableDisable;
	BOOL                    HDMI_IsEnabled;
	STVOUT_OutputType_t     HDMI_OutputType;
	STHDMI_ScanInfo_t       HDMI_ScanInfo;       
	STVOUT_ColorSpace_t     HDMI_Colorimetry;       
	STGXOBJ_AspectRatio_t   HDMI_AspectRatio;       
	STGXOBJ_AspectRatio_t   HDMI_ActiveAspectRatio;       
	STHDMI_PictureScaling_t HDMI_PictureScaling;    
	U32                     HDMI_AudioFrequency; 
	U32                     HDMI_IV0_Key;
	U32                     HDMI_IV1_Key;
	U32                     HDMI_KV0_Key;
	U32                     HDMI_KV1_Key;
	U32                     HDMI_DeviceKeys[80]; 
} HDMI_Context_t;


extern HDMI_Context_t  HDMI_Context[1];

extern BOOL bHDMI_restart;





extern BOOL CHHDMI_Setup(void);

extern BOOL CHHDMI_Term(void);

extern BOOL CH_HDMIAudeoSetup(void);




/*end of file*/
