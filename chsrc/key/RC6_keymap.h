/*
  Name:RC6_keymaop.h
  Description:header file of RC6 Remote Control code mapping,对应北京歌华遥控盒 
  Authors:yxl
  Date          Remark
  2004-04-1     Created
*/

#ifndef _RC6_RC_H
#define _RC6_RC_H

#ifdef E2P_COPY_COPY
#define SEND_E2P_KEY  0xFE
#define RECV_E2P_KEY  0xFF
#endif

#define EPG_END_KEY                 0xFC
#define NVOD_END_KEY                0xFD

#define RADIO_UPDATE_KEY      0xFA/*050708 ADD FOR RADIO BANNER 20070127 change
                                                             0xFE-->0xFA*/

#define	REMOTE_KEY_REPEATED			0XFF00 /*	0x80 */

#define REMOTE_SYSTEM_CODE				0X04  

/*20050220 add MENU+OK KEY*/ 
#define FONRT_MENU_OK_KEY               48
#define OUT_MENU_OK_KEY                0xFB
/*************************/

/*20050307 add update loader key*/
#define UPDATE_LOADER_KEY             0xFD
#if 0/*20051220 del*/
#define	MENU_CUM_EXIT_KEY				176	/*menu key */
#define	VOL_DOWN_CUM_LEFT_ARROW_KEY		57	/* volume- & left arrow key */
#define	VOL_UP_CUM_RIGHT_ARROW_KEY		169	/* volume+ & right arrow key */

#define	PROG_PREV_CUM_UP_ARROW_KEY		177	/* prog- & up arrow key */
#define	PROG_NEXT_CUM_DOWN_ARROW_KEY	153	/* prog+ & down arrow key */

#define OK_CUM_SELECT_KEY               56 /*ok key*/

#define	POWER_CUM_SELECT_KEY			0xe0 /*standby */
#define ALT_CUM_KEY                     0xd0 /*TV_RADIO */
#endif

#define KEY_A           140
#define KEY_B           141
#define KEY_C           142
#define KEY_D           143
#define KEY_E            144
#define  KEY_F           145

#define	EXIT_KEY						10   


#define	UP_ARROW_KEY					88
#define	DOWN_ARROW_KEY				89

#define	MUTE_KEY						13
#define	LEFT_ARROW_KEY				90	
#define	RIGHT_ARROW_KEY			    	91	

#define RED_KEY							109
#define GREEN_KEY						110
#define YELLOW_KEY						111
#define BLUE_KEY							112

#define OK_KEY							92	
#define SELECT_KEY					       92	
#define HELP_KEY							15	

#define POWER_KEY						12



#define FAV_KEY							0x22	
#define SEARCH_KEY						0x36	
#define AUDIO_KEY						78
#define VIDEO_KEY						0x04    

#define PAGE_UP_KEY				33
#define PAGE_DOWN_KEY			32

#define VOLUME_UP_KEY				16
#define VOLUME_DOWN_KEY 				17

#define P_UP_KEY 				33
#define P_DOWN_KEY 				32

#define LIST_KEY 			       245

#define RETURN_KEY			   131

#define STOCK_KEY                       147
#define	EPG_KEY							204		
#define NVOD_KEY						146
#define HTML_KEY                        256


#define	MENU_KEY						84
#define	HOMEPAGE_KEY					84		
#define   MOSAIC_PAGE_KEY               130

#define	VOLUME_PREV_KEY					16		
#define	VOLUME_NEXT_KEY					17	

#define ZOOM_KEY						RED_KEY		

#define	UP_ARROW_FRONTKEY				0xf01
#define	DOWN_ARROW_FRONTKEY				0xf02
#define	LEFT_ARROW_FRONTKEY				0xf03	
#define	RIGHT_ARROW_FRONTKEY			0xf04
#define	MENU_FRONTKEY						0xf05
#define	OK_FRONTKEY						0xf06

#ifdef SUMA_SECURITY
#define VIRKEY_REMOVECARD_KEY             0X888
#define VIRKEY_CLOSESKB_KEY             0X777
#define VIRKEY_CHANGETS_KEY			0x666
#define VIRKEY_CLOSESUMA_KEY		0x555
#endif

/*
 * since the ASCII code value for 0 to 9 is clashing with the
 * valid remote keys, we selected '0' as 0x50 instead of 0x30
 */
#define	TV_RADIO_KEY					78	/* RSW2 */
#define	KEY_DIGIT0     0		/* converted K0_KEY to K9_KEY */
#define	KEY_DIGIT1     1
#define	KEY_DIGIT2     2
#define	KEY_DIGIT3     3
#define	KEY_DIGIT4     4
#define	KEY_DIGIT5     5
#define	KEY_DIGIT6     6
#define	KEY_DIGIT7     7
#define	KEY_DIGIT8     8
#define	KEY_DIGIT9     9

#define	K1_KEY							0x35	/* RSW5 */
#define	K2_KEY							0x2D	/* RSW6 */
#define	K3_KEY							0x25	/* RSW7 */
#define	K4_KEY							0x1D	/* RSW8 */
#define	K5_KEY							0x34	/* RSW9 */
#define	K6_KEY							0x2C	/* RSW10 */
#define	K7_KEY							0x24	/* RSW11 */
#define	K8_KEY							0x1C	/* RSW12 */
#define	K9_KEY							0x33	/* RSW13 */
#define	K0_KEY							0x2B	/* RSW14 */


/* 定义Usif 事件虚拟键值 */
#define VKEY_NO_KEY                         0x0000
#define VKEY_NO_CARD				0xff1f	/*拔卡锁定遥控及前面板按键输入 */
#define VKEY_NIT_CHANGE			0xff20	/* nit版本变化 */
#define VKEY_SOFTWARE_CHANGE		0xff21	/* cdt版本变化 */
#define VKEY_NVOD_BEGIN			0xff22	/* nvod开始事件 */
#define VKEY_NVOD_END				0xff23	/* nvod结束事件 */
#define VKEY_EPG_BEGIN				0xff24	/* epg开始事件 */
#define VKEY_EPG_END				0xff25	/* epg结束事件 */
#define VKEY_SOFTWARE_CHANGE_FORCE		0xff26	/* cdt版本变化 ,强制升级*/
#define VKEY_SET					0xff27	/* 设置虚拟键 */
#define VKEY_BQID_CHANGE                  0xff28    /*智能卡bouquetid变化,强制重新搜索节目,llb add 060909*/  
#define VKEY_NetId_CHANGE                 0xff29    /*网络ID,机顶盒不在服务区内*/
#define VKEY_BAT_CHANGE                    0xff2A    /*用户BAT数据变化*/
#define VKEY_EMPTY_DATABASE		0xff2b	/* 数据库为空 */
#define VKEY_NVOD			              0xff2C	/* 从菜单进入NVOD */
#define VKEY_NVOD_MAINMENU             0xff2D	/*直接进入主菜单*/
#define VKEY_IPPV_START                      0xff2e    /*进入IPPV订购*/
#define VKEY_MOSAIC				 	 0xff30 /*进入MOSAIC*/
#define VKEY_EPG_F1_CURRENT             0xff31       /*当前信息*/
#define VKEY_EPG_F2_WEEK		        0xff32         /*周报信息*/
#define VKEY_EPG_F3_SORT		        0xff33        /*分类信息*/
#define VKEY_EPG_F4_TIMER                  0xff34         /*定时器*/
#define VKEY_EPG_MAINMENU                0xff35         /*跳入主界面*/
#define VKEY_START_OCSVCNAME          0xff36		/*启动oc服务，使用服务明为参数*/
#define VKEY_START_TVGAME		        0xff37		/*启动从网络中搜索下来的tv game*/
#define VKEY_TUNERANDBANNER            0xff38 		/*调频*/
#define VKEY_TFCA_SHOWERRORMES     0xff39		/*显示CA的没有授权信息信息*/

#define VKEY_USB_MSG1				0xff3a		/* 有移动设备接入 */
#define VKEY_USB_MSG2				0xff3b		/* 设备已可用*/
#define VKEY_USB_MSG3				0xff3c		/* 设备读取失败 */
#define VKEY_USB_MSG4				0xff3d 		/* 设备移除异常*/
#define VKEY_USB_MSG5				0xff3e		/* 设备移除 */

#define VKEY_BOOT_NAVIGATE		0xff3f		/* 开机导航LJiang081014 */

/* 定义Usif控制字 */
#define USIF_CONTROL_NONE			0xff40	/* 没有控制字 */
#define USIF_CONTROL_RESEARCH		0xff41	/* 重新搜索 */
#define USIF_CONTROL_UPDATE		0xff42	/* 软件升级 */
#define USIF_CONTROL_NVOD_BEGIN	0xff43	/* nvod开始 */
#define USIF_CONTROL_NVOD_END		0xff44	/* nvod结束 */
#define USIF_CONTROL_EPG_BEGIN	0xff45	/* epg开始 */
#define USIF_CONTROL_EPG_END		0xff46	/* epg结束 */
#define USIF_CONTROL_PLAY_TV		0xff47	/* 节目播放 */
#define USIF_CONTROL_PLAY_AUDIO	0xff48	/* 广播播放 */
#define USIF_CONTROL_STANDBY		0xff49	/* 待机 */
#define USIF_CONTROL_JMP_SC		0xff4a	/* 跳到收藏 */
#define USIF_CONTROL_JMP_EPG		0xff4b	/* 跳到节目 */
#define USIF_CONTROL_JMP_GB		0xff4c	/* 跳到广播 */
#define USIF_CONTROL_JMP_DTV		0xff4d	/* 跳到数字电视 */
#define USIF_CONTROL_JMP_ZX		0xff4e	/* 跳到咨询 */
#define USIF_CONTROL_JMP_DB		0xff4f	/* 跳到点播 */
#define USIF_CONTROL_JMP_SZ		0xff50	/* 跳到设置 */
#define USIF_CONTROL_JMP_YJ		0xff51	/* 跳到邮件 */
#define USIF_CONTROL_PLAY_NVOD	0xff52	/*从NVOD菜单观看正在播放的节目*/
#define USIF_CONTROL_MENU			0xff53	/*从其他菜单返回主页*/
#define USIF_CONTROL_JMP_LIST_TV	0xff54	/* 跳到电视节目列表 */
#define USIF_CONTROL_JMP_LIST_AUDIO	0xff55	/* 跳到广播节目列表 */
#define USIF_CONTROL_EMPTY_DATABASE	0xff56	/* 数据库为空 */
#define USIF_CONTROL_REFRESH		0xff57	/* 根据最新的BAT数据刷新节目*/
#define USIF_CONTROL_JMP_HTML         0xff58   /*跳到浏览器*/
#define USIF_CONTROL_JMP_MSC        0xff59   /*跳到MOSAIC*/ 
#define USIF_CONTROL_JMP_STOCK      0xff5a  /*跳到MOSAIC*/ 
#define USIF_CONTROL_JMP_GGPD		0xff5b	/*公共频道*/
#define USIF_CONTROL_JMP_YGZW		0xff5c	/*阳光政务*/
#define USIF_CONTROL_JMP_YYGB		0xff5d	/*音乐广播*/
#define USIF_CONTROL_JMP_FFPD		0xff5e	/*付费频道*/
#define USIF_CONTROL_JMP_ZXTJ		0xff5f	/*最新推荐*/
#define USIF_CONTROL_JMP_BFFW		0xff60	/*缤纷服务*/
#define USIF_CONTROL_UPDATEDBASE      0xff61/*节目参数变化修改*/
#define NUM_KEY                                 0x926d
#define GAME_KEY                               0xc13e
#define PROGM_SEARCH_KEY              0x6996
#define   EMAIL_KEY   0xb14e
#define FACTORY_KEY		0xb234			/* LJiang080915 add  工厂设置键 */
#define TVSHIFT_KEY		0xb235			/* LJiang080915 add 频道时移键 */
#define IPSWITCH_KEY	0xb236			/* LJiang080916 add 互动切换键 */
#define	FONT_EXIT_KEY				208	/*EXIT key */

#endif
/*end of file*/

