/*
	(c) Copyright changhong
  Name:channelbase.h
  Description:headet file about program base control for changhong QAM5516 DBVC platform
  Authors:yxl
  Date          Remark
  2004-06-30    Created

  2004-11-18    modifyed function CH6_AVControl()


Modifiction:
Date:2006-12-31 by yxl
1.将原来修改的无用部分delete掉
2.为了兼容7100 chip,进行下列修改
1）Modify function void CHB_NewChannel(STVID_Handle_t VHandle,STVID_ViewPortHandle_t VPHandle,int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo,
					STAUD_StreamContent_t AudType),add a new paramter STVID_Handle_t VHandle,and modify inner section
2）Modify function BOOL CHB_AVControl(STVID_Handle_t VHandle,STVID_ViewPortHandle_t VPHandle,CH6_VideoControl_Type VideoControl,BOOL AudioControl,CH6_AVControl_Type ControlType)
   add a new paramter STVID_Handle_t VHandle,and modify inner section
*/


/*yxl 2004-11-18 add this section*/
typedef enum
{
	CONTROL_VIDEO,
	CONTROL_AUDIO,
	CONTROL_VIDEOANDAUDIO
}CH6_AVControl_Type;

typedef enum
{
	VIDEO_DISPLAY,
	VIDEO_FREEZE,
	VIDEO_BLACK,
	VIDEO_CLOSE,
	VIDEO_OPEN,
	VIDEO_UNKNOWN /*YXL 2004-12-04 add this line*/
}CH6_VideoControl_Type;


/*end yxl 2004-11-18 add this section*/


/*yxl 2005-02-25 add this section*/
typedef enum
{
	AUDIO_MPEG1,
	AUDIO_MP3
	/*AUDIO_AC3	yxl 2005-11-29 add this line*/
}CH6_Audio_Type;
/*end yxl 2005-02-25 add this section*/


/*yxl 2007-04-19 add below section*/

#define MAX_VIEWPORT_COUNT 2
#define MAX_VIDDECODER_COUNT 2

typedef struct CH_SingleVIDCELL_s
{
	STVID_Handle_t VideoHandle;
	U8 VPortCount;
	STVID_ViewPortHandle_t  VPortHandle[MAX_VIEWPORT_COUNT];
		
}CH_SingleVIDCELL_t;

typedef struct CH_VIDCELL_s
{
	U8 VIDDecoderCount;
	CH_SingleVIDCELL_t  VIDCell[MAX_VIDDECODER_COUNT];
		
}CH_VIDCELL_t;

/*end yxl 2007-04-19 add below section*/

/*Function description:
	Set a new program to play  
Parameters description: 
	int  VideoPid:The video pid of the program to be set
	int AudeoPid: The AudeoPid pid of the program to be set
	int PCRPid:The PCRPid pid of the program to be set
	BOOL EnableVideo:Video control sign
			TRUE:stand for enable display video
			FALSE:stand for disable display video			
	BOOL EnableAudeo:Audeo control sign
			TRUE:stand for enable display audeo
			FALSE:stand for disable display audeo	
 
return value:NO
*/

#if 0 /*yxl 2007-04-19 modify below section*/
extern void CHB_NewChannel(STVID_Handle_t VHandle,STVID_ViewPortHandle_t VPHandle,int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo,
					STAUD_StreamContent_t AudType);

#else

extern void CHB_NewChannel(CH_SingleVIDCELL_t CHOVIDCell,STAUD_Handle_t AHandle,
					STPTI_Slot_t VSlot,STPTI_Slot_t ASlot,STPTI_Slot_t PSlot,
					int VideoPid,int AudeoPid,int PCRPid,
					BOOL EnableVideo,BOOL EnableAudeo,STAUD_StreamContent_t AudType);

#endif /*end yxl 2007-04-19 modify below section*/


/*Function description:
	Stop playing a program   
Parameters description: NO
 
return value:NO
*/
#if 0 /*yxl 2007-04-19 modify below section*/
extern void CH6_TerminateChannel(void);
#else
extern void CHB_TerminateChannel(CH_VIDCELL_t CHVIDCell,STAUD_Handle_t AHandle,
						  STPTI_Slot_t VSlot,STPTI_Slot_t ASlot,STPTI_Slot_t PSlot);

#endif /*end yxl 2007-04-19 modify below section*/

/*Function description:
	Set the size and position of the video outpot window  
Parameters description: 
	BOOL Small:the sign of setting mode of the video output window 
		TRUE:Set the video window to the non full screen mode,that is small window mode
		FALSE:Set the video window to the full screen mode,in this case,the other paramter has no sense
	unsigned short PosX:The start x position of the small video window to be set 
	unsigned short PosY:The start Y position of the small video window to be set 
	unsigned short Width: The width of the small video window to be set 
	unsigned short Height:The height of the small video window to be set 

return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
	
*/

#if 0 /*yxl 2005-09-21 rename CH6_SetVideoWindowSize() to CHB_SetVideoWindowSize(),
and add parameter STVID_ViewPortHandle_t VPHandle and modify inner */
extern BOOL  CH6_SetVideoWindowSize( BOOL Small,unsigned short PosX,unsigned short PosY,
						 unsigned short Width,unsigned short Height );
#else

#if 0 /*yxl 2008-07-16 modify input parameter type*/
extern BOOL  CHB_SetVideoWindowSize(STVID_ViewPortHandle_t VPHandle,BOOL Small,unsigned short PosX,unsigned short PosY,
						 unsigned short Width,unsigned short Height );
#else
extern BOOL  CHB_SetVideoWindowSize(STVID_ViewPortHandle_t VPHandle,BOOL Small,int PosX,int PosY,
						 unsigned short Width,unsigned short Height);
#endif /*end yxl 2008-07-16 modify input parameter type*/

#endif /*end yxl 2005-09-21*/



/* yxl 2004-10-25 add this section */
/*Function description:
	Control the video or audio of a program  
Parameters description: 
		CH6_AVControl_Type ControlType:control type
						CONTROL_VIDEO,stand for control video
						CONTROL_AUDIO,stand for control audio
						CONTROL_VIDEOANDAUDIO,stand for control both video and audio
CH6_VideoControl_Type VideoControl:how to control video,it has meaning when ControlType is CONTROL_VIDEO or CONTROL_VIDEOANDAUDIO
			VIDEO_DISPLAY:stand for enable video decode and display
			VIDEO_FREEZE: stand for video is stoppedin in freeze mode
			VIDEO_BLACK : stand for video is stoppedin in black mode
			VIDEO_CLOSE:stand for video is stoppedin in black mode and close video output
			VIDEO_OPEN:stand for open video output
		BOOL AudioControl:how to control audeo,it has meaning when ControlType is CONTROL_AUDIO or CONTROL_VIDEOANDAUDIO
					TRUE:stand for enable audeo decode and play
					FALSE:stand for disable audeo decode and play
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/

#if 0 /*yxl 2007-04-19 modify below section*/
extern BOOL CHB_AVControl(STVID_Handle_t VHandle,STVID_ViewPortHandle_t VPHandle,CH6_VideoControl_Type VideoControl,
				   BOOL AudioControl,CH6_AVControl_Type ControlType);


#else
extern BOOL CHB_AVControl(CH_VIDCELL_t CHVIDCell,STAUD_Handle_t AHandle,
				   CH6_VideoControl_Type VideoControl,BOOL AudioControl,
				   CH6_AVControl_Type ControlType);


#endif

/*yxl 2005-02-25 add this function*/
/*Function description:
	Mark type of current decoded audio stream,which is MPEG1 or MP3  
Parameters description: 
		CH6_Audio_Type:audio stream type
						AUDIO_MPEG1,stand for MPEG1 audio stream
						AUDIO_MP3,stand for MP3 audio stream
return value:NO 
*/
extern void CH6_MarkAudioStreamType(STAUD_StreamContent_t Type);

/*yxl 2005-02-25 add this function*/
/*Function description:
	Mark type of current audio decoded stream,which is MPEG1 or MP3  
Parameters description: NO

return value:
			return the type of current  decoded audio stream  
*/
extern STAUD_StreamContent_t CH6_GetAudioStreamType(void);


/*Function description:
	Enable SPDIF output  
Parameters description: NO

return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_EnableSPDIFOutput(void);
