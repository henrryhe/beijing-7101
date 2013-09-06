/*
  Name:NEC_keymaop.h
  Description:header file of NEC Remote Control code mapping,对应BK3c遥控盒 
  Authors:yxl
  Date          Remark
  2004-04-1     Created
*/


#ifdef E2P_COPY_COPY
#define SEND_E2P_KEY  0xFE
#define RECV_E2P_KEY  0xFF
#endif

#define SYSTEM_CODE     0X806F /*yxl 2005-09-15 add */

#define EPG_END_KEY                 0xFC
#define NVOD_END_KEY                0xFD


#define	REMOTE_KEY_REPEATED			0XFF00 /*	0x80 */

#define REMOTE_SYSTEM_CODE				0X04  

/*20050220 add MENU+OK KEY*/ 
#define FONRT_MENU_OK_KEY               48
#define OUT_MENU_OK_KEY                0xFB
/*************************/

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




#define HELP_KEY		0x02fd  /*corresponding to 信息 key*/
#define	TV_RADIO_KEY	0x0000  /*corresponding to 广播 key*/

	
#define AUDIO_KEY		0x9a65  /*corresponding to 伴音 key*/
#define VIDEO_KEY		0xaa55  /*corresponding to 图象 key*/

#define PAGE_UP_KEY		0x52ad
#define PAGE_DOWN_KEY	0xd22d


#define STOCK_KEY       0x22dd
#define	EPG_KEY			0x827d
#define NVOD_KEY		0x728d
#define HTML_KEY        0xb24d
#define GAME_KEY        0x32cd


#define KEY_A           0x0af5
#define KEY_B           0x8a75
#define KEY_C           0x4ab5
#define KEY_D           0xca35

#define RED_KEY			0x629d
#define GREEN_KEY		0x42bd
#define YELLOW_KEY		0x6a95
#define BLUE_KEY		0x5aa5


#define	HOMEPAGE_KEY	84		
#define MOSAIC_PAGE_KEY 130


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


#define VOLUME_UP_KEY	0xfffe
#define VOLUME_DOWN_KEY 0xfffd


#define P_UP_KEY 		PAGE_UP_KEY
#define P_DOWN_KEY 		PAGE_DOWN_KEY

#define SELECT_KEY		OK_KEY

/*20050916 add for TELETEXT KEY*/
#define TTX_KEY    0x926d
#define SCAN_KEY   0xa25d
#define LIST_KEY 		0x28d7
#define FAV_KEY			0xd02f
#define INFO_KEY		0xaa55  



/*end of file*/

