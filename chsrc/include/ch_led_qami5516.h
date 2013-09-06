/*
	(c) Copyright changhong
 Name:ch_led.h
  Description:headet file about led  and light display for changhong QAM5516 DBVC platform
  Authors:yxl
  Date          Remark
  2004-07-16    Created
*/

#if 0
#define FOR_LOADER_USE /*YXL 2005-01-12 add this macro*/
#endif
#ifndef FOR_LOADER_USE /*YXL 2005-01-12 add this macro*/
typedef enum
{
	LIGHT_OFF,
	LIGHT_ON
}CH6_LightState;
#endif /*YXL 2005-01-12 add this macro*/


/*Function description:
	Initialize the LED display control task 
Parameters description: NO
	int Priority:the priority of display control task
return value:
	TRUE:stand for fail
	FALSE:stand for success
*/
extern boolean CH_LedInit(int Priority);


#ifndef FOR_LOADER_USE /*YXL 2005-01-12 add this macro*/
/*Function description:
	Display current system time on LED 
Parameters description: NO

return value:
	TRUE:stand for fail
	FALSE:stand for success
*/
extern BOOL DisplayTimeLED(void);


/*Function description:
	Display a digital number on LED 
Parameters description: 
	U32 num:the digital number to be displayed

return value:NO
	
*/
extern void DisplayLED(U32 num);



/*yxl 2004-12-08 add this function*/
extern BOOL DisplayLEDString(char* pDisString);


/*Function description:
	Turn off the power light
Parameters description: NO

return value:
	TRUE:stand for fail
	FALSE:stand for success
*/
extern BOOL PowerLightOff(void);


/*Function description:
	Turn on the power light
Parameters description: NO

return value:
	TRUE:stand for fail
	FALSE:stand for success
*/
extern BOOL PowerLightOn(void);


/*Function description:
	Turn off the tuner lock light
Parameters description: NO

return value:
	TRUE:stand for fail
	FALSE:stand for success
*/
extern BOOL TunerLightOff(void);

/*Function description:
	Turn on the tuner lock light
Parameters description: NO

return value:
	TRUE:stand for fail
	FALSE:stand for success
*/
extern BOOL TunerLightOn(void);

#else/*YXL 2005-01-12 add this macro*/


/*yxl 2005-01-12 add this line*/
extern void CH6_LEDDisplayLoader(void);

#endif /*YXL 2005-01-12 add this macro*/

