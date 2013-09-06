#include "..\main\initterm.h"
#include "ch_vfd.h"
#include "..\key\keymap.h"

void Yxl_VFDTest(void)
{
	BOOL res;
	int i;
	int iKeyScanCode;


	CH_CELL_TYPE CellTemp=CELL_MUTE;
	CH_LEDInit();


	res=CH_VFD_LOginOff();

	res=CH_VFD_EnableDisplay();


	CH_CellControl(CELL_MUTE,DISPLAY_ON);
	CH_CellControl(CELL_720P,DISPLAY_ON);
	CH_CellControl(CELL_576I,DISPLAY_ON);
	CH_CellControl(CELL_VIDEO,DISPLAY_ON);
	CH_CellControl(CELL_LINE,DISPLAY_ON);	
	CH_CellControl(CELL_RADIO,DISPLAY_ON);	
/*res=CH_VFD_WriteNumber(0,5,12345);*/



	while(true)
	{
		iKeyScanCode=GetKeyFromKeyQueue(1);	
		switch(iKeyScanCode)
		{
		case UP_ARROW_KEY:
				res=CH_VFD_ClearAll();
				break;
		case DOWN_ARROW_KEY:
			CellTemp=CELL_MUTE;
#if 1
			for(i=0;i<=(U8)CELL_LINE;i++)
			{
				
				res=CH_CellControl(CellTemp,DISPLAY_ON);
				CellTemp++;
			}
#endif
			break;
			case LEFT_ARROW_KEY:
				res=CH_VFD_WriteNumber(0,5,12345);
				break;
			case RIGHT_ARROW_KEY:
			CellTemp=CELL_MUTE;
#if 1
			for(i=0;i<=(U8)CELL_LINE;i++)
			{
				
				res=CH_CellControl(CellTemp,DISPLAY_OFF);
				CellTemp++;
			}
#endif
				break;
			case EXIT_KEY:
				return;
		/*	default:
				break;*/

		}
		
		
		/*res=CH_VFD_ClearAll();*/
		
	}

	return ;
}