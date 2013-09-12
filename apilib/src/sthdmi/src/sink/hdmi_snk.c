/*******************************************************************************
File name   : hdmi_snk.c

Description : HDMI driver header file for SINK(receiver) side functions.

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#include <math.h>
#endif

#include "hdmi_snk.h"

/* Private Types ---------------------------------------------------------- */
typedef struct {
     char  IdManufacterChar;
     U32   Code;
 }sthdmi_IdManufacterCode_t;

typedef struct
{
     char HexNumber;
     char DecNumber;
}sthdmi_HextoDecConverter_t;



/* Private Constants ------------------------------------------------------ */
#define STHDMI_ID_MANUFACTER_CHAR1_MASK 0x7C00
#define STHDMI_ID_MANUFACTER_CHAR2_MASK 0x03E0
#define STHDMI_ID_MANUFACTER_CHAR3_MASK 0x001F

#define STHDMI_EDID_EXT_TAG_MONITOR_DESC 0x00    /* Monitor descriptor block */
#define STHDMI_EDID_EXT_TAG_LCD_TIMING   0x01    /* LCD Timings */
#define STHDMI_EDID_EXT_TAG_ADD_TIMING   0x02    /* Additional timing data type 2 */
#define STHDMI_EDID_EXT_TAG_EDID_VER2    0x20    /* EDID 2.0 Extension */
#define STHDMI_EDID_EXT_TAG_COLOR_INFO   0x30    /* Color Information type 0 */
#define STHDMI_EDID_EXT_TAG_DVI_FEATURE  0x40    /* DVI feature data */
#define STHDMI_EDID_EXT_TAG_TOUCH_SCREEN 0x50   /* touch screen data */
#define STHDMI_EDID_EXT_TAG_BLOCK_MAP    0xF0    /* Block Map */
#define STHDMI_EDID_EXT_TAG_MONITOR_MAN  0xFF    /* Extension Defined by  monitor manufacturer*/
#define STHDMI_EDID_DESCRIPTOR_DATA_END  0x0A    /* The End of the monitor Descriptor data*/
#define STHDMI_MAX_DESCRIPTOR_DATA_BYTE  0x0D    /* Defines the Max Descriptor data bytes */
#define STHDMI_MONITOR_PAD_FIELD         0x20    /* Value of Pad field */


/* The associated character was written in capital letter(No Low letters retrieved!)*/
static sthdmi_IdManufacterCode_t IdManufacterCharCode[26]=
{{'A',STHDMI_ID_MANUFACTER_CHAR_A_CODE},
 {'B',STHDMI_ID_MANUFACTER_CHAR_B_CODE},
 {'C',STHDMI_ID_MANUFACTER_CHAR_C_CODE},
 {'D',STHDMI_ID_MANUFACTER_CHAR_D_CODE},
 {'E',STHDMI_ID_MANUFACTER_CHAR_E_CODE},
 {'F',STHDMI_ID_MANUFACTER_CHAR_F_CODE},
 {'G',STHDMI_ID_MANUFACTER_CHAR_G_CODE},
 {'H',STHDMI_ID_MANUFACTER_CHAR_H_CODE},
 {'I',STHDMI_ID_MANUFACTER_CHAR_I_CODE},
 {'J',STHDMI_ID_MANUFACTER_CHAR_J_CODE},
 {'K',STHDMI_ID_MANUFACTER_CHAR_K_CODE},
 {'L',STHDMI_ID_MANUFACTER_CHAR_L_CODE},
 {'M',STHDMI_ID_MANUFACTER_CHAR_M_CODE},
 {'N',STHDMI_ID_MANUFACTER_CHAR_N_CODE},
 {'O',STHDMI_ID_MANUFACTER_CHAR_O_CODE},
 {'P',STHDMI_ID_MANUFACTER_CHAR_P_CODE},
 {'Q',STHDMI_ID_MANUFACTER_CHAR_Q_CODE},
 {'R',STHDMI_ID_MANUFACTER_CHAR_R_CODE},
 {'S',STHDMI_ID_MANUFACTER_CHAR_S_CODE},
 {'T',STHDMI_ID_MANUFACTER_CHAR_T_CODE},
 {'U',STHDMI_ID_MANUFACTER_CHAR_U_CODE},
 {'V',STHDMI_ID_MANUFACTER_CHAR_V_CODE},
 {'W',STHDMI_ID_MANUFACTER_CHAR_W_CODE},
 {'X',STHDMI_ID_MANUFACTER_CHAR_X_CODE},
 {'Y',STHDMI_ID_MANUFACTER_CHAR_Y_CODE},
 {'Z',STHDMI_ID_MANUFACTER_CHAR_Z_CODE}
 };

 static sthdmi_HextoDecConverter_t  HextoDecConverter[6]=
 {{'A', 10},
  {'B', 11},
  {'C', 12},
  {'D', 13},
  {'E', 14},
  {'F', 15}};

/* Private variables (static) --------------------------------------------- */
static BOOL  IsEDIDMemoryAllocated = FALSE;
/* Global Variables ------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes--------------------------------------------- */

static U32 sthdmi_GetColorParams(U32 LowBits, U32 HighBits, STHDMI_ColorType_t Color);
static U32 sthdmi_HextoDecConverter(char* Name_p, U32 Size);
static U32 sthdmi_GetDisplayParams(U32 ValueStored);
static U32 sthdmi_GetDisplayGamma(U32 ValueStored);
static void sthdmi_GetVideoInputDefinition(U32 ValueStored, STHDMI_VideoInputParams_t * const VideoInput);
static void sthdmi_GetVideoFeatureSupported (U32 ValueStored, STHDMI_FeatureSupported_t * const FeatureSupported);
static U32 sthdmi_GetEstablishedTimingI(U32 ValueStored);
static U32 sthdmi_GetEstablishedTimingII(U32 ValueStored);
static BOOL sthdmi_GetManufacturerTiming( U32 ValueStored);
static void sthdmi_GetStandardTimingParams(U32 ValueStored, U32 HorActivePixel, STHDMI_EDIDStandardTiming_t * const StdTiming);
static U32  sthdmi_GetDecValue (U32 ValueStored);
U32 sthdmi_GetNumberOfEdidTiming(U8* Buffer_p, U32 StartDetailedTiming);
static U32  sthdmi_GetGTFStartFrequency(U32 ValueStored);
static U32  sthdmi_GetGTFParams(U32 ValueStored);
static U32 sthdmi_GetGTFMParams (U32 ValueStored);
static void sthdmi_InitEDIDStruct(sthdmi_Unit_t * Unit_p);
#if defined (ST_OSLINUX)
static U32 sthdmi_LinuxPow (U32 x ,U32 y);
#endif /* ST_OSLINUX */


/* Exported Functions------------------------------------------------------ */
/*******************************************************************************
Name        :   sthdmi_GetIdManufacterName
Description :   retrieve the ID Manufacter Name based on ASCII compressed Code
Parameters  :   Code(in), IdManufacterName_p(out pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_GetIdManufacterName (U32 Code, char* IdManufacterName_p)
{
    U32 Index;
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    BOOL CharacterFound= FALSE;

    if (IdManufacterName_p==NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrieve the first character of the Id Manufacter Name.. */
    for (Index=0; Index<26 ;Index++)
    {
       if ((Code&STHDMI_ID_MANUFACTER_CHAR1_MASK)>>10 == IdManufacterCharCode[Index].Code)
       {
          CharacterFound= TRUE;
          break; /* found */
       }
    }

    (CharacterFound)?(*IdManufacterName_p=(char)IdManufacterCharCode[Index].IdManufacterChar):(*IdManufacterName_p='?');
    CharacterFound= FALSE;
    IdManufacterName_p++;

    /* Retrieve the Second character of the Id Manufacter Name.. */
    for (Index=0;Index<26 ; Index++)
    {
       if ((Code&STHDMI_ID_MANUFACTER_CHAR2_MASK)>>5== IdManufacterCharCode[Index].Code)
       {
          CharacterFound= TRUE;
          break; /* found */
       }
    }
    (CharacterFound)?(*IdManufacterName_p=(char)IdManufacterCharCode[Index].IdManufacterChar):(*IdManufacterName_p='?');
    CharacterFound= FALSE;
    IdManufacterName_p++;

    /* Retrieve the third character of the Id Manufacter Name.. */
    for (Index=0;Index<26 ; Index++)
    {
       if ((Code&STHDMI_ID_MANUFACTER_CHAR3_MASK)== IdManufacterCharCode[Index].Code)
       {
          CharacterFound= TRUE;
          break; /* found */
       }
    }
   (CharacterFound)?(*IdManufacterName_p=(char)IdManufacterCharCode[Index].IdManufacterChar):(*IdManufacterName_p='?');

   return(ErrorCode);

} /* end of sthdmi_GetIdManufacterName()*/
/*******************************************************************************
Name        :   sthdmi_HextoDecConverter
Description :   Converts Numbers from Hexadecimal to decimal representation
Parameters  :   Hexadecimal Number, Number of bytes(Size)
Assumptions :
Limitations :
Returns     :  none
*******************************************************************************/
static U32 sthdmi_HextoDecConverter(char* Name_p, U32 Size)
{
      U32 Index;
      U32 DecNumber=0;
      BOOL Found=FALSE;
      #ifdef ST_7710
      U32  Sum, Index1;
      #endif
      while ((Name_p!=NULL)&&(Size!=0))
      {
        for (Index=0;Index<6; Index++)
        {
           if (HextoDecConverter[Index].HexNumber== (char)*Name_p)
           {
             Found=TRUE;
             break; /* found */
           }
        }


#if defined(ST_7100)||defined(ST_7109)|| defined (ST_7200)
#if defined (ST_OSLINUX)

        (DecNumber +=(U32)((*Name_p))*(sthdmi_LinuxPow(16,(Size-1))));

#else
        (Found)?(DecNumber += HextoDecConverter[Index].DecNumber *(pow(16,(Size-1)))):
        (DecNumber +=(U32)(*Name_p)*(pow(16,(Size-1))));


#endif  /* ST_OSLINUX */
#endif

#ifdef ST_7710
        Sum =1;
        for (Index1=1;Index1<Size;Index1++)Sum*=16;
        (Found)?(DecNumber += HextoDecConverter[Index].DecNumber *Sum):
        (DecNumber +=(U32)(*Name_p)*Sum);
#endif

        Found =FALSE;/*pos++;*/Name_p++;Size--;
     }
  return(DecNumber);
} /* end of sthdmi_HextoDecConverter() */
/*******************************************************************************
Name        :   sthdmi_GetIDSerialNumber
Description :   Retrieve the last 5 digits of serial Number.
Parameters  :
Assumptions :
Limitations :
Returns     :  Id serial Number
*******************************************************************************/
static U32 sthdmi_GetIDSerialNumber (U32 IdSerialNumberCode)
{
  U32 IdSerialNumber;
  char IdSerialCode[2];

  IdSerialNumber=0;

  IdSerialCode[0]= (IdSerialNumberCode&STHDMI_ID_SER_NUM_BYTE1_MASK1)>>28;
  IdSerialCode[1]= (IdSerialNumberCode&STHDMI_ID_SER_NUM_BYTE1_MASK2)>>24;

  IdSerialNumber += sthdmi_HextoDecConverter(IdSerialCode,2);              /* 2^0 */

  IdSerialCode[0]= (IdSerialNumberCode&STHDMI_ID_SER_NUM_BYTE2_MASK1)>>20;
  IdSerialCode[1]= (IdSerialNumberCode&STHDMI_ID_SER_NUM_BYTE2_MASK2)>>16;
  IdSerialNumber += sthdmi_HextoDecConverter(IdSerialCode,2)*256;        /* 1*(2^8) */

  IdSerialCode[0]= (IdSerialNumberCode&STHDMI_ID_SER_NUM_BYTE3_MASK1)>>12;
  IdSerialCode[1]= (IdSerialNumberCode&STHDMI_ID_SER_NUM_BYTE3_MASK2)>>8;
  IdSerialNumber += sthdmi_HextoDecConverter(IdSerialCode,2)*512;        /* 2*(2^8) */

  IdSerialCode[0]= (IdSerialNumberCode&STHDMI_ID_SER_NUM_BYTE4_MASK1)>>4;
  IdSerialCode[1]= (IdSerialNumberCode&STHDMI_ID_SER_NUM_BYTE4_MASK2);
  IdSerialNumber += sthdmi_HextoDecConverter(IdSerialCode,2)*768;   /* 3*(2^8) */

  return(IdSerialNumber);

} /* end of sthdmi_GetIDSerialNumber()*/

/*******************************************************************************
Name        :   sthdmi_GetYearOfManufacter
Description :   Retrieve the Year of Manufacter which represent the year
                of monitor's manufacter.
Parameters  :  Stored Value(U32)
Assumptions :
Limitations :
Returns     : Year of Manufacter
*******************************************************************************/
static U32  sthdmi_GetYearOfManufacter (U32 ValueStored)
{
    /* The value stored is an offset from the year 1990 as derived from the following equation :*/
    /* Year of manufacter = Value Stored + 1990 */
    char Buffer[2];
    Buffer[0]= (ValueStored&0xF0)>>4;
    Buffer[1]= (ValueStored&0x0F);
    return(sthdmi_HextoDecConverter(Buffer,2)+1990);
} /* end of sthdmi_GetYearOfManufacter()*/
/*******************************************************************************
Name        :   sthdmi_GetDisplayParams
Description :   Retrieve the basic display parameters
Parameters  :   Stored Value (U32)
Assumptions :
Limitations :
Returns     :  U32
*******************************************************************************/
static U32 sthdmi_GetDisplayParams(U32 ValueStored)
{
    char Buffer[2];
    Buffer[0]= (ValueStored&0xF0)>>4;
    Buffer[1]= (ValueStored&0x0F);
    return(sthdmi_HextoDecConverter(Buffer,2)*10);

} /* end of sthdmi_GetDisplayParams()*/

/*******************************************************************************
Name        :   sthdmi_GetDisplayGamma
Description :   Retrieve the basic display parameters
Parameters  :   Stored Value (U32)
Assumptions :
Limitations :
Returns     : (Gamma display parameter) multiplied by 100
*******************************************************************************/
static U32 sthdmi_GetDisplayGamma(U32 ValueStored)
{
  char Buffer[2];
  Buffer[0]= (ValueStored&0xF0)>>4;
  Buffer[1]= (ValueStored&0x0F);
  /*return((double)(sthdmi_HextoDecConverter(Buffer,2)+100)/100);*/
  return(sthdmi_HextoDecConverter(Buffer,2)+100);
}  /* end of sthdmi_GetDisplayGamma()*/
/*******************************************************************************
Name        :   sthdmi_GetDecValue
Description :   Converts Hexadecimal to Decimal Value
Parameters  :   Stored Value
Assumptions :
Limitations :
Returns     :  Decimal Value returned
*******************************************************************************/
static U32  sthdmi_GetDecValue (U32 ValueStored)
{
  char Buffer[2];
  Buffer[0]= (ValueStored&0xF0)>>4;
  Buffer[1]= (ValueStored&0x0F);
  return(sthdmi_HextoDecConverter(Buffer,2));
} /* end of sthdmi_GetDecValue()*/

/*******************************************************************************
Name        :   sthdmi_GetCTFStartFrequency
Description :   Get Start Frequency for Secondary Curve
Parameters  :   Stored Value
Assumptions :
Limitations :
Returns     :  Start Frequency in kHz
*******************************************************************************/
static U32  sthdmi_GetGTFStartFrequency(U32 ValueStored)
{
  char Buffer[2];
  Buffer[0]= (ValueStored&0xF0)>>4;
  Buffer[1]= (ValueStored&0x0F);
  return(sthdmi_HextoDecConverter(Buffer,2)*2);
} /* end of sthdmi_GetGTFStartFrequency()*/
/*******************************************************************************
Name        :   sthdmi_GetGTFCParams
Description :   Get C or J parameter for Generalized Timing Formula (GTF)
Parameters  :   Stored Value
Assumptions :
Limitations :
Returns     :  C or J parameter (in decimal)
*******************************************************************************/
static U32  sthdmi_GetGTFParams(U32 ValueStored)
{
  char Buffer[2];
  Buffer[0]= (ValueStored&0xF0)>>4;
  Buffer[1]= (ValueStored&0x0F);
  return(sthdmi_HextoDecConverter(Buffer,2)/2);

} /* end of sthdmi_GetGTFParams */
/*******************************************************************************
Name        :   sthdmi_GetGTFMParams
Description :   Get M parameter for Generalized Timing Formula (GTF)
Parameters  :   Stored Value
Assumptions :
Limitations :
Returns     :  M parameter (in decimal)
*******************************************************************************/
static U32 sthdmi_GetGTFMParams (U32 ValueStored)
{
  char Buffer[4];
  Buffer[0]= (ValueStored&0xF000)>>16;
  Buffer[1]= (ValueStored&0xF00)>>8;
  Buffer[2]= (ValueStored&0xF0)>>4;
  Buffer[3]= (ValueStored&0x0F);
  return(sthdmi_HextoDecConverter(Buffer,4));

} /* end of sthdmi_GetGTFMParams()*/


#if defined (ST_OSLINUX)
/*******************************************************************************
Name        :   sthdmi_LinuxPow
Description :   Special to linux in order to do x power y (x ^ y)
Parameters  :   U32 X, U32 Y
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 sthdmi_LinuxPow(U32 X , U32 Y)
{
  U32 Result = 1;
  int i=0;
  for ( i=0; i<Y; i++)
  {
     Result = Result*X;
  }
  return(Result);
} /* end of sthdmi_LinuxPow()*/

#endif /* ST_OSLINUX */

/*******************************************************************************
Name        :   sthdmi_GetVideoInputDefinition
Description :   Retrieve the Video Input Definition
Parameters  :   Stored Value(in), Video input definition (pointer)
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void sthdmi_GetVideoInputDefinition(U32 ValueStored, STHDMI_VideoInputParams_t * const VideoInput)
{
    if ((ValueStored& STHDMI_DIGITAL_SIGNAL_LEVEL)==STHDMI_DIGITAL_SIGNAL_LEVEL)
    {

        /* Analog signal level */
        VideoInput->IsDigitalSignal =TRUE;

        /* Set all bits (1->6) to '0' */
        VideoInput->SignalLevelMax =0;
        VideoInput->SignalLevelMin =0;

        VideoInput->IsSetupExpected= FALSE;
        VideoInput->IsSepSyncSupported= FALSE;
        VideoInput->IsCompSyncSupported=FALSE;
        VideoInput->IsSyncOnGreenSupported=FALSE;
        VideoInput->IsPulseRequired=FALSE;

        /* Retrieve whether interface is signal compatible with VESA DFP 1.X TMDS CRGB */
        ((ValueStored&STHDMI_VIDEO_DFP_COMPATBLE)==STHDMI_VIDEO_DFP_COMPATBLE)?(VideoInput->IsVesaDFPCompatible=TRUE):
        (VideoInput->IsVesaDFPCompatible=FALSE);

    }
    else
    {
        /* all parameters settings are multiplied by 1000 */
        VideoInput->IsDigitalSignal = FALSE;

        if ((ValueStored&STHDMI_SIGNAL_LEVEL_STANDARD_OP1)==STHDMI_SIGNAL_LEVEL_STANDARD_OP1)
        {
            VideoInput->SignalLevelMax =700;
            VideoInput->SignalLevelMin =300;

        }
        else if ((ValueStored&STHDMI_SIGNAL_LEVEL_STANDARD_OP2)==STHDMI_SIGNAL_LEVEL_STANDARD_OP2)
        {
            VideoInput->SignalLevelMax =714;
            VideoInput->SignalLevelMin =286;
        }
        else if ((ValueStored&STHDMI_SIGNAL_LEVEL_STANDARD_OP3)==STHDMI_SIGNAL_LEVEL_STANDARD_OP3)
        {
            VideoInput->SignalLevelMax =1000;
            VideoInput->SignalLevelMin =400;
        }
        else
        {
            VideoInput->SignalLevelMax =700;
            VideoInput->SignalLevelMin =000;
        }
        /* retrieve the blank-to-black setup.. */
        ((ValueStored&STHDMI_VIDEO_BLANKTOBLACK_SETUP)==STHDMI_VIDEO_BLANKTOBLACK_SETUP)?(VideoInput->IsSetupExpected=TRUE):
        (VideoInput->IsSetupExpected=FALSE);

        /* retrieve whether the seperate sync is supported */
        ((ValueStored&STHDMI_VIDEO_SEPERATE_SYNC)==STHDMI_VIDEO_SEPERATE_SYNC)?(VideoInput->IsSepSyncSupported=TRUE):
        (VideoInput->IsSepSyncSupported=FALSE);

       /* retrieve whether the composite sync is supported */
       ((ValueStored&STHDMI_VIDEO_COMPOSITE_SYNC)==STHDMI_VIDEO_COMPOSITE_SYNC)?(VideoInput->IsCompSyncSupported=TRUE):
       (VideoInput->IsCompSyncSupported=FALSE);

       /* Retrieve whether sync on green video is supported */
       ((ValueStored&STHDMI_VIDEO_SYNC_ON_GREEN)==STHDMI_VIDEO_SYNC_ON_GREEN)?(VideoInput->IsSyncOnGreenSupported=TRUE):
       (VideoInput->IsSyncOnGreenSupported=FALSE);

       /* Retrieve whether serration of the Vsync is supported */
       ((ValueStored&STHDMI_VIDEO_SERRATION_OF_VSYNC)==STHDMI_VIDEO_SERRATION_OF_VSYNC)?(VideoInput->IsPulseRequired=TRUE):
       (VideoInput->IsPulseRequired=FALSE);

       VideoInput->IsVesaDFPCompatible=FALSE;
   }

} /* end of sthdmi_GetVideoInputDefinition() */
/*******************************************************************************
Name        :   sthdmi_GetVideoFeatureSupported
Description :   Retrieve the Video Feature Supported
Parameters  :   Stored Value(in), Video Feature Supported (pointer)
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void sthdmi_GetVideoFeatureSupported (U32 ValueStored, STHDMI_FeatureSupported_t * const FeatureSupported)
{
    ((ValueStored&STHDMI_FEATURE_STANDBY_ENABLED)==STHDMI_FEATURE_STANDBY_ENABLED)?(FeatureSupported->IsStandby=TRUE):
     (FeatureSupported->IsStandby=FALSE);

    ((ValueStored&STHDMI_FEATURE_SUSPEND_ENABLED)==STHDMI_FEATURE_SUSPEND_ENABLED)?(FeatureSupported->IsSuspend=TRUE):
    (FeatureSupported->IsSuspend=FALSE);

    ((ValueStored&STHDMI_FEATURE_ACTIVE_OFF)==STHDMI_FEATURE_ACTIVE_OFF)? (FeatureSupported->IsActiveOff=TRUE):
    (FeatureSupported->IsActiveOff=FALSE);

    if ((ValueStored&STHDMI_FEATURE_DISPLAY_TYPE_4)==STHDMI_FEATURE_DISPLAY_TYPE_4)
    {
        FeatureSupported->DisplayType= STHDMI_EDID_DISPLAY_TYPE_UNDEFINED;

    }
    else if ((ValueStored&STHDMI_FEATURE_DISPLAY_TYPE_2)==STHDMI_FEATURE_DISPLAY_TYPE_2)
    {
        FeatureSupported->DisplayType= STHDMI_EDID_DISPLAY_TYPE_RGB;
    }
    else if ((ValueStored&STHDMI_FEATURE_DISPLAY_TYPE_3)==STHDMI_FEATURE_DISPLAY_TYPE_3)
    {
        FeatureSupported->DisplayType= STHDMI_EDID_DISPLAY_TYPE_NON_RGB;
    }
    else
    {
        FeatureSupported->DisplayType= STHDMI_EDID_DISPLAY_TYPE_MONOCHROME;
    }

    ((ValueStored&STHDMI_FEATURE_RGB_STD_DEFAULT)==STHDMI_FEATURE_RGB_STD_DEFAULT)?(FeatureSupported->IsRGBDefaultColorSpace=TRUE):
    (FeatureSupported->IsRGBDefaultColorSpace=FALSE);

    ((ValueStored&STHDMI_FEATURE_PREFERRED_MODE)==STHDMI_FEATURE_PREFERRED_MODE)?(FeatureSupported->IsPreffredTimingMode=TRUE):
    (FeatureSupported->IsPreffredTimingMode=FALSE);

    ((ValueStored&STHDMI_FEATURE_GTF_SUPPORTED)==STHDMI_FEATURE_GTF_SUPPORTED)?(FeatureSupported->IsDefaultGTFSupported=TRUE):
    (FeatureSupported->IsDefaultGTFSupported=FALSE);

} /* end of sthdmi_GetVideoFeatureSupported() */
/*******************************************************************************
Name        :   sthdmi_GetEstablishedTimingI
Description :   Retrieve All the Established TimingI supported
Parameters  :   Stored Value(in)
Assumptions :
Limitations :
Returns     : U32
*******************************************************************************/
static U32 sthdmi_GetEstablishedTimingI(U32 ValueStored)
{
    U32 EstablishedTiming= 0;

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_8)==STHDMI_ESTABLISHED_TIMING_BIT_8)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_720_400_70):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_7)==STHDMI_ESTABLISHED_TIMING_BIT_7)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_720_400_88):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_6)==STHDMI_ESTABLISHED_TIMING_BIT_6)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_640_480_60):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_5)==STHDMI_ESTABLISHED_TIMING_BIT_5)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_640_480_67):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_4)==STHDMI_ESTABLISHED_TIMING_BIT_4)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_640_480_72):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_3)==STHDMI_ESTABLISHED_TIMING_BIT_3)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_640_480_75):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_2)==STHDMI_ESTABLISHED_TIMING_BIT_2)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_800_600_56):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_1)==STHDMI_ESTABLISHED_TIMING_BIT_1)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_800_600_60):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    return(EstablishedTiming);
} /* end of sthdmi_GetEstablishedTimingI() */

/*******************************************************************************
Name        :   sthdmi_GetEstablishedTimingII
Description :   Retrieve All the Established TimingII supported
Parameters  :   Stored Value(in)
Assumptions :
Limitations :
Returns     : U32
*******************************************************************************/
static U32 sthdmi_GetEstablishedTimingII(U32 ValueStored)
{
    U32 EstablishedTiming= 0;

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_8)==STHDMI_ESTABLISHED_TIMING_BIT_8)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_800_600_72):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_7)==STHDMI_ESTABLISHED_TIMING_BIT_7)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_800_600_75):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_6)==STHDMI_ESTABLISHED_TIMING_BIT_6)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_832_624_75):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_5)==STHDMI_ESTABLISHED_TIMING_BIT_5)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_1024_768_87):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_4)==STHDMI_ESTABLISHED_TIMING_BIT_4)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_1024_768_60):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_3)==STHDMI_ESTABLISHED_TIMING_BIT_3)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_1024_768_70):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_2)==STHDMI_ESTABLISHED_TIMING_BIT_2)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_1024_768_75):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    ((ValueStored &STHDMI_ESTABLISHED_TIMING_BIT_1)==STHDMI_ESTABLISHED_TIMING_BIT_1)?
            (EstablishedTiming |=STHDMI_EDID_TIMING_1280_1024_75):(EstablishedTiming |=STHDMI_ESTABLISHED_TIMING_NONE);

    return(EstablishedTiming);
} /* end of sthdmi_GetEstablishedTimingII() */
/*******************************************************************************
Name        :   sthdmi_GetManufacturerTiming
Description :   Retrieve All the Manufacturer's Timing
Parameters  :   Stored Value(in)
Assumptions :
Limitations :
Returns     : TRUE(1152*870@75Hz) FALSE: not specified.
*******************************************************************************/
static BOOL sthdmi_GetManufacturerTiming( U32 ValueStored)
{
    BOOL  IsManTimingSpecified;

    ((ValueStored&STHDMI_ESTABLISHED_TIMING_BIT_8)== STHDMI_ESTABLISHED_TIMING_BIT_8)?
            (IsManTimingSpecified=TRUE):(IsManTimingSpecified=FALSE);

     return(IsManTimingSpecified);
} /* end of sthdmi_GetManufacturerTiming()*/

/*******************************************************************************
Name        :   sthdmi_GetStandardTimingParams
Description :   Retrieve Standard Timing Params
Parameters  :   Stored Value(in), Horizontal active pixel
Assumptions :
Limitations :
Returns     :  none
*******************************************************************************/
static void sthdmi_GetStandardTimingParams(U32 ValueStored,U32 HorActivePixel,STHDMI_EDIDStandardTiming_t * const StdTiming)
{
    STHDMI_AspectRatio_t  AspectRatio;
    /*U32                   HorizontalActive;*/

    if ((ValueStored==0x01)&(HorActivePixel==0x01))
    {
        /* This standard Timing mode is not used */
        StdTiming->IsStandardTimingUsed=FALSE;
        StdTiming->HorizontalActive=0;
        StdTiming->VerticalActive=0;
        StdTiming->RefreshRate=0;
    }
    else
    {
        StdTiming->IsStandardTimingUsed = TRUE;

        if ((ValueStored&STHDMI_TIMING_ID_ASPECT_RATIO_16_10)==STHDMI_TIMING_ID_ASPECT_RATIO_16_10)
        {
            AspectRatio=STHDMI_ASPECT_RATIO_16_10;
        }
        else if ((ValueStored&STHDMI_TIMING_ID_ASPECT_RATIO_4_3)==STHDMI_TIMING_ID_ASPECT_RATIO_4_3)
        {
            AspectRatio=STHDMI_ASPECT_RATIO_4_3;
        }
        else if ((ValueStored&STHDMI_TIMING_ID_ASPECT_RATIO_5_4)==STHDMI_TIMING_ID_ASPECT_RATIO_5_4)
        {
            AspectRatio=STHDMI_ASPECT_RATIO_5_4;
        }
        else
        {
            AspectRatio=STHDMI_ASPECT_RATIO_16_9;
        }
        /* Copy the Horizontal active pixel... */
        StdTiming->HorizontalActive = HorActivePixel;

        /* Calculates the vertical active line */
        switch (AspectRatio)
        {
            case STHDMI_ASPECT_RATIO_16_10 :
                StdTiming->VerticalActive = (HorActivePixel/16)*10;
                break;

            case STHDMI_ASPECT_RATIO_4_3 :
                StdTiming->VerticalActive = (HorActivePixel/4)*3;
                break;

            case STHDMI_ASPECT_RATIO_5_4 :
                StdTiming->VerticalActive = (HorActivePixel/5)*4;
                break;
            case STHDMI_ASPECT_RATIO_16_9 :
                StdTiming->VerticalActive = (HorActivePixel/16)*9;
                break;
            default :
                break;
        }


        /* Calculate the Refresh Rate */
        StdTiming->RefreshRate = sthdmi_GetDecValue(ValueStored&STHDMI_TIMING_REFRESF_RATE_MASK)+60;
       }

} /* end of sthdmi_GetStandardTimingParams */
/*******************************************************************************
Name        :  sthdmi_GetColorParams
Description :  Retrieve color characteristic
Parameters  :  Low bits (0->1), High bits(2->9), Color(Red/Green/Blue/White)
Assumptions :
Limitations :
Returns     : Color Characteristic (float)
*******************************************************************************/
static U32 sthdmi_GetColorParams(U32 LowBits, U32 HighBits, STHDMI_ColorType_t Color)
{
    U32 UValue =0;
#if defined (ST_OSLINUX)
    U32 ColorParams=0;
#else
    double ColorParams=0;
#endif

    /* Concatenate the low bits(0->1) and High bits(2->9) */
    switch (Color)
    {
        case STHDMI_RED_X_COLOR :
            UValue = ((LowBits&STHDMI_COLOR_RED_X_LOW_BITS)>>6)|(HighBits<<2);
            break;

        case STHDMI_RED_Y_COLOR :
            UValue = ((LowBits&STHDMI_COLOR_RED_Y_LOW_BITS)>>4)|(HighBits<<2);
            break;

        case STHDMI_GREEN_X_COLOR :
            UValue = ((LowBits&STHDMI_COLOR_GREEN_X_LOW_BITS)>>2)|(HighBits<<2);
            break;

        case STHDMI_GREEN_Y_COLOR :
            UValue = (LowBits&STHDMI_COLOR_GREEN_Y_LOW_BITS)|(HighBits<<2);
            break;

        case STHDMI_BLUE_X_COLOR :
            UValue = ((LowBits&STHDMI_COLOR_BLUE_X_LOW_BITS)>>6)|(HighBits<<2);
            break;

        case STHDMI_BLUE_Y_COLOR :
            UValue = ((LowBits&STHDMI_COLOR_BLUE_Y_LOW_BITS)>>4)|(HighBits<<2);
            break;

        case STHDMI_WHITE_X_COLOR :
           UValue = ((LowBits&STHDMI_COLOR_WHITE_X_LOW_BITS)>>2)|(HighBits<<2);
           break;

        case STHDMI_WHITE_Y_COLOR :
           UValue = (LowBits&STHDMI_COLOR_WHITE_Y_LOW_BITS)|(HighBits<<2);
           break;
        default :
            break;
    }

#if defined(ST_OSLINUX)
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT0)==STHDMI_COLOR_CHARACTERISTIC_BIT0)?((U32)(ColorParams+=9 /*1/1024*/))
     :(ColorParams+=0); /*2^10*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT1)==STHDMI_COLOR_CHARACTERISTIC_BIT1)?((U32)(ColorParams+=19 /*1/512*/))
     :(ColorParams+=0);  /*2^9*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT2)==STHDMI_COLOR_CHARACTERISTIC_BIT2)?((U32)(ColorParams+=39 /*1/256*/))
     :(ColorParams+=0);  /*2^8*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT3)==STHDMI_COLOR_CHARACTERISTIC_BIT3)?((U32)(ColorParams+=78 /*1/128*/))
     :(ColorParams+=0);  /*2^7*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT4)==STHDMI_COLOR_CHARACTERISTIC_BIT4)?((U32)(ColorParams+=156 /*1/64*/))
     :(ColorParams+=0);   /*2^6*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT5)==STHDMI_COLOR_CHARACTERISTIC_BIT5)?((U32)(ColorParams+=312 /*1/32*/))
     :(ColorParams+=0);   /*2^5*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT6)==STHDMI_COLOR_CHARACTERISTIC_BIT6)?((U32)(ColorParams+=625 /*1/16*/))
     :(ColorParams+=0);   /*2^4*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT7)==STHDMI_COLOR_CHARACTERISTIC_BIT7)?((U32)(ColorParams+=1250 /*1/8*/))
     :(ColorParams+=0);    /*2^3*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT8)==STHDMI_COLOR_CHARACTERISTIC_BIT8)?((U32)(ColorParams+=2500 /*1/4)*/))
     :(ColorParams+=0);      /*4 */
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT9)==STHDMI_COLOR_CHARACTERISTIC_BIT9)?((U32)(ColorParams+=5000 /*1/2*/))
     :(ColorParams+=0);
#else
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT0)==STHDMI_COLOR_CHARACTERISTIC_BIT0)?((ColorParams+=0.0009765625 /*1/1024*/))
     :(ColorParams+=0.0); /*2^10*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT1)==STHDMI_COLOR_CHARACTERISTIC_BIT1)?((ColorParams+=0.001953125 /*1/512*/))
     :(ColorParams+=0.0);  /*2^9*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT2)==STHDMI_COLOR_CHARACTERISTIC_BIT2)?((ColorParams+=0.00390625 /*1/256*/))
     :(ColorParams+=0.0);  /*2^8*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT3)==STHDMI_COLOR_CHARACTERISTIC_BIT3)?((ColorParams+=0.0078125 /*1/128*/))
     :(ColorParams+=0.0);  /*2^7*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT4)==STHDMI_COLOR_CHARACTERISTIC_BIT4)?((ColorParams+=0.015625 /*1/64*/))
     :(ColorParams+=0.0);   /*2^6*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT5)==STHDMI_COLOR_CHARACTERISTIC_BIT5)?((ColorParams+=0.03125 /*1/32*/))
     :(ColorParams+=0.0);   /*2^5*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT6)==STHDMI_COLOR_CHARACTERISTIC_BIT6)?((ColorParams+=0.0625 /*1/16*/))
     :(ColorParams+=0.0);   /*2^4*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT7)==STHDMI_COLOR_CHARACTERISTIC_BIT7)?((ColorParams+=0.125 /*1/8*/))
     :(ColorParams+=0.0);    /*2^3*/
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT8)==STHDMI_COLOR_CHARACTERISTIC_BIT8)?((ColorParams+=0.25 /*1/4)*/))
     :(ColorParams+=0.0);      /*4 */
    ((UValue&STHDMI_COLOR_CHARACTERISTIC_BIT9)==STHDMI_COLOR_CHARACTERISTIC_BIT9)?((ColorParams+=0.5 /*1/2*/))
     :(ColorParams+=0.0);      /*2 */
#endif /*ST_OSLINUX  */
    return((U32)(ColorParams*1000));

} /* end of sthdmi_GetColorParams()*/
/*******************************************************************************
Name        :   sthdmi_FillEDIDTimingDescriptor
Description :   Fill EDID Structure.
Parameters  :   Buffer_p (pointer), TimingDescriptor(pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_FillEDIDTimingDescriptor(U8* Buffer_p, STHDMI_EDIDTimingDescriptor_t* const  TimingDesc_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 TmpBuffer1=0;U32 MiscParams=0; U32 LowBits1=0;
    U32 LowBits2=0; U32 LowBits3=0; U32 HighBits=0;
    char TmpBuffer[5];

    if ((TimingDesc_p == NULL)||(Buffer_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);

    }
    memcpy(TmpBuffer,Buffer_p,5);
    if((TmpBuffer[0]||TmpBuffer[1]||TmpBuffer[2]||TmpBuffer[4])==0) /*specific types of data*/
    {
        return(ST_NO_ERROR);
    }
    LowBits1= (U8)((*Buffer_p)&0xFF);Buffer_p++;
    HighBits= (U8)((*Buffer_p)&0xFF);Buffer_p++;
    TmpBuffer[0] = (((HighBits<<8)|LowBits1)&0xF000)>>12;
    TmpBuffer[1] = (((HighBits<<8)|LowBits1)&0xF00)>>8;
    TmpBuffer[2] = (((HighBits<<8)|LowBits1)&0xF0)>>4;
    TmpBuffer[3] = (((HighBits<<8)|LowBits1)&0xF);
    /* The pixel clock was multilplied by 100, you divide by 100 to get the real one */
    TimingDesc_p->PixelClock= sthdmi_HextoDecConverter(TmpBuffer,4);

   /* Calculates the Horizontal Active Pixel and Horizontal Active Pixel using 12bits of the EDID table*/
    LowBits1 = (U8)(*Buffer_p);Buffer_p++;
    LowBits2 = (U8)(*Buffer_p);Buffer_p++;
    HighBits = (U8)(*Buffer_p);Buffer_p++;
   /* TmpDescriptor = (((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4)|LowBits1);   */

    TmpBuffer[0]= ((((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4)|LowBits1)&0xF00)>>8;
    TmpBuffer[1]= ((((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4)|LowBits1)&0x0F0)>>4;
    TmpBuffer[2]= ((((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4)|LowBits1)&0x00F);
    TimingDesc_p->HorActivePixel = sthdmi_HextoDecConverter(TmpBuffer,3);

    TmpBuffer[0] = ((((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8)|LowBits2)&0xF00)>>8;
    TmpBuffer[1] = ((((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8)|LowBits2)&0xF0)>>4;
    TmpBuffer[2] = ((((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8)|LowBits2)&0x00F);
    TimingDesc_p->HorBlankingPixel = sthdmi_HextoDecConverter(TmpBuffer,3);

   /* Calculates the Vertical Active lines and the vertical Blanking line using 12bits of the EDID table*/
    LowBits1 = (U8)(*Buffer_p);Buffer_p++;
    LowBits2 = (U8)(*Buffer_p);Buffer_p++;
    HighBits = (U8)(*Buffer_p);Buffer_p++;

    TmpBuffer[0] = ((((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4)|LowBits1)&0xF00)>>8;
    TmpBuffer[1] = ((((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4)|LowBits1)&0x0F0)>>4;
    TmpBuffer[2] = ((((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4)|LowBits1)&0x00F);
    TimingDesc_p->VerActiveLine = sthdmi_HextoDecConverter(TmpBuffer,3);

    TmpBuffer[0] = ((((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8)|LowBits2)&0xF00)>>8;
    TmpBuffer[1] = ((((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8)|LowBits2)&0x0F0)>>4;
    TmpBuffer[2] = ((((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8)|LowBits2)&0x00F);
    TimingDesc_p->VerBlankingLine = sthdmi_HextoDecConverter(TmpBuffer,3);

   /* Calculates the Horizontal sync offset and Horizontal sync pulse width 10bits*/
    LowBits1 = (U8)(*Buffer_p);Buffer_p++;
    LowBits2 = (U8)(*Buffer_p);Buffer_p++;
    LowBits3 = (U8)(*Buffer_p);Buffer_p++;
    HighBits = (U8)(*Buffer_p);Buffer_p++;

    TmpBuffer[0] = ((((HighBits&STHDMI_TIMING_BITS67_MASK)<<2)|LowBits1)&0xF00)>>8;
    TmpBuffer[1] = ((((HighBits&STHDMI_TIMING_BITS67_MASK)<<2)|LowBits1)&0x0F0)>>4;
    TmpBuffer[2] = ((((HighBits&STHDMI_TIMING_BITS67_MASK)<<2)|LowBits1)&0x00F);
    TimingDesc_p->HSyncOffset = sthdmi_HextoDecConverter(TmpBuffer,3);

    TmpBuffer[0] = ((((HighBits&STHDMI_TIMING_BITS45_MASK)<<4)|LowBits2)&0xF00)>>8;
    TmpBuffer[1] = ((((HighBits&STHDMI_TIMING_BITS45_MASK)<<4)|LowBits2)&0x0F0)>>4;
    TmpBuffer[2] = ((((HighBits&STHDMI_TIMING_BITS45_MASK)<<4)|LowBits2)&0x00F);
    TimingDesc_p->HSyncWidth = sthdmi_HextoDecConverter(TmpBuffer,3);

    /* Calculates the vertical Sync offset and Vertical sync pulse width using 6 bits of the EDID Table */
    TmpBuffer[0] = ((((HighBits&STHDMI_TIMING_BITS23_MASK)<<2)|(LowBits3&STHDMI_TIMING_HIGH_BITS_MASK)>>4)&0xF0)>>4;
    TmpBuffer[1] = ((((HighBits&STHDMI_TIMING_BITS23_MASK)<<2)|(LowBits3&STHDMI_TIMING_HIGH_BITS_MASK)>>4)&0x0F);
    TimingDesc_p->VSyncOffset = sthdmi_HextoDecConverter(TmpBuffer,2);

    TmpBuffer[0] = ((((HighBits&STHDMI_TIMING_BITS01_MASK)<<4)|(LowBits3&STHDMI_TIMING_LOW_BITS_MASK))&0xF0)>>4;
    TmpBuffer[1] = ((((HighBits&STHDMI_TIMING_BITS01_MASK)<<4)|(LowBits3&STHDMI_TIMING_LOW_BITS_MASK))&0x0F);
    TimingDesc_p->VSyncWidth = sthdmi_HextoDecConverter(TmpBuffer,2);

    /* Calculate the H &V Image Size */
    LowBits1 = (U8)(*Buffer_p);Buffer_p++;
    LowBits2 = (U8)(*Buffer_p);Buffer_p++;
    HighBits = (U8)(*Buffer_p);Buffer_p++;

    TmpBuffer[0] = (((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4|LowBits1)&0xF00)>>8;
    TmpBuffer[1] = (((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4|LowBits1)&0x0F0)>>4;
    TmpBuffer[2] = (((HighBits&STHDMI_TIMING_HIGH_BITS_MASK)<<4|LowBits1)&0x00F);
    TimingDesc_p->HImageSize = sthdmi_HextoDecConverter(TmpBuffer,3);

    TmpBuffer[0] = (((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8|LowBits2)&0xF00)>>8;
    TmpBuffer[1] = (((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8|LowBits2)&0x0F0)>>4;
    TmpBuffer[2] = (((HighBits&STHDMI_TIMING_LOW_BITS_MASK)<<8|LowBits2)&0x00F);

    TimingDesc_p->VImageSize = sthdmi_HextoDecConverter(TmpBuffer,3);

    /* Precise the Horizontal and Vertical Border Pixel*/
    TmpBuffer[0] = ((*Buffer_p)&0xF0)>>4;
    TmpBuffer[1] = ((*Buffer_p)&0x0F);Buffer_p++;
    TimingDesc_p->HBorder = sthdmi_HextoDecConverter(TmpBuffer,2);

    TmpBuffer[0] = ((*Buffer_p)&0xF0)>>4;
    TmpBuffer[1] = ((*Buffer_p)&0x0F);Buffer_p++;
    TimingDesc_p->VBorder = sthdmi_HextoDecConverter(TmpBuffer,2);

    /* Set miscellaneous Detailed Timing parameters*/
    MiscParams = (U8)(*Buffer_p);

    ((MiscParams&STHDMI_TIMING_DESC_FLAG_INTERLACED)==STHDMI_TIMING_DESC_FLAG_INTERLACED)?
         (TimingDesc_p->IsInterlaced=TRUE):(TimingDesc_p->IsInterlaced=FALSE);

     TmpBuffer1 = (U8)((MiscParams&STHDMI_FLAG_DECODE_STEREO_BIT56_MASK)>>4)|(MiscParams&STHDMI_FLAG_DECODE_STEREO_BIT0_MASK);

     if ((TmpBuffer1&STHDMI_TIMING_STEREO_RIGHT_IMAGE)==STHDMI_TIMING_STEREO_RIGHT_IMAGE)
     {
       TimingDesc_p->Stereo = STHDMI_EDID_STEREO_RIGHT_IMAGE;
     }
     else if ((TmpBuffer1&STHDMI_TIMING_STEREO_LEFT_IMAGE)==STHDMI_TIMING_STEREO_LEFT_IMAGE)
     {
       TimingDesc_p->Stereo = STHDMI_EDID_STEREO_LEFT_IMAGE;
     }
     else if ((TmpBuffer1&STHDMI_TIMING_STEREO_2W_RIGHT_IMAGE)==STHDMI_TIMING_STEREO_2W_RIGHT_IMAGE)
     {
       TimingDesc_p->Stereo = STHDMI_EDID_STEREO_2WAY_INTERLEAVED_RIGHT;
     }
     else if ((TmpBuffer1&STHDMI_TIMING_STEREO_2W_LEFT_IMAGE)==STHDMI_TIMING_STEREO_2W_LEFT_IMAGE)
     {
       TimingDesc_p->Stereo = STHDMI_EDID_STEREO_2WAY_INTERLEAVED_LEFT;
     }
     else if ((TmpBuffer1&STHDMI_TIMING_STEREO_4W)==STHDMI_TIMING_STEREO_4W)
     {
       TimingDesc_p->Stereo = STHDMI_EDID_STEREO_4WAY_INTERLEAVED;
     }
     else if ((TmpBuffer1&STHDMI_TIMING_STEREO_SIDE_BY_SIDE)==STHDMI_TIMING_STEREO_SIDE_BY_SIDE)
     {
       TimingDesc_p->Stereo = STHDMI_EDID_STEREO_SBS_INTERLEAVED;
     }
     else
     {
       TimingDesc_p->Stereo = STHDMI_EDID_NO_STEREO;
     }

     /* Fill signal sync description */
     if ((MiscParams&STHDMI_TIMING_DESC_FLAG_DIG_SEP)==STHDMI_TIMING_DESC_FLAG_DIG_SEP)
     {
       TimingDesc_p->SyncDescription = STHDMI_EDID_SYNC_DIGITAL_SEPARATE;
     }
     else if ((MiscParams&STHDMI_TIMING_DESC_FLAG_BIP_ANA)==STHDMI_TIMING_DESC_FLAG_BIP_ANA)
     {
       TimingDesc_p->SyncDescription = STHDMI_EDID_SYNC_BIP_ANALOG_COMPOSITE;
     }
     else if ((MiscParams&STHDMI_TIMING_DESC_FLAG_DIG_COMP)==STHDMI_TIMING_DESC_FLAG_DIG_COMP)
     {
       TimingDesc_p->SyncDescription = STHDMI_EDID_SYNC_DIGITAL_COMPOSITE;
     }
     else
     {
       TimingDesc_p->SyncDescription = STHDMI_EDID_SYNC_ANALOG_COMPOSITE;
     }

     /* Fill Horizontal and Vertical polarity */
     ((MiscParams &STHDMI_FLAG_BIT2_MASK)==STHDMI_FLAG_BIT2_MASK)? (TimingDesc_p->IsHPolarityPositive=TRUE):
     (TimingDesc_p->IsHPolarityPositive=FALSE);

     ((MiscParams &STHDMI_FLAG_BIT1_MASK)==STHDMI_FLAG_BIT1_MASK)? (TimingDesc_p->IsVPolarityPositive=TRUE):
     (TimingDesc_p->IsVPolarityPositive=FALSE);

   return(ErrorCode);
} /* end of sthdmi_FillEDIDTimingDescriptor() */
/*******************************************************************************
Name        :   sthdmi_GetSinkParams
Description :   Fill EDID sink parameters
Parameters  :   Value Stored(U32), STHDMI_EDIDSinkParams_t(pointer)
Assumptions :
Limitations :
Returns     :   none
*******************************************************************************/
void  sthdmi_GetSinkParams ( U32 ValueStored, STHDMI_EDIDSinkParams_t* const SinkParams_p)
{
   char TmpBuffer[1];

  ((ValueStored&STHDMI_EDID_SINK_UNDERSCAN)==STHDMI_EDID_SINK_UNDERSCAN)?(SinkParams_p->IsUnderscanSink= TRUE):
  (SinkParams_p->IsUnderscanSink= FALSE);

  ((ValueStored&STHDMI_EDID_SINK_BASIC_AUDIO)==STHDMI_EDID_SINK_BASIC_AUDIO)?(SinkParams_p->IsBasicAudioSupported= TRUE):
  (SinkParams_p->IsBasicAudioSupported= FALSE);

  ((ValueStored&STHDMI_EDID_SINK_YCBCR444)==STHDMI_EDID_SINK_YCBCR444)?(SinkParams_p->IsYCbCr444= TRUE):
  (SinkParams_p->IsYCbCr444= FALSE);

   ((ValueStored&STHDMI_EDID_SINK_YCBCR422)==STHDMI_EDID_SINK_YCBCR422)?(SinkParams_p->IsYCbCr422= TRUE):
  (SinkParams_p->IsYCbCr422= FALSE);

   TmpBuffer[0] = ValueStored&STHDMI_EDID_SINK_NATIVE_MASK;
   SinkParams_p->NativeFormatNum = sthdmi_HextoDecConverter(TmpBuffer,1);

} /* end of sthdmi_GetSinkParams()*/

/*******************************************************************************
Name        :   sthdmi_FillShortVideoDescriptor
Description :   Fill Video Data block
Parameters  :   Unit_p, Buffer_p(pointer), Video Data Block (pointer)
Assumptions :
Limitations :
Returns     :   none
*******************************************************************************/
void  sthdmi_FillShortVideoDescriptor (sthdmi_Unit_t * Unit_p,U8* Buffer_p, STHDMI_VideoDataBlock_t * const VideoData_p)
{
   STHDMI_ShortVideoDescriptor_t *  CurrentVideoDesc_p;
   U32 ShortVideoByte=0;
   char TmpBuffer[2];
   int Index;

   /* Fill tag Code Video */
   ShortVideoByte = (U8)(*Buffer_p);Buffer_p++;
   VideoData_p->TagCode = ShortVideoByte&STHDMI_EDID_DATA_BLOCK_TAG_CODE;

   /* Total Number of Video Bytes */
   TmpBuffer[0] = ((ShortVideoByte&STHDMI_EDID_DATA_BLOCK_LENGTH)&0xF0)>>4;
   TmpBuffer[1] = (ShortVideoByte&STHDMI_EDID_DATA_BLOCK_LENGTH)&0x0F;
   VideoData_p->VideoLength = sthdmi_HextoDecConverter(TmpBuffer,2);

   /* CEA Short Video Descriptor */
   for (Index=0;Index< VideoData_p->VideoLength ;Index++ )
   {

      CurrentVideoDesc_p = (STHDMI_ShortVideoDescriptor_t*) memory_allocate(Unit_p->Device_p->CPUPartition_p, \
                                     sizeof(STHDMI_ShortVideoDescriptor_t));

      ShortVideoByte = (U8)(*Buffer_p);Buffer_p++;

     ((ShortVideoByte &STHDMI_CEA_SHORT_VIDEO_NATIVE)==STHDMI_CEA_SHORT_VIDEO_NATIVE)?(CurrentVideoDesc_p->IsNative=TRUE):
     (CurrentVideoDesc_p->IsNative=FALSE);

      TmpBuffer[0] = ((ShortVideoByte&STHDMI_CEA_SHORT_VIDEO_CODE_MASK)&0xF0)>>4;
      TmpBuffer[1] = ((ShortVideoByte&STHDMI_CEA_SHORT_VIDEO_CODE_MASK)&0x0F);
      CurrentVideoDesc_p->VideoIdCode = sthdmi_HextoDecConverter(TmpBuffer,2);

      /* Move to the next Video Descriptor */
      if (Index < VideoData_p->VideoLength)
      {
        CurrentVideoDesc_p->NextVideoDescriptor_p= VideoData_p->VideoDescriptor_p;
        VideoData_p->VideoDescriptor_p = CurrentVideoDesc_p;

      }
  }
  Unit_p->Device_p->VideoDescriptor_p = VideoData_p->VideoDescriptor_p;
  Unit_p->Device_p->VideoLength  =VideoData_p->VideoLength;

} /* end of sthdmi_FillShortVideoDescriptor()*/
/*******************************************************************************
Name        :   sthdmi_FillShortAudioDescriptor
Description :   Fill Audio data Block.
Parameters  :   Unit_p, Buffer_p(pointer), Audio Data (pointer)
Assumptions :
Limitations :
Returns     :   none
*******************************************************************************/
void  sthdmi_FillShortAudioDescriptor (sthdmi_Unit_t * Unit_p, U8* Buffer_p, STHDMI_AudioDataBlock_t* const AudioData_p)
{
   STHDMI_ShortAudioDescriptor_t*  CurrentAudioDesc_p;
   U32 ShortAudioByte=0;
   U8 AudioCode=0;
   char TmpBuffer[2];
   int Index;

   /* Fill Audio Tag Code */
   ShortAudioByte = (U8)(*Buffer_p);Buffer_p++;
   AudioData_p->TagCode  = ShortAudioByte&STHDMI_EDID_DATA_BLOCK_TAG_CODE;

    /* Total Number of Audio data Block */
   TmpBuffer[0] = ((ShortAudioByte&STHDMI_EDID_DATA_BLOCK_LENGTH)&0xF0)>>4;
   TmpBuffer[1] = (ShortAudioByte&STHDMI_EDID_DATA_BLOCK_LENGTH)&0x0F;
   AudioData_p->AudioLength = sthdmi_HextoDecConverter(TmpBuffer,2);

   for (Index=0;Index<AudioData_p->AudioLength ;Index+=3)
   {
        ShortAudioByte = (U8)(*Buffer_p);Buffer_p++;
        AudioCode = ShortAudioByte&STHDMI_AUDIO_FORMAT_CODE_MASK;

        CurrentAudioDesc_p = (STHDMI_ShortAudioDescriptor_t*) memory_allocate(Unit_p->Device_p->CPUPartition_p,\
                                     sizeof(STHDMI_ShortAudioDescriptor_t));

        if ((AudioCode&STHDMI_AUDIO_FORMAT_CODE_PCM)==STHDMI_AUDIO_FORMAT_CODE_PCM)
        {
            CurrentAudioDesc_p->AudioFormatCode = STAUD_STREAM_CONTENT_LPCM;
        }
        else if ((AudioCode&STHDMI_AUDIO_FORMAT_CODE_AC3)==STHDMI_AUDIO_FORMAT_CODE_AC3)
        {
            CurrentAudioDesc_p->AudioFormatCode = STAUD_STREAM_CONTENT_AC3;
        }
        else if ((AudioCode&STHDMI_AUDIO_FORMAT_CODE_MPEG1)==STHDMI_AUDIO_FORMAT_CODE_MPEG1)
        {
            CurrentAudioDesc_p->AudioFormatCode = STAUD_STREAM_CONTENT_MPEG1;
        }
        else if ((AudioCode&STHDMI_AUDIO_FORMAT_CODE_MP3)==STHDMI_AUDIO_FORMAT_CODE_MP3)
        {
            CurrentAudioDesc_p->AudioFormatCode = STAUD_STREAM_CONTENT_MP3;
        }
        else if ((AudioCode&STHDMI_AUDIO_FORMAT_CODE_MPEG2)==STHDMI_AUDIO_FORMAT_CODE_MPEG2)
        {
            CurrentAudioDesc_p->AudioFormatCode = STAUD_STREAM_CONTENT_MPEG2;
        }
        else /*if ((ShortAudioByte&STHDMI_AUDIO_FORMAT_CODE_DTS)==STHDMI_AUDIO_FORMAT_CODE_DTS)*/
        {
            CurrentAudioDesc_p->AudioFormatCode = STAUD_STREAM_CONTENT_DTS;
        }

        TmpBuffer[0] = ShortAudioByte&STHDMI_AUDIO_MAX_NUM_CHANNEL_MASK;
        CurrentAudioDesc_p->NumofAudioChannels = sthdmi_HextoDecConverter(TmpBuffer,1)+1;

        ShortAudioByte = (U8)(*Buffer_p);Buffer_p++;
        CurrentAudioDesc_p->AudioSupported  = ShortAudioByte;

        ShortAudioByte = (U8)(*Buffer_p);Buffer_p++;
        CurrentAudioDesc_p->MaxBitRate = ShortAudioByte;

        /* Move to the next CEA Short Audio Descriptor */
        if (Index < AudioData_p->AudioLength)
        {
          CurrentAudioDesc_p->NextAudioDescriptor_p = AudioData_p->AudioDescriptor_p;
          AudioData_p->AudioDescriptor_p = CurrentAudioDesc_p;
        }
    }
   Unit_p->Device_p->AudioDescriptor_p = AudioData_p->AudioDescriptor_p;
   Unit_p->Device_p->AudioLength = AudioData_p->AudioLength;
} /* end of sthdmi_FillShortAudioDescriptor() */

/*******************************************************************************
Name        :   sthdmi_FillSpeakerAllocation
Description :   Fill Speaker allocation data block Payload
Parameters  :   Buffer_p(pointer), SpeakerAllocation  (pointer)
Assumptions :
Limitations :
Returns     :   none
*******************************************************************************/
void  sthdmi_FillSpeakerAllocation (sthdmi_Unit_t * Unit_p,U8* Buffer_p, STHDMI_SpeakerAllocation_t* const SpeakerAllocation_p)
{
    U32 SpeakerAllocation, Index;
    char TmpBuffer[2];

   UNUSED_PARAMETER (Unit_p);
   /* Fill Audio Tag Code */
   SpeakerAllocation = (U8)(*Buffer_p);Buffer_p++;
   SpeakerAllocation_p->TagCode  = SpeakerAllocation&STHDMI_EDID_DATA_BLOCK_TAG_CODE;

    /* Total Number of Audio data Block */
   TmpBuffer[0] = ((SpeakerAllocation&STHDMI_EDID_DATA_BLOCK_LENGTH)&0xF0)>>4;
   TmpBuffer[1] = (SpeakerAllocation&STHDMI_EDID_DATA_BLOCK_LENGTH)&0x0F;
   SpeakerAllocation_p->SpeakerLength = sthdmi_HextoDecConverter(TmpBuffer,2);

   /* Fill Speaker Alloctaion Data Block */
   for (Index=0;Index<SpeakerAllocation_p->SpeakerLength; Index++)
   {
     SpeakerAllocation = (U8)(*Buffer_p);Buffer_p++;
     SpeakerAllocation_p->SpeakerData[Index]= SpeakerAllocation;
   }

} /* end of sthdmi_FillSpeakerAllocation()*/

/*******************************************************************************
Name        :   sthdmi_FillVendorSpecific
Description :   Fill Vendor Specific data Block
Parameters  :   Buffer_p(pointer), Vendor Specific Data Block(pointer)
Assumptions :
Limitations :
Returns     :   none
*******************************************************************************/
void  sthdmi_FillVendorSpecific (sthdmi_Unit_t * Unit_p, U8* Buffer_p, STHDMI_VendorDataBlock_t* const VendorDataBlock_p)
{
    U8* CurrentVendorData_p;
    U32 VendorByte;
    char TmpBuffer[2];
    U8 TmpEdid,Index=0;
    STHDMI_CEC_PhysicalAddress_t PhysicalAddress;

    /* Fill Audio Tag Code */
   VendorByte = (U8)(*Buffer_p);Buffer_p++;
   VendorDataBlock_p->TagCode  = VendorByte&STHDMI_EDID_DATA_BLOCK_TAG_CODE;

    /* Total Number of Audio data Block */
   TmpBuffer[0] = ((VendorByte&STHDMI_EDID_DATA_BLOCK_LENGTH)&0xF0)>>4;
   TmpBuffer[1] = (VendorByte&STHDMI_EDID_DATA_BLOCK_LENGTH)&0x0F;
   VendorDataBlock_p->VendorLength = sthdmi_HextoDecConverter(TmpBuffer,2);

   VendorByte = (U8)(*Buffer_p);Buffer_p++;
   VendorDataBlock_p->RegistrationId[0] =VendorByte;

   VendorByte = (U8)(*Buffer_p);Buffer_p++;
   VendorDataBlock_p->RegistrationId[1] =VendorByte;

   VendorByte = (U8)(*Buffer_p);Buffer_p++;
   VendorDataBlock_p->RegistrationId[2] =VendorByte;

    /* Vendor Specific Data Block */
        TmpEdid = (U8)(*Buffer_p );
        PhysicalAddress.A = (TmpEdid & 0xF0)>>4;
        PhysicalAddress.B = (TmpEdid & 0x0F);
        TmpEdid = (U8)(*(Buffer_p + 1));
        PhysicalAddress.C = (TmpEdid & 0xF0)>>4;
        PhysicalAddress.D = (TmpEdid & 0x0F);
#ifdef STHDMI_CEC
        if((PhysicalAddress.A != 0xF )&&(PhysicalAddress.B != 0xF )&&
           (PhysicalAddress.C != 0xF )&&(PhysicalAddress.D != 0xF ))
        {
            Unit_p->Device_p->CEC_Params.IsPhysicalAddressValid = TRUE;
        }
        Unit_p->Device_p->CEC_Params.PhysicalAddress = PhysicalAddress;
#endif
        VendorDataBlock_p->PhysicalAddress = PhysicalAddress;


   /* Fill the first vendor specific data block payload */
   CurrentVendorData_p = (U8*) memory_allocate(Unit_p->Device_p->CPUPartition_p, 32*sizeof(U8*));/* max is 32 */
   VendorDataBlock_p->VendorData_p= CurrentVendorData_p;

   Unit_p->Device_p->VendorData_p  = VendorDataBlock_p->VendorData_p;
   Unit_p->Device_p->VendorLength = VendorDataBlock_p->VendorLength;

   for (Index=0;(Index<(VendorDataBlock_p->VendorLength-3))&&(VendorDataBlock_p->VendorLength>3); Index++)
   {
    VendorByte = (U8)(*Buffer_p);Buffer_p++;
    (*CurrentVendorData_p)=VendorByte;
    CurrentVendorData_p++;
   }


} /* end of sthdmi_FillVendorSpecific() */
/*******************************************************************************
Name        :   sthdmi_GetNumberOfEdidTiming
Description :   Retrieve the number of Detailed Timing descriptor.
Parameters  :   Buffer_p(pointer), Start Detailed Timing Descriptor
Assumptions :
Limitations :
Returns     :   Number of Detailed Timing Descriptors.
*******************************************************************************/
U32 sthdmi_GetNumberOfEdidTiming(U8* Buffer_p, U32 StartDetailedTiming)
{
    BOOL  TimingExtFound = FALSE;
    U32 NumberofTimingExt=0;
    U32 NumberOfBytes = 0;
    U32 Index;

   while ((StartDetailedTiming + NumberOfBytes)<127)
   {
        for (Index=0;(Index<18)&&(Buffer_p!=NULL)&&(StartDetailedTiming + NumberOfBytes)<127;Index++)
        {
            if ((U8)(*Buffer_p)!=0)
            {
            TimingExtFound =TRUE;
            }
            NumberOfBytes++;
            Buffer_p++;
        }
        if ((TimingExtFound)&&(Index==18))
        {
            NumberofTimingExt++;
            TimingExtFound =FALSE;
        }
   }
   return(NumberofTimingExt);
} /* end of sthdmi_GetNumberOfEdidTiming() */
/*******************************************************************************
Name        :  sthdmi_FreeEDIDMemory
Description :  Free Edid Block Extension already allocated
Parameters  :  Unit_p
Assumptions :
Limitations :
Returns     :  ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  sthdmi_FreeEDIDMemory (sthdmi_Unit_t * Unit_p)
{
    U32 Index=0;
    U32 TmpEdid=0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	STHDMI_EDIDBlockExtension_t* CurrentEdidExt_p;
    STHDMI_ShortVideoDescriptor_t *  CurrentVideoDesc_p;
    STHDMI_ShortAudioDescriptor_t*  CurrentAudioDesc_p;

    if (IsEDIDMemoryAllocated)
    {

        /* Free memory reserved for Video Data block already allocated */
        for ( Index=0; Index < Unit_p->Device_p->VideoLength; Index++)
        {
            CurrentVideoDesc_p =  Unit_p->Device_p->VideoDescriptor_p;
            Unit_p->Device_p->VideoDescriptor_p = Unit_p->Device_p->VideoDescriptor_p->NextVideoDescriptor_p;
            STOS_MemoryDeallocate(Unit_p->Device_p->CPUPartition_p,CurrentVideoDesc_p);

        }
        /* Free memory reserved for Audio data block already allocated */

        for (Index=0; Index <Unit_p->Device_p->AudioLength; Index+=3)
        {
            CurrentAudioDesc_p =  Unit_p->Device_p->AudioDescriptor_p;
            Unit_p->Device_p->AudioDescriptor_p = Unit_p->Device_p->AudioDescriptor_p->NextAudioDescriptor_p ;
            STOS_MemoryDeallocate(Unit_p->Device_p->CPUPartition_p,CurrentAudioDesc_p);

        }
        /* Free memory reserved for Vendor specific already allocated*/
        if(Unit_p->Device_p->VendorData_p != NULL)
        {
            STOS_MemoryDeallocate(Unit_p->Device_p->CPUPartition_p, Unit_p->Device_p->VendorData_p);
        }

        for ( Index =0;Index <Unit_p->Device_p->EDIDExtensionFlag; Index ++)
        {
            CurrentEdidExt_p =  Unit_p->Device_p->EDIDExtension_p;

            /* Free Detailed Timing descriptor memory already allocated*/
            if ((CurrentEdidExt_p->TimingData.EDID861.Tag==STHDMI_EDID_EXT_TAG_ADD_TIMING)||
                (CurrentEdidExt_p->TimingData.EDID861A.Tag==STHDMI_EDID_EXT_TAG_ADD_TIMING)||
                (CurrentEdidExt_p->TimingData.EDID861B.Tag==STHDMI_EDID_EXT_TAG_ADD_TIMING))
            {
            if (CurrentEdidExt_p->TimingData.EDID861.RevisionNumber== STHDMI_EDIDTIMING_EXT_TYPE_VER_ONE)
            {
                STOS_MemoryDeallocate(Unit_p->Device_p->CPUPartition_p, CurrentEdidExt_p->TimingData.EDID861.TimingDesc_p);
            }
            else if (CurrentEdidExt_p->TimingData.EDID861A.RevisionNumber== STHDMI_EDIDTIMING_EXT_TYPE_VER_TWO)
            {
                STOS_MemoryDeallocate(Unit_p->Device_p->CPUPartition_p, CurrentEdidExt_p->TimingData.EDID861A.TimingDesc_p);
            }
            else if(CurrentEdidExt_p->TimingData.EDID861B.RevisionNumber== STHDMI_EDIDTIMING_EXT_TYPE_VER_THREE )
            {
                STOS_MemoryDeallocate(Unit_p->Device_p->CPUPartition_p, CurrentEdidExt_p->TimingData.EDID861B.TimingDesc_p);
            }
            else
            {
                continue;
            }
            }
            /* Free Monitor block whether it 's already allocated */
            TmpEdid = CurrentEdidExt_p->MonitorBlock.DataTag;

            if (((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG1)== STHDMI_MONITOR_DESCRIPTOR_TAG1)||
                    ((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG2)== STHDMI_MONITOR_DESCRIPTOR_TAG2)||
                    ((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG4)== STHDMI_MONITOR_DESCRIPTOR_TAG4))
            {
                STOS_MemoryDeallocate (Unit_p->Device_p->CPUPartition_p,Unit_p->Device_p->MonitorData_p);
            }
        /* Free memory of the extension block already allocated */
            Unit_p->Device_p->EDIDExtension_p = Unit_p->Device_p->EDIDExtension_p->NextBlockExt_p;
            STOS_MemoryDeallocate(Unit_p->Device_p->CPUPartition_p,CurrentEdidExt_p);
        }
        IsEDIDMemoryAllocated =FALSE;
        Unit_p->Device_p->VideoLength=0;
        Unit_p->Device_p->AudioLength=0;
        Unit_p->Device_p->VendorLength=0;
        Unit_p->Device_p->EDIDExtensionFlag=0;
        memset(&Unit_p->Device_p->EDIDProductDesc,0, sizeof(STHDMI_EDIDProdDescription_t));
   }
   else
   {
      ErrorCode = ST_ERROR_NO_MEMORY;
      STTBX_Print((" EDID Memory Not Allocated!!"));
   }
   return(ErrorCode);
} /* End of sthdmi_FreeEDIDMemory() */

/*******************************************************************************
Name        :  sthdmi_InitEDIDStruct
Description :  Initialize EDID structures
Parameters  :  Unit_p
Assumptions :
Limitations :
Returns     :  ST_ErrorCode_t
*******************************************************************************/
static void sthdmi_InitEDIDStruct(sthdmi_Unit_t * Unit_p)
{
     Unit_p->Device_p->VideoLength       = 0;
     Unit_p->Device_p->AudioLength       = 0;
     Unit_p->Device_p->VendorLength      = 0;
     Unit_p->Device_p->EDIDExtensionFlag = 0;
     Unit_p->Device_p->VideoDescriptor_p = NULL;
     Unit_p->Device_p->AudioDescriptor_p = NULL;
     Unit_p->Device_p->EDIDExtension_p = NULL;
     Unit_p->Device_p->VendorData_p = NULL;
}
/*******************************************************************************
Name        :  sthdmi_FillSinkEDID
Description :  Fill Extended Display Identification Data(EDID) structure ver1.X
Parameters  :  Unit_p, EDIDStruct_p (EDID structure pointer)
Assumptions :
Limitations :
Returns     :  ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_FillSinkEDID(sthdmi_Unit_t * Unit_p, STHDMI_EDIDSink_t * const EDIDStruct_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVOUT_TargetInformation_t  TargetInfo;
    U32 Index = 0;U8 RGLowBits=0;U8 BWLowBits=0;
    U8 HighBits=0;U32 TmpEdid=0;U32 Length;U32 NumberOfTiming=0;
    U8* EDIDBuffer_p; char TmpBuffer[2];U32 ExtBlockLoop=0;U32 EdidExt;
    STHDMI_EDIDTimingDescriptor_t*  EdidTiming_p; U32 Pos=0;
    STHDMI_EDIDBlockExtension_t* CurrentEdidExt_p;
    U32 LengthTotal = 4;U32 TmpEdid1=0;


    /* Initializing EDID structures*/
    sthdmi_InitEDIDStruct(Unit_p);
    /* Get target information... */
    ErrorCode = STVOUT_GetTargetInformation(Unit_p->Device_p->VoutHandle, &TargetInfo);

    if (ErrorCode!= ST_NO_ERROR)
    {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_GetTargetInformation() failed. Error = 0x%x !", ErrorCode));
                 return(ErrorCode);
    }
    /* Retrieve EDID information */

    EDIDBuffer_p = TargetInfo.SinkInfo.Buffer_p;

    /* Fill Header EDID structure.. */
    for (Index=0;Index<8 ;Index++)
    {
        EDIDStruct_p->EDIDBasic.EDIDHeader[Index]= (U8)*EDIDBuffer_p;
        EDIDBuffer_p++;
    }

    /* Fill Vendor/product identification */

    /* Fill ID Manufacter Name... */
    TmpEdid =(U8)(*EDIDBuffer_p)<<8;
    EDIDBuffer_p++;
    TmpEdid |= (U8)(*EDIDBuffer_p);
    EDIDBuffer_p++;

    ErrorCode = sthdmi_GetIdManufacterName(TmpEdid, EDIDStruct_p->EDIDBasic.EDIDProdDescription.IDManufactureName);
#if defined(ST_OSLINUX)
    printk("sthdmi_FillSinkEDID : IDManufactureName is : %s \n\r" , EDIDStruct_p->EDIDBasic.EDIDProdDescription.IDManufactureName );
#endif /* ST_OSLINUX */
    if (ErrorCode!= ST_NO_ERROR)
    {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Can not Retrieve the Id Manufacter Name. Error = 0x%x !", ErrorCode));
         /*memory_deallocate(Unit_p->Device_p->CPUPartition_p,TargetInfo.SinkInfo.Buffer_p);*/
         return(ErrorCode);
    }
    #if 0
    for (Index=0;Index<3 ;Index++)
    {
        printf("the value of character is %c\n",EDIDStruct_p->EDIDBasic.EDIDProdDescription.IDManufactureName[Index]);
    }
    printf("\n");
    #endif


    /* Fill ID product Code .. */
    TmpEdid = (U8)(*EDIDBuffer_p)<<8;
    EDIDBuffer_p++;
    TmpEdid |= (U8)(*EDIDBuffer_p);
    EDIDBuffer_p++;
    EDIDStruct_p->EDIDBasic.EDIDProdDescription.IDProductCode = (U32)TmpEdid ;

    /* Fill ID Serial Number... */
    TmpEdid = (U8)(*EDIDBuffer_p)<<24; EDIDBuffer_p++;
    TmpEdid |= (U8)(*EDIDBuffer_p)<<16; EDIDBuffer_p++;
    TmpEdid |= (U8)(*EDIDBuffer_p)<<8; EDIDBuffer_p++;
    TmpEdid |= (U8)(*EDIDBuffer_p); EDIDBuffer_p++;
    EDIDStruct_p->EDIDBasic.EDIDProdDescription.IDSerialNumber =(U32)sthdmi_GetIDSerialNumber(TmpEdid);
#if defined(ST_OSLINUX)
    printk("sthdmi_FillSinkEDID : IDSerialNumber  is : %d \n\r" , EDIDStruct_p->EDIDBasic.EDIDProdDescription.IDSerialNumber );
#endif /* ST_OSLINUX */

    /* Fill Week of manufacter... */
    EDIDStruct_p->EDIDBasic.EDIDProdDescription.WeekOfManufacture = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
#if defined(ST_OSLINUX)
    printk("sthdmi_FillSinkEDID : WeekOfManufacture  is : %d \n\r" , (U8)(*EDIDBuffer_p));
#endif /* ST_OSLINUX */

    /* Fill Year of manufacter ... */
    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
    EDIDStruct_p->EDIDBasic.EDIDProdDescription.YearOfManufacture = sthdmi_GetYearOfManufacter(TmpEdid);
#if defined(ST_OSLINUX)
    printk("sthdmi_FillSinkEDID : YearOfManufacture  is : %d \n\r" , EDIDStruct_p->EDIDBasic.EDIDProdDescription.YearOfManufacture);
#endif /* ST_OSLINUX */
   /* Fill EDID verion/revision */
   EDIDStruct_p->EDIDBasic.EDIDInfos.Version= (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDInfos.Revision =(U8)(*EDIDBuffer_p);EDIDBuffer_p++;

    /* Copy information into structure used for audio WA on 7710...*/
    memcpy((void*)&Unit_p->Device_p->IDManufactureName,(void*)&EDIDStruct_p->EDIDBasic.EDIDProdDescription.IDManufactureName,sizeof(char)*3);

   /* Fill Basic Display parameters */

   /* Fill Video Input Definition */
   TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   sthdmi_GetVideoInputDefinition(TmpEdid, &EDIDStruct_p->EDIDBasic.EDIDDisplayParams.VideoInput);
#if defined(ST_OSLINUX)
   printk("sthdmi_FillSinkEDID -> VideoInput : SignalLevelMax  is : %d \n\r" , EDIDStruct_p->EDIDBasic.EDIDDisplayParams.VideoInput.SignalLevelMax);
   printk("sthdmi_FillSinkEDID -> VideoInput : SignalLevelMin  is : %d \n\r" , EDIDStruct_p->EDIDBasic.EDIDDisplayParams.VideoInput.SignalLevelMin);
#endif /* ST_OSLINUX */

   /* Fill Maximum Image size */
   TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDDisplayParams.MaxHorImageSize = sthdmi_GetDisplayParams(TmpEdid);

   TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDDisplayParams.MaxVerImageSize = sthdmi_GetDisplayParams(TmpEdid);

   /* The display gamma was multiplied by 100 */
   TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDDisplayParams.DisplayGamma  = sthdmi_GetDisplayGamma(TmpEdid);

   /* Fill Feature Support */
   TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   sthdmi_GetVideoFeatureSupported (TmpEdid, &EDIDStruct_p->EDIDBasic.EDIDDisplayParams.FeatureSupport);

   /* Fill Color Characteristic */
   RGLowBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   BWLowBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

   /* Fill Chroma info, Red X *1000 */
   HighBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDColorParams.Red_x  =sthdmi_GetColorParams(RGLowBits,HighBits, STHDMI_RED_X_COLOR);
   /* Fill Chroma info, Red Y *1000 */

   HighBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDColorParams.Red_y  =sthdmi_GetColorParams(RGLowBits,HighBits, STHDMI_RED_Y_COLOR);
   /* Fill Chroma info, Green X *1000 */

   HighBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDColorParams.Green_x  =sthdmi_GetColorParams(RGLowBits,HighBits, STHDMI_GREEN_X_COLOR);
   /* Fill Chroma info, Green Y *1000 */

   HighBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDColorParams.Green_y  =sthdmi_GetColorParams(RGLowBits,HighBits, STHDMI_GREEN_Y_COLOR);
   /* Fill Chroma info, Blue X *1000 */

   HighBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDColorParams.Blue_x  =sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_BLUE_X_COLOR);
   /* Fill Chroma info, Blue Y *1000 */

   HighBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDColorParams.Blue_y  =sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_BLUE_Y_COLOR);
   /* Fill Chroma info, White X *1000 */

   HighBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDColorParams.White_x  =sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_WHITE_X_COLOR);

   /* Fill Chroma info, White Y *1000 */
   HighBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
   EDIDStruct_p->EDIDBasic.EDIDColorParams.White_y  =sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_WHITE_Y_COLOR);

  /* Fill Established TimingI ... */
  TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
  EDIDStruct_p->EDIDBasic.EDIDEstablishedTiming.Timing1 = sthdmi_GetEstablishedTimingI(TmpEdid);

  /* Fill Established TimingII ... */
  TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
  EDIDStruct_p->EDIDBasic.EDIDEstablishedTiming.Timing2 = sthdmi_GetEstablishedTimingII(TmpEdid);

  /* Fill Manufacturer Timing.. */
  TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
  EDIDStruct_p->EDIDBasic.EDIDEstablishedTiming.IsManTimingSupported = sthdmi_GetManufacturerTiming(TmpEdid);

  /* Fill Standard Timing Identification 1->8 */
  for (Index=0; Index<8 ;Index++)
  {
    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
    sthdmi_GetStandardTimingParams((U32)(*EDIDBuffer_p), TmpEdid, &EDIDStruct_p->EDIDBasic.EDIDStdTiming[Index]);EDIDBuffer_p++;
  }

  /* Fill Detailed Timing Decsription 1->4 */
  for (Index=0;Index<4 ;Index++)
  {
    ErrorCode = sthdmi_FillEDIDTimingDescriptor(EDIDBuffer_p, &EDIDStruct_p->EDIDBasic.EDIDDetailedDesc[Index]);
    /* Move the EDID pointer to the next detailed timing descriptor */
    for (Pos=0;Pos<18;Pos++,EDIDBuffer_p++);

    if (ErrorCode!= ST_NO_ERROR)
    {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_FillEDIDTimingDescriptor() failed. Error = 0x%x !", ErrorCode));
         /*memory_deallocate(Unit_p->Device_p->CPUPartition_p,TargetInfo.SinkInfo.Buffer_p);*/
         return(ErrorCode);
    }

  }
  /* Fill Extension Flag and checksum  */
  TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
  EDIDStruct_p->EDIDBasic.EDIDExtensionFlag = sthdmi_GetDecValue(TmpEdid);
  TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
  EDIDStruct_p->EDIDBasic.EDIDChecksum      = sthdmi_GetDecValue(TmpEdid);

   /* Fill Sink EDID Extension */
   for (ExtBlockLoop=0;(ExtBlockLoop <EDIDStruct_p->EDIDBasic.EDIDExtensionFlag); ExtBlockLoop++)
   {
        TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

        /* Allocate EDID extension data structure */
        CurrentEdidExt_p = (STHDMI_EDIDBlockExtension_t*) memory_allocate (Unit_p->Device_p->CPUPartition_p, \
                                        sizeof(STHDMI_EDIDBlockExtension_t));

        memset(CurrentEdidExt_p,0,sizeof(STHDMI_EDIDBlockExtension_t));

        /* Memory allocation management */

        IsEDIDMemoryAllocated =TRUE;

        if (TmpEdid == STHDMI_EDID_EXT_TAG_ADD_TIMING)
        {
            EdidExt = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

            switch ((STHDMI_EDIDTimingExtType_t)EdidExt)
            {
                case STHDMI_EDIDTIMING_EXT_TYPE_NONE:
                    break;

                case STHDMI_EDIDTIMING_EXT_TYPE_VER_ONE :
                    /* Fill Timing Extension Tag  */
                    CurrentEdidExt_p->TimingData.EDID861.Tag = STHDMI_EDID_EXT_TAG_ADD_TIMING;

                    /* Fill Timing Extension Revision */
                    CurrentEdidExt_p->TimingData.EDID861.RevisionNumber = EdidExt;

                    /* Fill Timing Extension offset where detailed data begins */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    CurrentEdidExt_p->TimingData.EDID861.Offset = TmpEdid;

                    /* If d=4, no data is provided in the reserved data block  */
                    /* If d=0, no data is provided in the reserved data block, and, no detailed timing descriptor are provided */
                    if (CurrentEdidExt_p->TimingData.EDID861A.Offset!=0)
                    {
                            /* Move the EDID pointer to the end of reserved data block */
                            for (Pos=4;Pos<CurrentEdidExt_p->TimingData.EDID861.Offset;Pos++,EDIDBuffer_p++);

                            /*Retrieve the Number of Detailed Timing Descriptor  */
                            NumberOfTiming= sthdmi_GetNumberOfEdidTiming(EDIDBuffer_p, \
                                                CurrentEdidExt_p->TimingData.EDID861.Offset);

                                EdidTiming_p = (STHDMI_EDIDTimingDescriptor_t*)memory_allocate(Unit_p->Device_p->CPUPartition_p,\
                                                                                            NumberOfTiming * sizeof(STHDMI_EDIDTimingDescriptor_t));
                            CurrentEdidExt_p->TimingData.EDID861.TimingDesc_p = EdidTiming_p;
                            CurrentEdidExt_p->TimingData.EDID861.NumberOfTiming = NumberOfTiming;

                            for (Index=CurrentEdidExt_p->TimingData.EDID861.Offset;
                                Index<(CurrentEdidExt_p->TimingData.EDID861.Offset+(18*NumberOfTiming));Index+=18)
                            {
                                ErrorCode = sthdmi_FillEDIDTimingDescriptor(EDIDBuffer_p,EdidTiming_p);

                                /* Move the EDID pointer to the next detailed timing descriptor */
                                for (Pos=0;(Pos<18)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++); EdidTiming_p++;

                                if (ErrorCode!= ST_NO_ERROR)
                                {
                                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_FillEDIDTimingDescriptor() failed. Error = 0x%x !", ErrorCode));
                                    /*memory_deallocate(Unit_p->Device_p->CPUPartition_p,TargetInfo.SinkInfo.Buffer_p);*/
                                    return(ErrorCode);
                                }
                            }
                            /* Skip the padding bytes and move the pointer to the checksum bytes */
                            for (Pos=0;(Pos<(127-(CurrentEdidExt_p->TimingData.EDID861.Offset+(18*NumberOfTiming))));Pos++)
                            {
                                EDIDBuffer_p++;
                            }
                    }
                    else
                    {
                        CurrentEdidExt_p->TimingData.EDID861.TimingDesc_p = NULL;
                        CurrentEdidExt_p->TimingData.EDID861.NumberOfTiming = 0;
                        EDIDBuffer_p+=124; /* Last was after Offset in pos 0,1,2,[3],4,.. -> Checksum in [127] */
                    }
                    TmpEdid = (U8)(*EDIDBuffer_p);
                    CurrentEdidExt_p->TimingData.EDID861.Checksum = TmpEdid;
                    EDIDBuffer_p++;


                    break;

                case STHDMI_EDIDTIMING_EXT_TYPE_VER_TWO:
                    CurrentEdidExt_p->TimingData.EDID861A.Tag = STHDMI_EDID_EXT_TAG_ADD_TIMING;

                    /* Fill Timing Extension Revision */
                    CurrentEdidExt_p->TimingData.EDID861A.RevisionNumber = EdidExt;

                    /* Fill Timing Extension offset where detailed data begins */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    CurrentEdidExt_p->TimingData.EDID861A.Offset = TmpEdid;

                    /* Byte #3 */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    sthdmi_GetSinkParams(TmpEdid, &CurrentEdidExt_p->TimingData.EDID861A.FormatDescription);


                    /* If d=4, no data is provided in the reserved data block  */
                    /* If d=0, no data is provided in the reserved data block, and, no detailed timing descriptor are provided */
                    if (CurrentEdidExt_p->TimingData.EDID861A.Offset!=0)
                    {
                            /* Move the EDID pointer to the end of reserved data block */
                            for (Pos=4;Pos<CurrentEdidExt_p->TimingData.EDID861A.Offset;Pos++,EDIDBuffer_p++);

                            /*Retrieve the Number of Detailed Timing Descriptor  */

                            NumberOfTiming= sthdmi_GetNumberOfEdidTiming(EDIDBuffer_p, \
                                                CurrentEdidExt_p->TimingData.EDID861A.Offset);
                            EdidTiming_p = (STHDMI_EDIDTimingDescriptor_t*)memory_allocate(Unit_p->Device_p->CPUPartition_p,\
                                                                                           NumberOfTiming * sizeof(STHDMI_EDIDTimingDescriptor_t));
                            CurrentEdidExt_p->TimingData.EDID861A.TimingDesc_p = EdidTiming_p;
                            CurrentEdidExt_p->TimingData.EDID861A.NumberOfTiming = NumberOfTiming;



                            for (Index=CurrentEdidExt_p->TimingData.EDID861A.Offset;
                                Index<(CurrentEdidExt_p->TimingData.EDID861A.Offset+(18*NumberOfTiming));Index+=18)
                            {
                                ErrorCode = sthdmi_FillEDIDTimingDescriptor(EDIDBuffer_p,EdidTiming_p);

                                /* Move the EDID pointer to the next detailed timing descriptor */
                                for (Pos=0;(Pos<18)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++); EdidTiming_p++;

                                if (ErrorCode!= ST_NO_ERROR)
                                {
                                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_FillEDIDTimingDescriptor() failed. Error = 0x%x !", ErrorCode));
                                    /*memory_deallocate(Unit_p->Device_p->CPUPartition_p,TargetInfo.SinkInfo.Buffer_p);*/
                                    return(ErrorCode);
                                }
                            }
                            /* Skip the padding bytes and move the pointer to the checksum bytes */
                            for (Pos=0;(Pos<(127-(CurrentEdidExt_p->TimingData.EDID861A.Offset+(18*NumberOfTiming))));Pos++)
                            {
                                EDIDBuffer_p++;
                            }
                    }
                    else
                    {
                        CurrentEdidExt_p->TimingData.EDID861A.TimingDesc_p = NULL;
                        CurrentEdidExt_p->TimingData.EDID861A.NumberOfTiming = 0;
                        EDIDBuffer_p+=123; /* Last was after FormatDescription in pos 0,1,2,3,[4],5,.. -> Checksum in [127] */
                    }
                    TmpEdid = (U8)(*EDIDBuffer_p);
                    CurrentEdidExt_p->TimingData.EDID861A.Checksum = TmpEdid;
                    EDIDBuffer_p++;

                    break;
                case STHDMI_EDIDTIMING_EXT_TYPE_VER_THREE:

                    /* Fill Timing Extension Tag */
                    CurrentEdidExt_p->TimingData.EDID861B.Tag = STHDMI_EDID_EXT_TAG_ADD_TIMING;

                    /* Fill Timing Extension Revision */
                    CurrentEdidExt_p->TimingData.EDID861B.RevisionNumber = EdidExt;

                    /* Fill Timing Extension offset where detailed data begins */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    CurrentEdidExt_p->TimingData.EDID861B.Offset = TmpEdid;

                    /* Byte #3 */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    sthdmi_GetSinkParams(TmpEdid, &CurrentEdidExt_p->TimingData.EDID861B.FormatDescription);

                    /* If no data is provided in the reserved data block, then d=4 */
                    /* If d=0, then no detailed timing descriptor are provided and
                    * no data is provided in the reserved data block  */

                    if (CurrentEdidExt_p->TimingData.EDID861B.Offset!=0)
                    {
                         /* Retrieve the Fourth data Block */
                         /*  Video Data Block, Audio data Block, Vendor Specific Data Block and Speaker Allocation Data Block*/

                        /* as LengthTotal = 4, this means (Offset != 4) -> DataBlock present */
                        while (LengthTotal  < (CurrentEdidExt_p->TimingData.EDID861B.Offset))
                        {
                            TmpEdid = (U8)((*EDIDBuffer_p)&STHDMI_EDID_DATA_BLOCK_TAG_CODE)>>5;

                            switch (TmpEdid)
                            {
                               case  1:
                                    /* Fill Audio Short Decsriptor */
                                    sthdmi_FillShortAudioDescriptor(Unit_p, EDIDBuffer_p, \
                                            &CurrentEdidExt_p->TimingData.EDID861B.DataBlock.AudioData);
                                    /* Move the EDID Pointer to the next Allocation Data Block */
                                    Length= CurrentEdidExt_p->TimingData.EDID861B.DataBlock.AudioData.AudioLength;

                                    for (Pos=0;(Pos<=Length)&&(EDIDBuffer_p!=NULL) ;Pos++,EDIDBuffer_p++);
                                    break;

                                case 2 :
                                    /* Fill Video Short Descriptor */
                                    sthdmi_FillShortVideoDescriptor(Unit_p, EDIDBuffer_p, \
                                            &CurrentEdidExt_p->TimingData.EDID861B.DataBlock.VideoData);

                                    /* Move the EDID Pointer to the next Data Block */
                                    Length= CurrentEdidExt_p->TimingData.EDID861B.DataBlock.VideoData.VideoLength;
                                    for (Pos=0;(Pos<=Length)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++);
                                    break;

                                case 3 :
                                    /* Fill Vendor Specific Data Block */
                                    sthdmi_FillVendorSpecific(Unit_p, EDIDBuffer_p,  \
                                            &CurrentEdidExt_p->TimingData.EDID861B.DataBlock.VendorData);
                                    /* Move the EDID Pointer to the next Data Block */
                                    Length= CurrentEdidExt_p->TimingData.EDID861B.DataBlock.VendorData.VendorLength;
                                    for (Pos=0;(Pos<=Length)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++);
                                    break;

								 case 4 :
                                    /*Fill Speaker Allocation data Block */
                                    sthdmi_FillSpeakerAllocation(Unit_p, EDIDBuffer_p,  \
                                            &CurrentEdidExt_p->TimingData.EDID861B.DataBlock.SpeakerData);


                                    /* Move the EDID Pointer to the Vendor Specfic Data Block */
                                    Length= CurrentEdidExt_p->TimingData.EDID861B.DataBlock.SpeakerData.SpeakerLength;
                                    for (Pos=0;(Pos<=Length)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++);
                                    break;

                                 default:
                                    /*Support of unknown CEA extension just skip it*/
                                    TmpEdid1 = (U8)(*EDIDBuffer_p);
                                    TmpBuffer[0] = ((TmpEdid1&STHDMI_EDID_DATA_BLOCK_LENGTH)&0xF0)>>4;
                                    TmpBuffer[1] = ((TmpEdid1&STHDMI_EDID_DATA_BLOCK_LENGTH)&0x0F);
                                    Length = sthdmi_HextoDecConverter(TmpBuffer,2);
                                    for (Pos=0;(Pos<=Length)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++);
                                    break;
                                }

                                 LengthTotal = LengthTotal + Length + 1;
							}
                            /*Retrieve the Number of Detailed Timing Descriptor  */

                            NumberOfTiming= sthdmi_GetNumberOfEdidTiming(EDIDBuffer_p, \
                                                CurrentEdidExt_p->TimingData.EDID861B.Offset);

                            EdidTiming_p = (STHDMI_EDIDTimingDescriptor_t*)memory_allocate(Unit_p->Device_p->CPUPartition_p,\
                                                                                           NumberOfTiming * sizeof(STHDMI_EDIDTimingDescriptor_t));
                            CurrentEdidExt_p->TimingData.EDID861B.TimingDesc_p = EdidTiming_p;
                            CurrentEdidExt_p->TimingData.EDID861B.NumberOfTiming = NumberOfTiming;


                            for (Index=CurrentEdidExt_p->TimingData.EDID861B.Offset;
                                Index<(CurrentEdidExt_p->TimingData.EDID861B.Offset+(18*NumberOfTiming));Index+=18)
                            {

                                ErrorCode = sthdmi_FillEDIDTimingDescriptor(EDIDBuffer_p,EdidTiming_p);

                                /* Move the EDID pointer to the next detailed timing descriptor */
                                for (Pos=0;(Pos<18)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++); EdidTiming_p++;

                                if (ErrorCode!= ST_NO_ERROR)
                                {
                                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_FillEDIDTimingDescriptor() failed. Error = 0x%x !", ErrorCode));
                                    /*memory_deallocate(Unit_p->Device_p->CPUPartition_p,TargetInfo.SinkInfo.Buffer_p);*/
                                    return(ErrorCode);
                                }
                            }
                            /* Skip the padding bytes and move the pointer to the checksum bytes */
                            for (Pos=0;(Pos<(127-(CurrentEdidExt_p->TimingData.EDID861B.Offset+(18*NumberOfTiming))));Pos++)
                            {
                                EDIDBuffer_p++;
                            }
                    }
                    else
                    {
                        CurrentEdidExt_p->TimingData.EDID861B.TimingDesc_p = NULL;
                        CurrentEdidExt_p->TimingData.EDID861B.NumberOfTiming = 0;
                        EDIDBuffer_p+=123; /* Last was after FormatDescription in pos 0,1,2,3,[4],5,.. -> Checksum in [127] */
                    }
                    TmpEdid = (U8)(*EDIDBuffer_p);
                    CurrentEdidExt_p->TimingData.EDID861B.Checksum = TmpEdid;
                    EDIDBuffer_p++;

                    break;
                default :
                    break;
            }

        }
        else if (TmpEdid == STHDMI_EDID_EXT_TAG_MONITOR_DESC)
        {
            /* Flag (2 bytes)=0000h when block used as descriptor, 1st byte already identified*/
            TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
            /* Flag : reserved =00h when block used as descriptor*/
            TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
            /* Data type Tag (binary coded) */
            CurrentEdidExt_p->MonitorBlock.DataTag = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
            /* Flag =00hwhen block used as descriptor */
            TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
            TmpEdid = (U8)CurrentEdidExt_p->MonitorBlock.DataTag;

            /* It deals with monitor serial number, or with ASCII string or with monitor name */
            if (((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG1)== STHDMI_MONITOR_DESCRIPTOR_TAG1)||
                 ((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG2)== STHDMI_MONITOR_DESCRIPTOR_TAG2)||
                 ((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG4)== STHDMI_MONITOR_DESCRIPTOR_TAG4))
            {
              /* Allocate memory for Monitor data descriptor */
              CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Data_p = memory_allocate(\
                    Unit_p->Device_p->CPUPartition_p, STHDMI_MAX_DESCRIPTOR_DATA_BYTE*sizeof(U8));

              /* Retrieve the Monitor data pointer*/
              Unit_p->Device_p->MonitorData_p = CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Data_p;

              CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Length=0;
              do
              {
                 (*CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Data_p) = (*EDIDBuffer_p);

                 EDIDBuffer_p++;
                 TmpEdid = (U8)(*CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Data_p);
                 CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Data_p++;
                 CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Length++;
		         Length = CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Length;

              }while((TmpEdid != STHDMI_EDID_DESCRIPTOR_DATA_END) && (Length < STHDMI_MAX_DESCRIPTOR_DATA_BYTE));

               TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
               Length  = CurrentEdidExt_p->MonitorBlock.DataDescriptor.MonitorData.Length;

               /* Skip padding field determined with ASCII code = 0x20*/
               if ((Length < 13)&&(TmpEdid==STHDMI_MONITOR_PAD_FIELD))
               {
                /* Skip padding field determined with ASCII code = 0x20*/
                do
                {
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    Length++;
                }while ((Length < 13)&&(TmpEdid==STHDMI_MONITOR_PAD_FIELD));
               }
            }
            else if ((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG3)==STHDMI_MONITOR_DESCRIPTOR_TAG3)
            {
                /* Retrieve min vertical rate in Hz*/
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.MinVertRate =sthdmi_GetDecValue (TmpEdid);

                /* Retrieve max vertical rate in Hz*/
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.MaxVertRate =sthdmi_GetDecValue (TmpEdid);

                /* Retrieve min Horizontal rate in kHz */
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.MinHorRate =sthdmi_GetDecValue (TmpEdid);

                /* Retrieve max Horizontal rate in kHz */
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.MaxHorRate =sthdmi_GetDecValue (TmpEdid);

                /* Retrieve Max Supported pixel Clock, binary coded Clock rate in MHz/10 */
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                TmpBuffer[0]= (TmpEdid&0xF0)>>4;
                TmpBuffer[1]= (TmpEdid&0x0F);
                CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.MaxPixelClock =  \
                        sthdmi_HextoDecConverter(TmpBuffer,2)*10;

               /* Check Whether Secondary Timing Formula was supported? */
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

                if ((TmpEdid&STHDMI_SECONDRY_TIMING_SUPPORTED)==STHDMI_SECONDRY_TIMING_SUPPORTED)
                {
                    /* Secondrary GTF  was supported */
                    CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.IsSecondGTFSupported =TRUE;

                    /* Reserved Set 00h, just skip it */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

                    /* Get Start Frequency for secondrary curve in kHz*/
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.StartFrequency = \
                        sthdmi_GetGTFStartFrequency(TmpEdid);

                   /* Get standard Generalized Timing Formula Parameters */
                    /* C parameter, multiplied by 2 */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.GTFParams.C = \
                        sthdmi_GetGTFParams(TmpEdid);

                    /* M parameter ,Concatenate the Value that will be converted */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    HighBits =(U8)(*EDIDBuffer_p)<<8;EDIDBuffer_p++;
                    TmpEdid |= HighBits;
                    CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.GTFParams.M = \
                        sthdmi_GetGTFMParams(TmpEdid);

                    /* K parameter */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.GTFParams.K = \
                    sthdmi_GetDecValue(TmpEdid);

                    /* J parameter */
                    TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.GTFParams.J = \
                        sthdmi_GetGTFParams(TmpEdid);
                 }
                 else
                 {
                    /* No secondrary Timing Formula supported */
                    CurrentEdidExt_p->MonitorBlock.DataDescriptor.RangeLimit.IsSecondGTFSupported =FALSE;

                    /* byte 11 should be set to 0x0A, just skip it..*/
                    TmpEdid = (U8)(*EDIDBuffer_p);

                    if (TmpEdid == STHDMI_EDID_DESCRIPTOR_DATA_END)
                    {
                        EDIDBuffer_p++;
                    }
                    else
                    {
                        ErrorCode=ST_ERROR_BAD_PARAMETER;
                    }

                    /* bytes 12-17 are padding fields, just skip them...  */
                    for (Index=0;Index<5 ;Index++)
                    {
                        /* it deals with padding field */
                        TmpEdid = (U8)(*EDIDBuffer_p);
                        if (TmpEdid == STHDMI_MONITOR_PAD_FIELD)
                        {
                            EDIDBuffer_p++;
                        }
                        else
                        {
                           ErrorCode=ST_ERROR_BAD_PARAMETER;
                        }
                    }
                 }
            }
            else if ((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG5)==STHDMI_MONITOR_DESCRIPTOR_TAG5)
            {
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                /* An index number of 00h indicates that no color point data follows */
                if (TmpEdid!=STHDMI_NO_COLOR_POINT)
                {
                    BWLowBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    HighBits =  (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

                   /* Retrieve White X value *1000 */
                   CurrentEdidExt_p->MonitorBlock.DataDescriptor.ColorPoint.White_x[0]= \
                    sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_WHITE_X_COLOR);

                   HighBits =  (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

                   /* Retrieve White X value *1000 */
                   CurrentEdidExt_p->MonitorBlock.DataDescriptor.ColorPoint.White_y[0]= \
                    sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_WHITE_Y_COLOR);

                   /* Retrieve White Gamma *1000*/
                   HighBits =  (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                   CurrentEdidExt_p->MonitorBlock.DataDescriptor.ColorPoint.WhiteGamma[0]= \
                    sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_BLUE_Y_COLOR);


                }
                else
                {
                    /* just skip 3 bytes */
                   for (Pos=0;(Pos<3)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++);
                }

                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                /* Go to the next white point, An index number of 00h indicates that no color point data follows */
                if (TmpEdid!=STHDMI_NO_COLOR_POINT)
                {
                    BWLowBits = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                    HighBits =  (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

                   /* Retrieve White X value *1000 */
                   CurrentEdidExt_p->MonitorBlock.DataDescriptor.ColorPoint.White_x[1]= \
                    sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_WHITE_X_COLOR);

                   HighBits =  (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

                   /* Retrieve White X value *1000 */
                   CurrentEdidExt_p->MonitorBlock.DataDescriptor.ColorPoint.White_y[1]= \
                    sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_WHITE_Y_COLOR);

                   /* Retrieve White Gamma *1000*/
                   HighBits =  (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                   CurrentEdidExt_p->MonitorBlock.DataDescriptor.ColorPoint.WhiteGamma[1]= \
                    sthdmi_GetColorParams(BWLowBits,HighBits, STHDMI_BLUE_Y_COLOR);
                }
                else
                {
                    /* just skip 3 bytes */
                   for (Pos=0;(Pos<3)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++);
                }

            }
            else if ((TmpEdid&STHDMI_MONITOR_DESCRIPTOR_TAG6)==STHDMI_MONITOR_DESCRIPTOR_TAG6)
            {
                /* retrieve the standard Timing identification 9->14 */
                for (Index=0; Index<6 ;Index++)
                {
                     TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                     sthdmi_GetStandardTimingParams((U32)(*EDIDBuffer_p), TmpEdid, \
                         &CurrentEdidExt_p->MonitorBlock.DataDescriptor.TmingId.StdTiming[Index]);EDIDBuffer_p++;
                }

                /* End of Data descriptor */
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;

                if (TmpEdid == STHDMI_EDID_DESCRIPTOR_DATA_END)
                {
                     EDIDBuffer_p++;
                }
                else
                {
                    ErrorCode=ST_ERROR_BAD_PARAMETER;
                }

            }
            else
            {
                /* Unknown forma of data descriptor, just skip the 13 bytes */
                for (Pos=0;(Pos<13)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++);
            }

        }
        else if (TmpEdid == STHDMI_EDID_EXT_TAG_BLOCK_MAP)
        {
            CurrentEdidExt_p->MapBlock.Tag = STHDMI_EDID_EXT_TAG_BLOCK_MAP;

            for (Index=0;Index<126;Index++)
            {
                TmpEdid = (U8)(*EDIDBuffer_p);EDIDBuffer_p++;
                CurrentEdidExt_p->MapBlock.ExtensionTag[Index]=TmpEdid;
            }
            CurrentEdidExt_p->MapBlock.CheckSum =(*EDIDBuffer_p);EDIDBuffer_p++;
        }
        else
        {
            /* Unknown block, just skip the 128 bytes */
             for (Pos=0;(Pos<128)&&(EDIDBuffer_p!=NULL);Pos++,EDIDBuffer_p++);
        }

    /* Move to the next Extension Block */
    CurrentEdidExt_p->NextBlockExt_p = EDIDStruct_p->EDIDExtension_p;
    EDIDStruct_p->EDIDExtension_p = CurrentEdidExt_p;
   }
  Unit_p->Device_p->EDIDExtensionFlag = EDIDStruct_p->EDIDBasic.EDIDExtensionFlag;
  Unit_p->Device_p->EDIDExtension_p = EDIDStruct_p->EDIDExtension_p;

  /* For CEC Task */
#ifdef STHDMI_CEC
  Unit_p->Device_p->CEC_Params.Notify |= CEC_NOTIFY_EDID;
  Unit_p->Device_p->CEC_Params.IsEDIDRetrived = TRUE;
  STOS_SemaphoreSignal(Unit_p->Device_p->CEC_Sem_p);
#endif

  /*memory_deallocate(Unit_p->Device_p->CPUPartition_p,TargetInfo.SinkInfo.Buffer_p);*/ /*already made in STVOUT_Term()*/
  return(ErrorCode);
}/* end of sthdmi_FillSinkEDID() */
/* End of hdmi_snk.c */
/* ------------------------------- End of file ---------------------------- */

