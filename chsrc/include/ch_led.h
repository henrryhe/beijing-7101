/*
	(c) Copyright changhong
 Name:ch_led.h
  Description:headet file about led  and light display for changhong 7710 platform
  Authors:yxl
  Date         Remark
  2005-08-23   Created
*/
#include "..\util\ch_time.h"


#if 0
#define FOR_LOADER_USE /*YXL 2005-01-12 add this macro*/
#endif

#if 0 /*yxl 2005-09-15 move this enum to ch_led.c file*/

#ifndef FOR_LOADER_USE /*YXL 2005-01-12 add this macro*/
typedef enum
{
	LIGHT_OFF,
	LIGHT_ON
}CH6_LightState;
#endif /*YXL 2005-01-12 add this macro*/

#endif 

/*Function description:
	Initialize the LED display control task 
Parameters description: NO
	int Priority:the priority of display control task
return value:
	TRUE:stand for fail
	FALSE:stand for success
*/
extern boolean CH_LedInit(ST_DeviceName_t I2CDeviceName,ST_DeviceName_t PIOLightDeviceName);



/*Function description:
	Display current system time on LED 
Parameters description: NO

return value:
	TRUE:stand for fail
	FALSE:stand for success
*/
#if 0  /*yxl 2006-02-18 modify this function,Add para 	TDTTIME DisTime*/
extern boolean DisplayTimeLED(void);
#else
extern boolean DisplayTimeLED(TDTTIME TempTime);
#endif /*end yxl 2006-02-18 modify this function,Add para 	TDTTIME DisTime*/

/*Function description:
	Display a digital number on LED 
Parameters description: 
	U32 num:the digital number to be displayed

return value:NO
	
*/
extern void DisplayLED(U32 iProgNo);


extern boolean DisplayFreqLED(int Freq);



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



extern void CH_LED_DisplayHDMode( void );
/*end of file*/

