/*
  Name:NEC_keymaop.h
  Description:header file of NEC Remote Control code mapping,∂‘”¶BK6A“£øÿ∫– 
  Authors:yxl
  Date          Remark
  2006-03-24     Created
*/


#ifdef E2P_COPY_COPY
#define SEND_E2P_KEY  0xFE
#define RECV_E2P_KEY  0xFF
#endif

#define SYSTEM_CODE     0X807b /*yxl 2005-09-15 add */


/*20050220 add MENU+OK KEY*/ 
#define FONRT_MENU_OK_KEY               48
#define OUT_MENU_OK_KEY                0xFB
/*************************/
#define EPG_END_KEY                 0xFC

/*20050307 add update loader key*/
#define UPDATE_LOADER_KEY             0xFD


#define POWER_KEY		0x48b7
#define	MUTE_KEY		0x08f7
#define	EXIT_KEY		0xa857   
#define RETURN_KEY		0xa858
#define	MENU_KEY		0x6897

#define OK_KEY			0xda25
#define	UP_ARROW_KEY	0xd827
#define	DOWN_ARROW_KEY	0xf807
#define	LEFT_ARROW_KEY	0x7887	
#define	RIGHT_ARROW_KEY	0x58a7	





#define	TV_RADIO_KEY	0x926D 

	
#define AUDIO_KEY		0x9a65  /*corresponding to ∞È“Ù key*/
#define VIDEO_KEY		0xfff2   


#define PAGE_UP_KEY		0xfff3
#define PAGE_DOWN_KEY	0xfff4

#define	EPG_KEY			0x8A75


#define RED_KEY			0x22dd
#define GREEN_KEY		0x12ed
#define YELLOW_KEY		0x32cd
#define BLUE_KEY		0x2ad5

#define HELP_KEY		0xfff0 

#define VIDEO_fFORMAT_KEY RED_KEY

#define	KEY_DIGIT0     0		
#define	KEY_DIGIT1     1
#define	KEY_DIGIT2     2
#define	KEY_DIGIT3     3
#define	KEY_DIGIT4     4
#define	KEY_DIGIT5     5
#define	KEY_DIGIT6     6
#define	KEY_DIGIT7     7
#define	KEY_DIGIT8     8
#define	KEY_DIGIT9     9


#define VOLUME_UP_KEY	0xfff5
#define VOLUME_DOWN_KEY 0xfff6


#define P_UP_KEY 		PAGE_UP_KEY
#define P_DOWN_KEY 		PAGE_DOWN_KEY

#define SELECT_KEY		OK_KEY

#define TTX_KEY         0xAA55
#define SCAN_KEY		0xa25d
#define LIST_KEY 		0xBA45
#define FAV_KEY			0x827D
#define INFO_KEY		0xB24D



/*end of file*/

