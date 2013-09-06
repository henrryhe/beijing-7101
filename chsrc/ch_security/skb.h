/**
* ISKB2.h

* 定义Browser(双向业务)要求软键盘模块实现的接口函数，
* 即软键盘模块提供给Browser的接口

* 概念说明
*    密码图片：由平台前端生成的一张具有10个固定位置的图片 ，每个位置呈现0-9
*               中的一个数字（随机、不重复） ；
*    对应关系：指"密码图片"中具体某个位置所对应的数字；
*    焦点：    以透明的方式叠加在"密码图片"之上的方块；
*    口令坐标信息：用户选择口令时"焦点"所在的坐标位置，也即对应数字所在的坐标位置。
*/


#ifndef __SKB2BROWSER_H__
#define __SKB2BROWSER_H__




typedef enum
{
	 MSG_KEY_NUM0    = 0, //0，给JS传ASCII的A，添加一个“*”
	 MSG_KEY_NUM1,       //  //1-11是STB给SKB的键值，SKB不能返回这些值给STB
	 MSG_KEY_NUM2,
	 MSG_KEY_NUM3,
	 MSG_KEY_NUM4,
	 MSG_KEY_NUM5,
	 MSG_KEY_NUM6,
	 MSG_KEY_NUM7,
	 MSG_KEY_NUM8,
	 MSG_KEY_NUM9,

	 /* 遥控器上的快捷键值，非法按键给出无交互提示 */
	 MSG_KEY_UP      =10,//10，遥控器“上”键，基于中间件转换键值给JS（iPanel=1）
	 MSG_KEY_DOWN,       //11，遥控器“下”键，基于中间件转换键值给JS（iPanel=2）
	 MSG_KEY_LEFT,       //12，遥控器“左”键，基于中间件转换键值给JS（iPanel=3）
	 MSG_KEY_RIGHT,      //13，遥控器“右”键，基于中间件转换键值给JS（iPanel=4）
	 MSG_KEY_OK,         //14，遥控器“确定”键，基于中间件转换键值给JS（iPanel=13）
	 MSG_KEY_EXIT,       //15, 遥控器“退出”键，强行退出SKB（提示MSG_KEY_HINT3）
	 MSG_KEY_BACK,       //16, 遥控器“返回”键，删除一位口令，基于中间件转换键值给JS（iPanel=340）
	 MSG_KEY_TERMINATE,  //17, 遥控器“主页”键，返回主页（提示MSG_KEY_HINT3）

	 /* 新添加的键值 */
	 MSG_KEY_CLOSESKB,   //18，SKB上关闭按钮，基于中间件转换键值给JS（iPanel=67，'C'）
	 MSG_KEY_DEL,        //19，SKB上删除按钮，意义同MSG_KEY_BACK
	 MSG_KEY_HELP    =20,//20，SKB上帮助按钮，基于中间件转换键值给JS（iPanel=72，'H'）
	 
	 /* 提示信息的键值：73-90，ASCII的'I'-'Z'，JS弹出有交互的提示信息 */
	 MSG_KEY_HINT1   =73, //73，ASCII的I，提示密码位数不足XX位
	 MSG_KEY_HINT2,       //74，ASCII的J，提示密码已经达到最大位数
	 MSG_KEY_HINT3,       /*75，ASCII的K，提示用户：按“确定”退出SKB,按“返回”
						       继续口令输入，信息提示框上不带按钮；如果带按钮，需用左右
						       键移动焦点，由SKB计算焦点位置，其逻辑应与JS一致。*/
	                    			  //76-89，保留 
	 MSG_KEY_HINT18  =90, //90，ASCII的Z，提示错误的按键，JS从右下角浮出
	                      				//无需交互的帮助信息，自动消失。
} SKB_EV_KEYCODE;


#endif //__SKB2BROWSER_H__

