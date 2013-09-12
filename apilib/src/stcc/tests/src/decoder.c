/******************************************************************************
 *
 * File Name   : decoder.c
 *
 * Description : implementaion of closed caption decoder engine
 *
 * Copyright 2000 STMicroelectronics. All Rights Reserved.
 *
 ******************************************************************************/

#include <string.h>
#include "trace.h"
#include <stdio.h>

#if !defined (ST_OSLINUX)
#include "sttbx.h"
#endif

#include "stos.h"
#include "stcc.h"
/*#include "Z:\dvdgr-prj-valitls\valid_trace.h"*/

/*
******************************************************************************
 Constants
******************************************************************************
*/

#define DriverPartition system_partition

#define DEFAULT_FOREGROUND  0x0     /* Italics, underline, and flash: Off */
#define DEFAULT_BACKGROUND  0x1F    /* Black, Opaque, ON BG */
#define DEFAULT_COLOR       0x0     /* White FG */
#define CHANNEL2            0x08

#define ERROR 0x7f
#define PARITY_RESET 0x7f
#define PARITY_ERROR 0x80

#define CONTROL_CODE_LOWER_RANGE    0x10  /* Control Codes have a range of 0x10 - 0x1F */
#define CONTROL_CODE_UPPER_RANGE    0x1F

#define PRINTING_CHAR_LOWER_LIMIT   0x20  /* Printing characters have a range of
                                             0x20 - 0x7f */

#define MISC_CODES_SET_2            0x17
#define MISC_CODES_SET_1_LINE21     0x14   /* first code of the 2 chars deals with line 21 */
#define MISC_CODES_SET_1_LINE284    0x15   /* first code of the 2 chars deals with line284 */
#define PAC_CONTROL_CODE            0x40
#define FIRST_MIDROW_OR_SPECIAL     0x11
#define SECOND_CHAR_SPECIAL         0x30

#define TAB_OFFSET_LOW              0x21
#define TAB_OFFSET_HIGH             0x23

#define TRANSPARENT_SPACE           0x19
/*---------------------------------*/
/*  CAPTION FLAGS                   */
/*---------------------------------*/
#define CAPTION_ENABLE              ((U8)0x01)
#define PRINT_PAIR                  ((U8)0x02)
#define PRINT_ENABLE                ((U8)0x04)
#define CONTROL_PAIR                ((U8)0x08)
#define TEXT_TRANSMISSION           ((U8)0x10)
#define FON_RECEIVED                ((U8)0x20)
#define VALID_DATA_RECEIVED         ((U8)0x40)
#define BANK1_ACTIVE                ((U8)0x80)

/*---------------------------------*/
/*  Caption Row Flags              */
/*---------------------------------*/
#define ROW_ON                      ((U8)0x01)
#define ROW_READY                   ((U8)0x02)

/*---------------------------------*/
/*  Foreground Caption style       */
/*---------------------------------*/
#define ITALICS                     ((U8)0x01)
#define UNDERLINE                   ((U8)0x02)
#define FLASH_ON                    ((U8)0x04)


/*---------------------------------*/
/*  USER INPUT FLAGS               */
/*---------------------------------*/
#define FIELD_1                     ((U8)0X01)
#define DATA_CHANNEL_2              ((U8)0X02)
#define TEXT_ENABLE                 ((U8)0x04)


/*---------------------------------*/
/*  Display Mode Control Flags     */
/*---------------------------------*/

#define ROLL_UP0                    ((U8)0x01)
#define ROLL_UP1                    ((U8)0x02)
#define PAINT_ON                    ((U8)0x04)
#define POP_ON                      ((U8)0x08)
#define SCROLL_ENABLED              ((U8)0x10)

/* for XDS_FLAG */
#define XDS                         ((U8)0x01)
#define CURRENT_CLASS               ((U8)0x02)
#define PACKET_START                ((U8)0x04)
#define PROGRAM_RATING              ((U8)0x08)
#define MISCELLANEOUS_CLASS         ((U8)0x10)
#define TOD                         ((U8)0x20)
#define TOD_VALID                   ((U8)0x40)
#define PR_VALID                    ((U8)0x80)


/* for XDS_FLAG1 */
#define PR_RECEIVED_ONCE            ((U8)0x01)
#define LTZ_VALID                   ((U8)0x02)
#define LTZ_RECEIVED_ONCE           ((U8)0x04)
#define LOCAL_TIME_ZONE             ((U8)0x08)

#define FIRST_ROW 0
#define LAST_ROW  20
#define BUFFER_SIZE (LAST_ROW-FIRST_ROW) + 1

#define TRACE_CC1_COLOR (TRC_ATTR_BACKGND_BLACK | TRC_ATTR_FORGND_RED | TRC_ATTR_BRIGHT)
#define TRACE_CC2_COLOR (TRC_ATTR_BACKGND_BLACK | TRC_ATTR_FORGND_GREEN | TRC_ATTR_BRIGHT)
#define TraceBufferColor(x) /*trace_color x*/
#define TRC_ATTR_FORGND_BLACK    0x01000000
#define TRC_ATTR_FORGND_RED      0x02000000
#define TRC_ATTR_FORGND_GREEN    0x03000000
#define TRC_ATTR_FORGND_YELLOW   0x04000000
#define TRC_ATTR_FORGND_BLUE     0x05000000
#define TRC_ATTR_FORGND_MAGENTA  0x06000000
#define TRC_ATTR_FORGND_CYAN     0x07000000
#define TRC_ATTR_FORGND_WHITE    0x08000000
#define TRC_ATTR_BACKGND_BLACK   0x10000000
#define TRC_ATTR_BACKGND_RED     0x20000000
#define TRC_ATTR_BACKGND_GREEN   0x30000000
#define TRC_ATTR_BACKGND_YELLOW  0x40000000
#define TRC_ATTR_BACKGND_BLUE    0x50000000
#define TRC_ATTR_BACKGND_MAGENTA 0x60000000
#define TRC_ATTR_BACKGND_CYAN    0x70000000
#define TRC_ATTR_BACKGND_WHITE   0x80000000
#define TRC_ATTR_BRIGHT          0x00000010





typedef struct
{
    U8  Char;
    U8  Color;          /*  Foreground Color */
    U8  Foreground;     /*  b0: Italics, b1: underline, b2: Flashing.
                            These style bits are either ON or OFF */
}STCC_CaptionChar_t;

typedef struct
{
    U8  ScreenRowNumber;
    U8  CharCount;
    U8  Background;     /*  b0 = Opacity: "0" = Opaque, "1" = Semi-transparent;
                        b1: BG ON/OFF, b4 b3 b2: Background color Style bits; */
    U8  RowFlags;           /*  Flags of row: b0 = ON/OFF, b1: Ready to display, */
  STCC_CaptionChar_t    CharacterRow[34];
}STCC_CaptionRow_t;

typedef struct
{
  BOOL ScrollRows;    /* 1 = ON; 0 = OFF */
  BOOL DisplayEnabled ;
  STCC_CaptionRow_t DisplayBuffer[BUFFER_SIZE];
  U8  ProgRating[4];                  /* used for V-chip programming */
  U8  LocalTimeZone[2];
  U8  TimeOfTheDay[6];
}STCC_DisplayData_t;

typedef struct
{
U8  NewChar1, NewChar2;             /* new NTSC characters */
U8  OldChar1, OldChar2;             /* old buffered NTSC characters */
U8  WindowSize;                     /* holds the size of roll-up window */
U8  CursorPosition, BufferRowNumber;
U8  CaptionFlags, DisplayModeFlags;
U8  UserInputFlags, XdsFlag;
U8  XdsFlag1;
U8* PntrToTable;
U8  ProgRating[4];                  /* used for V-chip programming */
U8  LocalTimeZone[2];
U8  TimeOfTheDay[6];
STCC_CaptionRow_t   CaptionBuffer[BUFFER_SIZE];
STCC_DisplayData_t DisplayData;
}STCC_InstanceData_t;

/*
******************************************************************************
Prototypes of all functions contained in this file
******************************************************************************
*/
void STCC_DisplayCaption(STCC_DisplayData_t* DisplayData_p);
void ProcessNewCC(U8* NTSC);
static void InitCC(void);
static void ClosedCaptionSetup(STCC_InstanceData_t* Inst_p);
static void ProcessCCControl(STCC_InstanceData_t* Inst_p);
static void ProcessPAC(STCC_InstanceData_t* Inst_p);
static void ProcessMidrow(STCC_InstanceData_t* Inst_p);
static void ProcessMisc1(STCC_InstanceData_t* Inst_p);
static void ProcessMisc2(STCC_InstanceData_t* Inst_p);
static void ProcessSpecial(STCC_InstanceData_t* Inst_p);
static void CopyChar(STCC_InstanceData_t* Inst_p, U8* NTSC);
static void UpdatePAC(STCC_InstanceData_t* Inst_p);
static void TextRestart(STCC_InstanceData_t* Inst_p);
static void ResumeTextDisplay(STCC_InstanceData_t* Inst_p);
static void ResumeDirectCaptioning(STCC_InstanceData_t* Inst_p);
static void ResumeCaptionLoading(STCC_InstanceData_t* Inst_p);
static void ClearRows(STCC_InstanceData_t* Inst_p,U8 RowNumber,U8 NumberOfRows,U8 nitialColumn);
static void FlashON(STCC_InstanceData_t* Inst_p);
static void TestMode(STCC_InstanceData_t* Inst_p);
static void DeleteCharacter(STCC_InstanceData_t* Inst_p, STCC_CaptionChar_t* ptr2);
static void PrintCharacter(STCC_InstanceData_t* Inst_p, U8 Character);
static void SetAttribute(STCC_InstanceData_t* Inst_p);
static void ProcessBank(STCC_InstanceData_t* Inst_p, U8 BankToProcess, U8 ScreenRow);
static void EndOFCaption(STCC_InstanceData_t* Inst_p);
static void Backspace(STCC_InstanceData_t* Inst_p);
static void PopOn(STCC_InstanceData_t* Inst_p, U8   ScreenRow);
static void PaintOn(STCC_InstanceData_t* Inst_p, U8 ScreenRow);
static void DeleteToEndOfRow(STCC_InstanceData_t* Inst_p);
static void EraseDisplayedMemory(STCC_InstanceData_t* Inst_p);
static void EraseNonDisplayedMemory(STCC_InstanceData_t* Inst_p);
static boolean IsParityValid(U8 character);
static void SetScreenMap(STCC_InstanceData_t* Inst_p, U8 BaseRow);
static void CommandRU(STCC_InstanceData_t* Inst_p);
static void RollUp(STCC_InstanceData_t* Inst_p, U8  BaseRow);
static void CarriageReturn(STCC_InstanceData_t* Inst_p);
static void SetXdsFlags(STCC_InstanceData_t* Inst_p);
static void UpdateDisplayBuffer(STCC_InstanceData_t* Inst_p);

static BOOL FirstRollUp = TRUE;
static STCC_InstanceData_t* Instance_p;
static STCC_InstanceData_t  Instance;


void STCC_DisplayCaption(STCC_DisplayData_t* DisplayData_p)

{
    U8 i,m,b;
#ifdef STCC_TERMINAL_DECODER
    U8 j;
#endif  /*ifdef STCC_TERMINAL_DECODER */

    if(DisplayData_p->DisplayEnabled == TRUE)
    {
        for(i=0;i<8;i++)
        {
            if(DisplayData_p->DisplayBuffer[i].RowFlags & ROW_ON)
            {
                if(DisplayData_p->DisplayBuffer[i].RowFlags & ROW_READY)
                {
                    if(DisplayData_p->DisplayBuffer[i].ScreenRowNumber > 9)
                    {
                        m=1;
                        b= DisplayData_p->DisplayBuffer[i].ScreenRowNumber-10;
                    }
                    else
                    {
                        m=0;
                        b=DisplayData_p->DisplayBuffer[i].ScreenRowNumber;
                    }
#ifdef STCC_TERMINAL_DECODER
                    TraceBuffer(("%c%c%d%d%c%c%c",  0x1b, '[',m ,b,';','3','H'));
                    TraceBuffer(("%c%c%c",  0x1b, '[', 'K'));


                    for(j=0;j<DisplayData_p->DisplayBuffer[i].CharCount;j++)
                    {
                     TraceBuffer(("%c",DisplayData_p->DisplayBuffer[i].CharacterRow[j].Char));
                    }
#endif  /*ifdef STCC_TERMINAL_DECODER */
                }
            }
        }
    }
    return;
}


void InitCC(void)
{
    int i, j;
    U32     ServiceChoice=1;

   Instance_p=& Instance;
   /*
    TraceBuffer(("Test Uart"));

    STTBX_Print(("HyperTerminal settings :38400 bps / 8bits / even\n"));
    STTBX_Print(("                       1 bit stop / no flow ctrl\n"));
    STTBX_Print(("                       ANSI terminal.\n")); */
    switch(ServiceChoice)
   {
        case 1:
        {   /* user choose CC1 */
        Instance_p->UserInputFlags |=  FIELD_1;       /* slice on field 1 */
        Instance_p->UserInputFlags &= ~TEXT_ENABLE;   /* mode CC */
        Instance_p->UserInputFlags &= ~DATA_CHANNEL_2;        /* CC1 */
        }
        break;

        case 2:
        {   /* user choose CC2 */
        Instance_p->UserInputFlags |=  FIELD_1;       /* slice on field 1 */
        Instance_p->UserInputFlags &= ~TEXT_ENABLE;   /* mode CC */
        Instance_p->UserInputFlags |=  DATA_CHANNEL_2;        /* CC2 */
        }
        break;

        case 3:
        {   /* user choose CC3 */
        Instance_p->UserInputFlags &= ~FIELD_1;        /* slice on field 2 */
        Instance_p->UserInputFlags &= ~TEXT_ENABLE;    /* mode CC */
        Instance_p->UserInputFlags &= ~DATA_CHANNEL_2;         /* CC3 */
        }
        break;

        case 4:
        {   /* user choose CC4 */
        Instance_p->UserInputFlags &= ~FIELD_1;        /* slice on field 2 */
        Instance_p->UserInputFlags &= ~TEXT_ENABLE;    /* mode CC */
        Instance_p->UserInputFlags |=  DATA_CHANNEL_2;         /* CC4 */
        }
        break;

        case 5:
        {   /* user choose TEXT1 */
        Instance_p->UserInputFlags |=  FIELD_1;        /* slice on field 1 */
        Instance_p->UserInputFlags |=  TEXT_ENABLE;    /* mode TEXT */
        Instance_p->UserInputFlags &= ~DATA_CHANNEL_2;         /* TEXT1 */
        }
        break;

        case 6:
        {   /* user choose TEXT2 */
        Instance_p->UserInputFlags |= FIELD_1;        /* slice on field 1 */
        Instance_p->UserInputFlags |= TEXT_ENABLE;     /* mode TEXT */
        Instance_p->UserInputFlags |= DATA_CHANNEL_2;         /* TEXT2 */
        }
        break;

        case 7:
        {   /* user choose TEXT3 */
        Instance_p->UserInputFlags &= ~FIELD_1;       /* slice on field 2 */
        Instance_p->UserInputFlags |= TEXT_ENABLE;     /* mode TEXT */
        Instance_p->UserInputFlags &= ~DATA_CHANNEL_2;        /* TEXT3 */
        }
        break;

        case 8:
        {   /* user choose TEXT4 */
        Instance_p->UserInputFlags &= ~FIELD_1;        /* slice on field 2 */
        Instance_p->UserInputFlags |= TEXT_ENABLE;      /* mode TEXT */
        Instance_p->UserInputFlags |= DATA_CHANNEL_2;          /* TEXT4 */
        }
        break;

        default:
        {
        return;
        }
   }

    Instance_p->UserInputFlags &=0x07;
    /* Initialize CC buffer rows and all related variables */
    for(i=0;i<16;i++)
    {
        Instance_p->CaptionBuffer[i].Background = DEFAULT_BACKGROUND;
        Instance_p->CaptionBuffer[i].CharCount = 0x0;
        Instance_p->CaptionBuffer[i].RowFlags = 0x0;
        Instance_p->CaptionBuffer[i].ScreenRowNumber = 15;
        Instance_p->CursorPosition = 0x0;
        Instance_p->DisplayData.DisplayEnabled = FALSE;

        for(j=0;j<35;j++)
        {
             Instance_p->CaptionBuffer[i].CharacterRow[j].Char = 0x0;
             Instance_p->CaptionBuffer[i].CharacterRow[j].Color = DEFAULT_COLOR;
             Instance_p->CaptionBuffer[i].CharacterRow[j].Foreground = DEFAULT_FOREGROUND;
        }
    }

   /*ClearRows(Instance_p,FIRST_ROW,BUFFER_SIZE,0);*/
   ClosedCaptionSetup(Instance_p);


   /* Init program rating */
   Instance_p->PntrToTable = NULL;
   for(i=0;i<4; i++)
   {
        Instance_p->ProgRating[i]=0x00;
        Instance_p->TimeOfTheDay[i] = 0x0;
   }
        Instance_p->TimeOfTheDay[4]  = 0x00;
        Instance_p->TimeOfTheDay[5]  = 0x00;
        Instance_p->LocalTimeZone[0] = 0x00;
        Instance_p->LocalTimeZone[1] = 0x00;

   UpdateDisplayBuffer(Instance_p);
   Instance_p->DisplayData.ScrollRows = FALSE;
}

/*
----------------------------------------------------------
ClosedCaptionSetup(): Initializes closed caption variables
mostly global variables that are related to mode and operation
----------------------------------------------------------
*/
void ClosedCaptionSetup(STCC_InstanceData_t* Inst_p)
{
  int i;

  /* Initialiaze the decoder values to defaults */
    Inst_p->BufferRowNumber = 0;
    Inst_p->NewChar1 = Inst_p->OldChar1 = 0x0;
    Inst_p->NewChar2 = Inst_p->OldChar2 = 0x0;
    Inst_p->WindowSize = 0x3;
    Inst_p->XdsFlag = 0x0;
    Inst_p->XdsFlag1 = 0x0;

    Inst_p->CaptionFlags = 0;
    Inst_p->DisplayModeFlags = 0;
    Inst_p->CursorPosition = 0;

  for ( i=0;i<16;i++)
  {
    Inst_p->CaptionBuffer[i].CharCount = 0x0;
    Inst_p->CaptionBuffer[i].RowFlags = 0x0;
    Inst_p->CaptionBuffer[i].ScreenRowNumber = i;
  }

}

/*
-----------------------------------------------------------
ProcessNewCC(): This function processes the new data condition.
New data available from the Slicer is checked for possible
corruption of the parity bit on character two (so called rabbit ears).
New data is also identified as a control pair or printing pair.
We then check to see if the display should be automatically enabled,
and process the data accordingly.
-----------------------------------------------------------
*/
void ProcessNewCC( U8* NTSC)
{
    static U32 FirstInit=0;
    STCC_DisplayData_t * DisplayData_p ;

    if(FirstInit==0)
    {
        InitCC();
    }
    FirstInit++;
    DisplayData_p = &(Instance_p->DisplayData);

    Instance_p->DisplayData.DisplayEnabled = FALSE;
    CopyChar(Instance_p,NTSC); /* copy character if thier parity is Ok */

    /* Are parity bits corrupted ? */
    if ((Instance_p->NewChar1 & PARITY_ERROR)||(Instance_p->NewChar2 & PARITY_ERROR))
    {  /* Yes, at least one */

        if (Instance_p->NewChar2 & PARITY_ERROR)  /* Is the parity bit of the first character corrupted ? */
        { /* Yes, set to character error */
            Instance_p->NewChar1 = ERROR;

            if (Instance_p->NewChar2 & PARITY_ERROR) /* Is the parity bit of the second character corrupted ? */
            {    /* Yes, set to character error */
                 Instance_p->NewChar2 = ERROR;
            }
            else
            {  /* Is the previous data a control pair ?
                  (as you know, control codes are transmitted twice) */
                if (Instance_p->CaptionFlags & CONTROL_PAIR)
                {    /* Yes, is it the same control pair ? */
                    if (Instance_p->NewChar2 == Instance_p->OldChar2)
                    {   /* Yes, do not print error character, this control pair
                        has already been processed (transmitted twice) */
                        Instance_p->CaptionFlags &= ~CONTROL_PAIR;
                        return;
                    }
                }
            }
        }
        else
        {      /* First character ok so, set the character two to error */
            Instance_p->NewChar2 = ERROR;
            if (Instance_p->NewChar1 >= CONTROL_CODE_LOWER_RANGE && Instance_p->NewChar1 <= CONTROL_CODE_UPPER_RANGE)  /* Is it a control pair ? */
            {   /* Yes, do not print error character, may be we will be able to
                process the next same control pair or may be it has already been
                processed (transmitted twice) */
                Instance_p->CaptionFlags &= ~CONTROL_PAIR;
                return;
            }
        }

        /* Print error characters, Skip EDS characters on field 2  */
        if (Instance_p->NewChar1 < CONTROL_CODE_LOWER_RANGE)
        {
            if(Instance_p->NewChar1 != 0)   /* Not a NULL */
            {
                Instance_p->CaptionFlags &= ~PRINT_ENABLE; /* Do not print to
                                                         Caption Buffer */
            }
            if ((Instance_p->NewChar1 > 0x00) || !(Instance_p->UserInputFlags & FIELD_1))
            { /* xds data on field 2 available */
                SetXdsFlags(Instance_p);  /* Update and save XDS data */
            }
        }

        Instance_p->CaptionFlags &= ~CONTROL_PAIR;    /* Set related flag */

         /* Check if we are allowed to print the current character */
        if (    (Instance_p->CaptionFlags & PRINT_ENABLE)
             && (   (Instance_p->DisplayModeFlags != 0x00)
                 || (Instance_p->CaptionFlags & TEXT_TRANSMISSION)))
        {   /* Skip non printing character */
            if (Instance_p->NewChar1 >= PRINTING_CHAR_LOWER_LIMIT)
            {
                PrintCharacter(Instance_p, Instance_p->NewChar1);
            }
            /* Skip non printing character */
            if (Instance_p->NewChar2 >= PRINTING_CHAR_LOWER_LIMIT)
            {
                PrintCharacter(Instance_p, Instance_p->NewChar2);
            }
        }
    }
    else
    {      /* No, parity corruption, everything is fine */

        if((Instance_p->XdsFlag & PACKET_START)||!(Instance_p->UserInputFlags & FIELD_1))
        { /* xds data received */
            SetXdsFlags(Instance_p);
        }
        else
        {
            /* Printing pair or control pair ? */
            if (Instance_p->NewChar1 >= CONTROL_CODE_LOWER_RANGE && Instance_p->NewChar1 <= CONTROL_CODE_UPPER_RANGE)
            {    /* It is a control pair, is the previous data also a control pair ?
                (control codes are transmitted twice) */
                if (Instance_p->CaptionFlags & CONTROL_PAIR)
                {   /* Yes, is it the same control pair ? */
                    if ((Instance_p->NewChar1 == Instance_p->OldChar1) && (Instance_p->NewChar2 == Instance_p->OldChar2))
                    {     /* Yes, do not process control pair twice (it as already been done) */
                        Instance_p->CaptionFlags &= ~CONTROL_PAIR;
                        return;
                    }
                }
                Instance_p->CaptionFlags |= CONTROL_PAIR; /* Set related flag and process control pair (first time) */
                ProcessCCControl(Instance_p);
            }
            else
            {
                /* Process printing pair or process XDS characters on field 2*/
                if (Instance_p->NewChar1 < CONTROL_CODE_LOWER_RANGE)
                {
                    if(Instance_p->NewChar1 !=0)
                    {
                        Instance_p->CaptionFlags &= ~PRINT_ENABLE;
                    }
                    if ((Instance_p->NewChar1 > 0x00) || !(Instance_p->UserInputFlags & FIELD_1))
                    { /* XDS data received */
                        SetXdsFlags(Instance_p);
                    }
                }
                Instance_p->CaptionFlags &= ~CONTROL_PAIR;       /* Set related flag */

                /* Check if we are allowed to print the current character */
                if (    (Instance_p->CaptionFlags & PRINT_ENABLE)
                     && (    (Instance_p->DisplayModeFlags != 0x00)
                          || (Instance_p->CaptionFlags & TEXT_TRANSMISSION)))
                {
                    /* Skip non printing character */
                    if (Instance_p->NewChar1 >= PRINTING_CHAR_LOWER_LIMIT)
                    {
                        PrintCharacter(Instance_p, Instance_p->NewChar1);
                        /*Instance_p->DisplayData.DisplayEnabled |= 0x01;*/
                        Instance_p->DisplayData.DisplayEnabled = TRUE;
                    }
                    /* Skip non printing character */
                    if (Instance_p->NewChar2 >= PRINTING_CHAR_LOWER_LIMIT)
                    {
                        PrintCharacter(Instance_p, Instance_p->NewChar2);
                        /*Instance_p->DisplayData.DisplayEnabled |= 0x01;*/
                        Instance_p->DisplayData.DisplayEnabled = TRUE;
                    }
                }
            }
        }
    }
    UpdateDisplayBuffer(Instance_p);
    /* if(FirstInit %8 == 0) */
    STCC_DisplayCaption(&Instance_p->DisplayData);

    return;
}

/*
--------------------------------------------------------------
ProcessCCControl(): Checks the control codes recieved. If they
are intended for the channel the user has selected then the control
get processed. otherwise discard them.
--------------------------------------------------------------
*/
void ProcessCCControl(STCC_InstanceData_t* Inst_p)
{
    /* see if the control codes are for the channel we're currently decoding */

   if((Inst_p->NewChar1 & 0x08) && !(Inst_p->UserInputFlags & DATA_CHANNEL_2))
   {    /* No, DATA_CHANNEL_2 mismatch */
        Inst_p->CaptionFlags &=~PRINT_ENABLE;
        return;
   }
   if(!(Inst_p->NewChar1 & 0x08)&& (Inst_p->UserInputFlags & DATA_CHANNEL_2))
   {
      Inst_p->CaptionFlags &=~PRINT_ENABLE;
      return;
   }
   /* Yes, a channel match, then save new character */
   Inst_p->OldChar1 = Inst_p->NewChar1;
   Inst_p->OldChar2 = Inst_p->NewChar2;

   Inst_p->NewChar1 &=~0x08; /* resets the channel bit */

   /* Process if the second character is a printing character */
   if(Inst_p->NewChar2 >= PRINTING_CHAR_LOWER_LIMIT)
   {   /* Is the second character in the valid range */
       /* Does the control code change mode? */
      if((Inst_p->NewChar1 == MISC_CODES_SET_1_LINE21)||(Inst_p->NewChar1 == MISC_CODES_SET_1_LINE284))
      {
         if(((Inst_p->NewChar1 == MISC_CODES_SET_1_LINE21) &&
             (Inst_p->UserInputFlags & FIELD_1))||
             ((Inst_p->NewChar1 ==MISC_CODES_SET_1_LINE284) &&
             (!(Inst_p->UserInputFlags & FIELD_1))))
            {
                /* Is this a misc 1 command */
                if(Inst_p->NewChar2 < SECOND_CHAR_SPECIAL)
                {
                    ProcessMisc1(Inst_p);
                    return;
                }
            }
      }

        /* See if this data is what the user selected*/
        TestMode(Inst_p);
        if(Inst_p->CaptionFlags & PRINT_ENABLE)
         {
            /* Is it a PAC */
            if(Inst_p->NewChar2>= PAC_CONTROL_CODE)
            {
                ProcessPAC(Inst_p);
                return;
            }
             /* Is it a midow or specil control code */
            if(Inst_p->NewChar1 == FIRST_MIDROW_OR_SPECIAL)
            {
                if(Inst_p->NewChar2 >= SECOND_CHAR_SPECIAL)
                {
                    ProcessSpecial(Inst_p);
                    return;
                }
                else
                {
                ProcessMidrow(Inst_p);
                return;
                }
            }

            /* Is it a misc.2 */
            if(Inst_p->NewChar1 == MISC_CODES_SET_2)
            {
                ProcessMisc2(Inst_p);
                return;
            }

        }


   }
}
/*
-----------------------------------------------------------
ProcessPAC(): This function checks the PAC for validity and
calculates the specified screen row. It then processes the
PAC according to the current type of caption or Text being received.
-----------------------------------------------------------
*/
void ProcessPAC(STCC_InstanceData_t* Inst_p)
{
    U8 ScreenRow;

    ScreenRow = Inst_p->NewChar1 - 0x10;      /* Set first character range to 0-7 */

    if ((ScreenRow == 0) && (Inst_p->NewChar1 >= 0x60))        /* Return if out of range */
    {
        return;
    }

    switch (ScreenRow)
    {
        case 0:
        {
            ScreenRow = 11;
        }
        break;

        case 1:
        {
            ScreenRow = 1;
        }
        break;

        case 2:
        {
            ScreenRow = 3;
        }
        break;

        case 3:
        {
            ScreenRow = 12;
        }
        break;

        case 4:
        {
            ScreenRow = 14;
        }
        break;

        case 5:
        {
            ScreenRow = 5;
        }
        break;

        case 6:
        {
            ScreenRow = 7;
        }
        break;

        case 7:
        {
            ScreenRow = 9;
        }
        break;
    }

    /* Access second defined row if bit 5 is set */
    if (Inst_p->NewChar2 & PRINTING_CHAR_LOWER_LIMIT)
    {
        ScreenRow++;
        /* Then reset second row bit */
        Inst_p->NewChar2 &= ~PRINTING_CHAR_LOWER_LIMIT;
    }

    /* Update row attribute if Text codes */
    if (Inst_p->CaptionFlags & TEXT_TRANSMISSION)
    {
        UpdatePAC(Inst_p);
        return;
    }
    /* Otherwise, update related modes */
    if (Inst_p->DisplayModeFlags & POP_ON)
    {
        PopOn(Inst_p, ScreenRow);
        return;
    }
    if (Inst_p->DisplayModeFlags & PAINT_ON)
    {
        PaintOn(Inst_p, ScreenRow);
        return;
    }

    RollUp(Inst_p, ScreenRow);
}


/*
-----------------------------------------------------------
ProcessMidrow(): This function processes the midrow control
code pair. It sets the color to the received value and writes
it into the Display Buffer.
-----------------------------------------------------------
*/
void ProcessMidrow(STCC_InstanceData_t* Inst_p)
{
    /* Set color attributes */
    SetAttribute(Inst_p);
}
/*
-----------------------------------------------------------
ProcessMisc1(): processes the group1-Misc Commands
Misc1 Commands are:
    ResumeCaptionLoading:   Resume Caption Loading
    Backspace:   BackSpace
   DeleteToEndOfRow:    Delete to End of Row
   RU2: Roll Up 2
   RU3: Roll Up 3
   RU4: Roll Up 4
   FO:  Flash On
   ResumeDirectCaptioning:  Resume Direct Captioning
   TextRestart: Text Restart
    ResumeTextDisplay:  Resume Text Dislay
   EraseDisplayedMemory:    Erase Displayed Memory

   CarriageReturn:  Carriage Return
   EraseNonDisplayedMemory: Erase Nondisplayed Memory
   EndOFCaption:    End Of Caption
-----------------------------------------------------------
*/
void ProcessMisc1(STCC_InstanceData_t* Inst_p)
{
    /* Instance_p->NewChar2 -=PRINTING_CHAR_LOWER_LIMIT;*/

   switch(Inst_p->NewChar2 -PRINTING_CHAR_LOWER_LIMIT)
   {
      case 0:
          ResumeCaptionLoading(Inst_p);
          break;

      case 1:
        Backspace(Inst_p);
        break;

      case 2:
      case 3:
        return;
        break;

      case 4:
        DeleteToEndOfRow(Inst_p);
        break;

      case 5:
      case 6:
      case 7:
        CommandRU(Inst_p);
        break;

      case 8:
        FlashON(Inst_p);
        break;

      case 9:
        ResumeDirectCaptioning(Inst_p);
        break;

      case 10:
        TextRestart(Inst_p);
        break;

      case 11:
        ResumeTextDisplay(Inst_p);
        break;

      case 12:
        EraseDisplayedMemory(Inst_p);
        break;

      case 13:
        CarriageReturn(Inst_p);
        break;

      case 14:
        EraseNonDisplayedMemory(Inst_p);
        break;

      case 15:
      {
      EndOFCaption(Inst_p);
      }break;
   }
}
/*
-----------------------------------------------------------
ProcessMisc2(): This function processes the second group of
miscellaneous commands. These are tab commands: TO1,TO2,TO3.
-----------------------------------------------------------
*/
void ProcessMisc2(STCC_InstanceData_t* Inst_p)
{
   U8 TabControl;

    TabControl = Inst_p->NewChar2;

    /* Make sure we have received a TAB command */
    if ((Inst_p->NewChar2 >= 0x21) && (Inst_p->NewChar2 <= 0x23))
    {
        /* If so, keep number of CursorPosition to tab over */
        TabControl &= 0x03;
        for (; TabControl>0; TabControl--)
        {
            /* Increment CursorPosition as long as CursorPosition is lower than 32 characters */
            if (Inst_p->CursorPosition < 33)
                Inst_p->CursorPosition++;
        }
    }
}
/*
-----------------------------------------------------------
ProcessSpecial():  This function writes the specified special
character into the Display Buffer. The value must be shifted
to produce the correct printing ascii character. Transparent
space is a special case, but i do not know if it is possible
to get a transparent space via special characters.
-----------------------------------------------------------
*/
void ProcessSpecial(STCC_InstanceData_t* Inst_p)
{
    U8 i;

    /* Shift to printing characters */
    i = (Inst_p->NewChar2 - PRINTING_CHAR_LOWER_LIMIT);
    /* Is a transparent space ? */
    if (i == 0x19)
        PrintCharacter(Inst_p,0);
    else
    {
        /* print special char */
        PrintCharacter(Inst_p,i);
    }
}
/*
-----------------------------------------------------------
EndOFCaption():   This function processes the EndOFCaption command. It sets the
current type of data being received to a pop-on caption. It
will swap the current cursor from the other bank if in a pop-on
caption previous to this command. It toggles the bank to be
displayed and requests the screen to be refreshed.
-----------------------------------------------------------
*/
void EndOFCaption(STCC_InstanceData_t* Inst_p)
{
    int i;

    if(Inst_p->CaptionFlags & TEXT_TRANSMISSION)
    {
        /* set mode to Caption transmission */
        Inst_p->CaptionFlags &= ~TEXT_TRANSMISSION;
    }
    else
    {
        /* See if data is what the user wants */
        TestMode(Inst_p);
        if (Inst_p->CaptionFlags & PRINT_ENABLE)
        {
            /* Disable vertical scrolling */
            /* Instance_p->VerticalScrollingEnabled = 0;*/
            Inst_p->DisplayModeFlags &=~SCROLL_ENABLED;
            /* Disable paint on and roll_up modes */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags & ~(PAINT_ON | ROLL_UP1 | ROLL_UP0);
            /* Enable pop_on mode */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags | POP_ON;

            /* Toggle ON/OFF the relevant rows */

            if(Inst_p->CaptionFlags & BANK1_ACTIVE)
            {
                    for(i=0;i<4;i++)
                    {
                        Inst_p->CaptionBuffer[i].RowFlags |=ROW_ON;
                        Inst_p->CaptionBuffer[i+4].RowFlags &=~ROW_ON;
                    }
                    Inst_p->CaptionFlags &=~BANK1_ACTIVE;
            }
            else
            {
                for(i=0;i<4;i++)
                {
                    Inst_p->CaptionBuffer[i].RowFlags &=~ROW_ON;
                    Inst_p->CaptionBuffer[i+4].RowFlags |=ROW_ON;
                }
                /* Ignore printing character until next valid control code */
                Inst_p->CaptionFlags &= ~PRINT_ENABLE;
                Inst_p->CaptionFlags |=BANK1_ACTIVE;
            }

        }
    }
}
/*
-----------------------------------------------------------
ResumeTextDisplay(): This function processes the Resume Text
Display command. It sets the current type of data being
received to Text.
-----------------------------------------------------------
*/
void ResumeTextDisplay(STCC_InstanceData_t* Inst_p)
{
    /* We are in Text transmission */
    Inst_p->CaptionFlags |= TEXT_TRANSMISSION;
    /* See if data is what the user wants */
    TestMode(Inst_p);
}
/*
-----------------------------------------------------------
ResumeCaptionLoading():  This function processes the ResumeCaptionLoading command.
- It sets the current type of data being received to a pop-on caption.
- It will swap the current cursor for the saved cursor from the other
bank if not in a pop-on caption previous to this command.
-----------------------------------------------------------
*/
void ResumeCaptionLoading(STCC_InstanceData_t* Inst_p)
{
    STCC_CaptionRow_t*  ptr2;
    int i;
    /* We are in Caption transmission */
    Inst_p->CaptionFlags &= ~TEXT_TRANSMISSION;
    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (Inst_p->CaptionFlags & PRINT_ENABLE)
    {
        if(Inst_p->DisplayModeFlags !=0 )
        {   /* not the first time to run decoder */
            /* Disable paint on, roll_up modes */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags & ~(PAINT_ON | ROLL_UP1 | ROLL_UP0);
            /* Enable pop on mode */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags | POP_ON;
            /* Instance_p->VerticalScrollingEnabled = 0;*/
            Inst_p->DisplayModeFlags &=~SCROLL_ENABLED;

            /* point to row number 4 */
            ptr2 = &Inst_p->CaptionBuffer[4];
            /* Select the second bank  as default displayed bank */
            Inst_p->BufferRowNumber = Inst_p->BufferRowNumber | 0x04;

            /* If we are displaying bank 1 select bank 0 */
            if (ptr2->RowFlags &ROW_ON)
            {
                Inst_p->BufferRowNumber = Inst_p->BufferRowNumber & ~0x04;
                Inst_p->CaptionFlags |=BANK1_ACTIVE;
            }
            else
            {
                Inst_p->CaptionFlags &=~BANK1_ACTIVE;
            }
        }
        else
        {   /* First time to enter decoder, must turn on one of the banks */
            /* Disable paint on, roll_up modes */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags & ~(PAINT_ON | ROLL_UP1 | ROLL_UP0);
            /* Enable pop on mode */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags | POP_ON;
            /* Instance_p->VerticalScrollingEnabled = 0;*/
            Inst_p->DisplayModeFlags &=~SCROLL_ENABLED;

            Inst_p->BufferRowNumber = 0x0;
            /* Select the first bank  as default displayed bank */
            for(i=0;i<4;i++)
            {
                Inst_p->CaptionBuffer[i].RowFlags &=~ROW_ON;
                Inst_p->CaptionBuffer[i+4].RowFlags |=ROW_ON;
            }
            Inst_p->CaptionFlags |=BANK1_ACTIVE;
        }
    }
}
/*
-----------------------------------------------------------
ResumeDirectCaptioning():  This function processes the ResumeDirectCaptioning command.
- It sets the current type of data being received to a paint-caption.
- It will swap the current cursor for the saved cursor from the
other bank if in a pop-on caption previous to this command .
-----------------------------------------------------------
*/
void ResumeDirectCaptioning(STCC_InstanceData_t* Inst_p)
{
    STCC_CaptionRow_t*  ptr2;
    U8 i;
   /* We are in Caption transmission */
    Inst_p->CaptionFlags &= ~TEXT_TRANSMISSION;
    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (Inst_p->CaptionFlags & PRINT_ENABLE)
    {
        /* if this is the first time decoder runs */
        if(Inst_p->DisplayModeFlags !=0)
        {
            /* Disable pop on mode */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags & ~(POP_ON | ROLL_UP1 | ROLL_UP0);
            /* Enable paint on mode */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags | PAINT_ON;
            /* Instance_p->VerticalScrollingEnabled = 0;*/
            Inst_p->DisplayModeFlags &=~SCROLL_ENABLED;
            /* point to row 4 of the caption buffer */
            ptr2 = &Inst_p->CaptionBuffer[4];

            Inst_p->BufferRowNumber &=0x07;
            Inst_p->BufferRowNumber = Inst_p->BufferRowNumber & ~0x04;
            /* If we are displaying rows 4-7 then select rows 4-7 */
            if (ptr2->RowFlags & ROW_ON)
            {
                Inst_p->BufferRowNumber &=0x07;
                Inst_p->BufferRowNumber = Inst_p->BufferRowNumber | 0x04;
            }
        }
        else
        {
            /* First time to enter decoder, must turn on one of the banks */
            /* Disable pop on, roll_up modes */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags & ~(POP_ON | ROLL_UP1 | ROLL_UP0);
            /* Enable pop on mode */
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags | PAINT_ON;
            /* Instance_p->VerticalScrollingEnabled = 0;*/
            Inst_p->DisplayModeFlags &=~SCROLL_ENABLED;

            Inst_p->BufferRowNumber = 0x0;
            /* Select the first bank  as default displayed bank */
            for(i=0;i<4;i++)
            {
                Inst_p->CaptionBuffer[i].RowFlags |=ROW_ON;
                Inst_p->CaptionBuffer[i+4].RowFlags &=~ROW_ON;
            }

        }
    }
}
/*
-----------------------------------------------------------
CommandRU():  This function processes the RU2, RU3, RU4
commands. It checks to see if we are already processing a roll-up
caption. If not, it erases all buffer rows . If so, it checks to
see if the size is too large for the current base row. If yes, it
lowers the window size to fit. It erases rows in the buffer which
are outside the window. The value stored in DisplayModeFlags for
the number of rows in the window is actually one less than the
number of rows.
-----------------------------------------------------------
*/
void CommandRU(STCC_InstanceData_t* Inst_p)
{
  /*STCC_CaptionRow_t* ptr1;*/
    U8 NewWindowSize, CurrentWindowSize;
    /* We are in Caption transmission */
    Inst_p->CaptionFlags &= ~TEXT_TRANSMISSION;
    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (Inst_p->CaptionFlags & PRINT_ENABLE)
    {
        /* Disable vertical scrolling */
        Inst_p->DisplayModeFlags &=~SCROLL_ENABLED;
        /* Get roll-up mode */
        NewWindowSize = Inst_p->NewChar2 - 0x24;

        /* Get current roll-up window size */
        CurrentWindowSize = Inst_p->DisplayModeFlags & (ROLL_UP1 | ROLL_UP0);
        /* Is it the first time we enter the roll_up mode */
        if (CurrentWindowSize != 0)
        {
            /* No, update window size */
            Inst_p->DisplayModeFlags &=~(POP_ON|PAINT_ON|ROLL_UP1|ROLL_UP0);
            Inst_p->DisplayModeFlags = Inst_p->DisplayModeFlags|NewWindowSize;

            Inst_p->WindowSize = ++NewWindowSize;
            Inst_p->BufferRowNumber = Inst_p->WindowSize - 1;

        }
        else
        {
            /* First time into roll_up mode, set new window size */
            /*Instance_p->DisplayModeFlags |=CR_FIRST_TIME;*/

            Inst_p->DisplayModeFlags &=~(POP_ON|PAINT_ON|ROLL_UP1|ROLL_UP0);

            Inst_p->DisplayModeFlags |= NewWindowSize;

            ClearRows(Inst_p, FIRST_ROW, 8,0);
            /* initial row number to start with  */
            /* Start with rows 3-0 as default */
            NewWindowSize++;
            Inst_p->WindowSize = NewWindowSize;

            Inst_p->BufferRowNumber = Inst_p->WindowSize - 1;
        }
    }

}

/*
-----------------------------------------------------------
RollUp():  This function checks to see if the screen row
specified by the PAC is valid. It then remaps the rows
accordingly and updates the cursor position.
-----------------------------------------------------------
*/
void RollUp(STCC_InstanceData_t* Inst_p, U8 BaseRow)
{

    /* If no mode is selected then default to RU2 */
    if((Inst_p->DisplayModeFlags & 0x0F) == 0)
   {
    Inst_p->DisplayModeFlags |=ROLL_UP0;
    /* Get roll-up window size */
    Inst_p->WindowSize = 3;
   }
    /* If window size higher than screen row number, set screen row number
    to window size */
    if (Inst_p->WindowSize > BaseRow)
        BaseRow = Inst_p->WindowSize;
    SetScreenMap(Inst_p, BaseRow);

    UpdatePAC(Inst_p);
}
/*
-----------------------------------------------------------
TextRestart():  This function processes the TextRestart command.
It sets the current type of data being received to Text, clears
the screen, and moves the cursor to home.
-----------------------------------------------------------
*/
void TextRestart(STCC_InstanceData_t* Inst_p)
{
    /* set to Text transmission */
    Inst_p->CaptionFlags |= TEXT_TRANSMISSION;
    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (Inst_p->CaptionFlags & PRINT_ENABLE)
    {
        /* Turn-off scrolling till page is full */
        Inst_p->DisplayModeFlags &= ~SCROLL_ENABLED;
        /* Erase the whole buffer */
        ClearRows(Inst_p, FIRST_ROW, BUFFER_SIZE,0);

        /* Do not display the first row in Text mode */

        /* Point the first location of the Display Buffer */
        Inst_p->BufferRowNumber = 1;
        Inst_p->CursorPosition = 0;

        /* Map row by setting their vertical address on the screen (in Text
        mode, we should be able to display 15 lines */
        Inst_p->WindowSize = BUFFER_SIZE;
        SetScreenMap(Inst_p,15);
    }
}
/*
-----------------------------------------------------------
FlashON():  This function processes the FlashON command. The flash
attribute is set and the foreground color is written into
the buffer.
-----------------------------------------------------------
*/
void FlashON(STCC_InstanceData_t* Inst_p)
{
    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (Inst_p->CaptionFlags & PRINT_ENABLE)
    {
        /* Set FON_RECEIVED bit to update Row atttribute */
        Inst_p->CaptionFlags |=FON_RECEIVED;
        SetAttribute(Inst_p);
    }
}
/*
-----------------------------------------------------------
Backspace(): Meaning : This function processes the Backspace command. It
checks to see if the cursor is already at the start of the
line. If not, it checks to see if it is at the end of the
line. If so, the last two characters will be deleted. The
cursor is moved back one character and it is deleted.
Check FCC recommendations on Backspace
-----------------------------------------------------------
*/
void Backspace(STCC_InstanceData_t* Inst_p)
{
    U8 i;
    STCC_CaptionChar_t* ptr1;

    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (Inst_p->CaptionFlags & PRINT_ENABLE)
    {
        /* Is it the begining of the row ? */
        if (Inst_p->CursorPosition != 0)
        {
            /* No, decrement cursor position */
            Inst_p->CursorPosition--;

            /* Point to the current location of the Caption Buffer */
            ptr1 = &Inst_p->CaptionBuffer[Inst_p->BufferRowNumber].
                    CharacterRow[Inst_p->CursorPosition];

            /* is it the end of the row ? */
            if (Inst_p->CursorPosition >= 32)
            {
                /* Still the same problem, if we are in Text mode, then write
                   Caption background. Otherwise clear the two last characters */
                i = 0;
                if (Inst_p->UserInputFlags & TEXT_ENABLE)
                i = 0x20;


                (ptr1-1)->Char = i;
                (ptr1-1)->Foreground =  DEFAULT_FOREGROUND;
                (ptr1-1)->Color =       DEFAULT_COLOR;

                (ptr1-2)->Char = i;
                (ptr1-2)->Foreground =  DEFAULT_FOREGROUND;
                (ptr1-2)->Color =       DEFAULT_COLOR;

            }
        /* Delete character at current cursor position */
        DeleteCharacter(Inst_p, ptr1);
        }
    }
}
/*
-----------------------------------------------------------
PopOn():  This function proceses the non-displayed bank
-----------------------------------------------------------
*/
void PopOn(STCC_InstanceData_t* Inst_p, U8  ScreenRow)
{
    U8 RowToProcess;

    /* Select rows 4-7 as default */
    RowToProcess = 4;
    if(Inst_p->CaptionBuffer[4].RowFlags & ROW_ON)
        RowToProcess = 0;

    ProcessBank(Inst_p, RowToProcess, ScreenRow);
}

/*
-----------------------------------------------------------
PaintOn():
-----------------------------------------------------------
*/
void PaintOn(STCC_InstanceData_t* Inst_p, U8    ScreenRow)
{
    U8 RowToProcess;

    /* Select bank 0 as default */
    RowToProcess = 0;
    if(Inst_p->CaptionBuffer[4].RowFlags & ROW_ON)
        RowToProcess = 4;

    ProcessBank(Inst_p,RowToProcess, ScreenRow);
}
/*
-----------------------------------------------------------
DeleteToEndOfRow():  This function processes the DeleteToEndOfRow command. It deletes
all characters from the current cursor position to the end
of the row. It checks to see if the trailing control code
should be added. Finally, it checks to see if the row is
empty so it can be freed.
-----------------------------------------------------------
*/
void DeleteToEndOfRow(STCC_InstanceData_t* Inst_p)
{

    U8 TempCursorPosition;
    STCC_CaptionChar_t *ptr1, *temp;

    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (Inst_p->CaptionFlags & PRINT_ENABLE)
    {

        TempCursorPosition = Inst_p->CursorPosition;
        /*ptr1 = &Instance_p->DisplayBuffer[Instance_p->BufferRowNumber][Instance_p->CursorPosition];*/
        ptr1 = &Inst_p->CaptionBuffer[Inst_p->BufferRowNumber].
                                        CharacterRow[Inst_p->CursorPosition];
        /* Save current cursor position for a while */
        /*q = ptr1;*/
        temp = ptr1;

        /* Delete character to the end of the row without changing cursor position */
        ClearRows(Inst_p, Inst_p->BufferRowNumber, 1, Inst_p->CursorPosition);
        Inst_p->CursorPosition = TempCursorPosition;  /* ClearRow() resets cursor to 1,
                                                but we must restore it */
    }
}

/*
----------------------------------------------------------
EraseDisplayedMemory(): This function processes the Erase Display Memory
command. The current bank being displayed is checked, then erased.
----------------------------------------------------------
*/
void EraseDisplayedMemory(STCC_InstanceData_t* Inst_p)
{
    U8 i;
    STCC_CaptionRow_t* ptr1;

    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (!(Inst_p->UserInputFlags & TEXT_ENABLE))
    {
        /* Instance_p->VerticalScrollingEnabled = 0;*/
        Inst_p->DisplayModeFlags &=~SCROLL_ENABLED;
        /* Process the first bank as default */
        i = FIRST_ROW;
        ptr1 = &Inst_p->CaptionBuffer[i];
        /* Is the second bank selected ? */
        if (!(ptr1->RowFlags & ROW_ON))
            /* Yes, select the second bank */
            i = FIRST_ROW+4;
        /* Erase the slected bank */
        ClearRows(Inst_p,i, 4,0);
    }
}

/*
----------------------------------------------------------
EraseNonDisplayedMemory(): This function processes the EraseNonDisplayedMemory command. The current
bank being displayed is checked, then the other bank is erased.
----------------------------------------------------------
*/
void EraseNonDisplayedMemory(STCC_InstanceData_t* Inst_p)
{
    U8 i;
    STCC_CaptionRow_t* ptr1;
    /* See if data is what the user wants */
    TestMode(Inst_p);
    if (!(Inst_p->UserInputFlags & TEXT_ENABLE))
    {
        /* Process the second bank as default */
        i = FIRST_ROW+4;
        ptr1 = &Inst_p->CaptionBuffer[i];
        /* Is the second bank selected ? */
        if (ptr1->RowFlags & ROW_ON)
            /* Yes, select the first bank */
            i = FIRST_ROW;
        /* Erase the selected bank */
        ClearRows(Inst_p,i, 4,0);
    }
}


/*
----------------------------------------------------------
CarriageReturn(): This function processes the CarriageReturn command. It checks to
see if the Caption is a roll-up one. If so, it erases the
line which was at the top of the window. It moves the cursor
to the start of the next line. It puts the new row at the same
location as the previous base row. It then decrements the row
numbers and vertical address for the other rows. Finally, it sets
the current row to be at the top of the window, and sets the
scroll offset.
----------------------------------------------------------
*/

void CarriageReturn(STCC_InstanceData_t* Inst_p)
{
  U8 i, TempScreenRow, count;
    STCC_CaptionRow_t TempRow;
  STCC_CaptionRow_t* ptr1;    /* pointer to current row */

  TestMode(Inst_p);
  if (Inst_p->CaptionFlags & PRINT_ENABLE)
  {
     if(!(Inst_p->CaptionFlags & TEXT_TRANSMISSION))
     { /* We are not in Text mode so process only if roll-up Caption */
       if (!(Inst_p->DisplayModeFlags & (ROLL_UP1 | ROLL_UP0)))
       return;
    /* Inst_p->DisplayModeFlags &=~SCROLL_ENABLED; *//* Flag to be used by OSD */
     }

        /* Go on if we are receiving Text or roll-up data inputs */

        /* Save row pointer for a while */
        i = Inst_p->BufferRowNumber;

        /* Point to the beginning of the next row "CARRIAGE RETURN" */
        Inst_p->CursorPosition = 0;
        if (Inst_p->CaptionFlags & TEXT_TRANSMISSION)
        {
          Inst_p->BufferRowNumber++;

          if(Inst_p->BufferRowNumber == 15)
          {
          Inst_p->DisplayModeFlags |= SCROLL_ENABLED; /* OSD must scroll*/
          Inst_p->BufferRowNumber =14;

          /* Roll_up the buffer rows */
          count = Inst_p->WindowSize - 1;
          TempRow = Inst_p->CaptionBuffer[1];

          for(i=0;i<count; i++)
          {
            TempRow = Inst_p->CaptionBuffer[i+1];
            Inst_p->CaptionBuffer[i] = TempRow;
          }
          ClearRows(Inst_p, count, 1, 0); /* clear bottom row */
          ptr1=&Inst_p->CaptionBuffer[count];
          ptr1->RowFlags = (ROW_ON | ROW_READY); /* enable row */
          }
          else
          {
            Inst_p->DisplayModeFlags &=~SCROLL_ENABLED;
          }

        }
        else                                /* Closed Caption mode */
        {
            count = Inst_p->WindowSize - 1;

            for(i=0;i<count; i++)
            {
                TempRow = Inst_p->CaptionBuffer[i+1];
                TempScreenRow = Inst_p->CaptionBuffer[i].ScreenRowNumber;
                Inst_p->CaptionBuffer[i] = TempRow;
                Inst_p->CaptionBuffer[i].ScreenRowNumber = TempScreenRow;
            }
        ClearRows(Inst_p, count, 1, 0); /* clear bottom row */
        Inst_p->CaptionBuffer[count].RowFlags = (ROW_ON | ROW_READY);
        }
  }
  /*Inst_p->DisplayData.DisplayEnabled |= 0x01;*/
  Inst_p->DisplayData.DisplayEnabled = TRUE;
  return;
}
/*
----------------------------------------------------------
PrintCharacter(): Prints the displayable character to the
caption buffer. Adds a leading and trailing "blank" if
necassary.
----------------------------------------------------------
*/
void PrintCharacter(STCC_InstanceData_t* Inst_p, U8 NewCharacter)
{
   STCC_CaptionChar_t* ptr1;
   STCC_CaptionRow_t* ptr2;

    /* Point to current char & row location in Caption Buffer respectively */
    ptr1 = &Inst_p->CaptionBuffer[Inst_p->BufferRowNumber].
            CharacterRow[Inst_p->CursorPosition];

    ptr2 = &Inst_p->CaptionBuffer[Inst_p->BufferRowNumber];



    /* Is the new character a transparent space */
    if(NewCharacter == 0)
    {
        /* Yes delete any character in that position */
        ptr1->Char = 0x0;
    }
    else
    {
      ptr1->Char = NewCharacter;

    }
    /* Point the next position */
  if (Inst_p->CursorPosition < 34)
    {
        Inst_p->CursorPosition++;
        ptr2->CharCount = Inst_p->CursorPosition;
    }

}
/*
----------------------------------------------------------
DeleteCharacter(): Deletes character from the current cursor
position in the display buffer.
----------------------------------------------------------
*/
void DeleteCharacter(STCC_InstanceData_t* Inst_p, STCC_CaptionChar_t* ptr1)
{
    /* Are we in closed caption mode */
    if (!(Inst_p->CaptionFlags & TEXT_TRANSMISSION))
    { /* Yes, delete character at current location */

        ptr1->Char = 0x0;

    }
}

/*
----------------------------------------------------------
ProcessBank(): This function checks if the row specified in
the PAC is already assigned. If not, it looks for a row which
is not busy. The free row is then assigned and the color
attributes are initialized.
----------------------------------------------------------
*/

void ProcessBank(STCC_InstanceData_t* Inst_p,U8 RowToProcess, U8 ScreenRow)
{
    U8 i, Temp;
    /*U8 *ptr1;*/
   STCC_CaptionRow_t* ptr2;

    /* Check if screen row has already been assigned via a prev. PAC */
    Temp = RowToProcess;
    for (i=RowToProcess; i<(RowToProcess+4); i++)
    {
        ptr2= &Inst_p->CaptionBuffer[i];
        if (ScreenRow == ptr2->ScreenRowNumber)
        {
            /* yes ScreenRow already assigned,
               a modification to current row content is required */
            Inst_p->BufferRowNumber = i;
            UpdatePAC(Inst_p);
            return;
        }
    }

    /* If the screen row has not been already defined, check if one row is
    available */
    for (i=RowToProcess; i<(RowToProcess+4); i++)
    {
        /*RowToProcess--;*/
        ptr2= &Inst_p->CaptionBuffer[i];
        if (!(ptr2->RowFlags & ROW_READY))
        {
            /* Yes, there is at least one row available */
            Inst_p->BufferRowNumber = i;
            /* Set screen row position */
            ptr2->ScreenRowNumber = ScreenRow;

            /* Initialize row attributes for the new row */
            ptr2->Background = DEFAULT_BACKGROUND;
            ptr2->RowFlags |= ROW_READY;

            UpdatePAC(Inst_p);
            return;
        }
    }
    return;
}

/*
----------------------------------------------------------
TestMode(): This function tests to see if the mode the user
specified initially matches the current data being received.
If it matches, printing characters will be allowed to be printed.
If not, printing characters will be ignored.
----------------------------------------------------------
*/
void TestMode(STCC_InstanceData_t* Inst_p)
{
    /* Disable printing as default */
    Inst_p->CaptionFlags &= ~PRINT_ENABLE;
    /* If the user has requested Text mode and we are receiving Text
    characters, then enable printing */
    if ((Inst_p->UserInputFlags & TEXT_ENABLE) && (Inst_p->CaptionFlags & TEXT_TRANSMISSION))
    {
        Inst_p->CaptionFlags |= PRINT_ENABLE;
        Inst_p->CaptionFlags |= VALID_DATA_RECEIVED;
    }
    /* If we are not in Text mode and if we are not receiving Text
    characters, then enable printing */
    if (!(Inst_p->UserInputFlags & TEXT_ENABLE) &&
       !(Inst_p->CaptionFlags & TEXT_TRANSMISSION))
        Inst_p->CaptionFlags |= PRINT_ENABLE;
}

/*
----------------------------------------------------------
SetAttribute():  This function sets the current color according
to the received code. The current color specifies the
foreground value and all of its attributes (FLASH is processed
somewhere else).
----------------------------------------------------------
*/
void SetAttribute(STCC_InstanceData_t* Inst_p)
{


    U8 i, ColorControl;
    STCC_CaptionChar_t* ptr1;

    ptr1 = &Inst_p->CaptionBuffer[Inst_p->BufferRowNumber].
            CharacterRow[Inst_p->CursorPosition];

    /* Check if FlashON code was received */
    if(Inst_p->CaptionFlags & FON_RECEIVED)
    {   /* Reset Flash bit */
        Inst_p->CaptionFlags &=~FON_RECEIVED;

        ptr1->Char = 0x20;     /* FlashON appears as a space on the screen */
        /* turn flash on for current and all remaining chars */
        for(i=Inst_p->CursorPosition; i<32;i++)
        {
            ptr1->Foreground |= FLASH_ON;
            ptr1++;
        }
        Inst_p->CursorPosition++; /* update cursor position */

        Inst_p->CaptionBuffer[Inst_p->BufferRowNumber].CharCount=
                            Inst_p->CursorPosition;
    }
    else
    {
        ptr1->Char = 0x20;     /* color codes appears as space on the screen */
        /* Get new character 2 */
        ColorControl = Inst_p->NewChar2;
        /* Check if underline is enabled */
        if(ColorControl & 0x01)
        {   /* Set or un-set underline bit */
            for(i=Inst_p->CursorPosition; i<32;i++)
            {
            ptr1->Foreground = ptr1->Foreground ^ UNDERLINE;
            ptr1++;
            }
            /* Reset ptr1 to current location */
            ptr1 = &Inst_p->CaptionBuffer[Inst_p->BufferRowNumber].
            CharacterRow[Inst_p->CursorPosition];
        }
            /* Now, keep only color and italic info */
            ColorControl = ((ColorControl >> 1) & 0x07);

            if(ColorControl == 0x07) /* check italics */
            {   /* Turn italics on and flash off, and force color to white*/
                for(i=Inst_p->CursorPosition; i<32;i++)
                {
                ptr1->Foreground ^= ITALICS;
                ptr1->Foreground &=~FLASH_ON;
                ptr1->Foreground &=~UNDERLINE;
                ptr1->Color = 0x0;
                ptr1++;
                }
            }
            else
            {
               for(i=Inst_p->CursorPosition; i<32;i++)
                {
                ptr1->Color &= ~0x07; /* clear original color */
                ptr1->Color |=ColorControl;
                ptr1->Foreground &=~FLASH_ON;
                ptr1->Foreground &=~ITALICS;
                ptr1->Foreground &=~UNDERLINE;
                ptr1++;
                }
            }
            Inst_p->CursorPosition++;
            Inst_p->CaptionBuffer[Inst_p->BufferRowNumber].CharCount =
                            Inst_p->CursorPosition;

    }

    return;



}
/*
----------------------------------------------------------
ClearRows(): This function starts at the specified StartRowNumber,
clears each char value to 0x0, resets foreground and background
attributes to defaults.
notes: See DeleteToEndOfRow()
----------------------------------------------------------
*/
void ClearRows(STCC_InstanceData_t* Inst_p,U8 StartRowNumber,U8 NumberOfRows, U8 CurrentCursorPosition)
{
    U8 i, j;                        /* Temporary storage */
    STCC_CaptionChar_t* ptr1;
    STCC_CaptionRow_t*  ptr2;

    /* Set selected number of row */
    for ( ; NumberOfRows != 0 ; NumberOfRows--)
    {
        /* Set 0 as the default value to set-up the Display Buffer */
    j = 0x20;
        /* Define ptr1 to be used with DeleteToEndOfRow() function */
        ptr1 = &Inst_p->CaptionBuffer[StartRowNumber].CharacterRow[CurrentCursorPosition];
        ptr2 = &Inst_p->CaptionBuffer[StartRowNumber];

        ptr2->RowFlags &=~(ROW_READY | ROW_ON); /* Disable the current row */
        ptr2->Background = DEFAULT_BACKGROUND;

        /* If Text mode requested by the user, set caption background as the
        default value to set-up the Caption Buffer and set related flags. The
        purpose of that is to display a black box even no Text transmission
        is available */
        if(Inst_p->UserInputFlags & TEXT_ENABLE)
        {
            ptr2->RowFlags = (ROW_ON |ROW_READY);
            j = 0x20;
        }

        /* Set to 0 char values to 0x0 and FG/BG to defaults */
    for (i=CurrentCursorPosition; i<34; i++)
        {
            ptr1->Char = j;
            ptr1->Color = DEFAULT_COLOR;
            ptr1->Foreground = DEFAULT_FOREGROUND;
            ptr1++;

        }
        /* Point next row */

        /*ptr2->ScreenRowNumber = StartRowNumber;*/
        StartRowNumber++;


    }
    /* Move cursor to the proper position */
    StartRowNumber--;
    Inst_p->BufferRowNumber = StartRowNumber;
    Inst_p->CursorPosition = CurrentCursorPosition;

}

/*
-----------------------------------------------------------
UpdatePAC():  This function updates the current foreground
attributes according to the PAC. If it is a"true" PAC, it
sets the initial row attributes and moves the cursor to the
start of the line. If it is an indent PAC, it resets the
foreground attributes and moves the cursor to the specified
CursorPosition.
-----------------------------------------------------------
*/
void UpdatePAC(STCC_InstanceData_t* Inst_p)
{
    U8 Character;
    STCC_CaptionRow_t* ptr2;

    ptr2 = &Inst_p->CaptionBuffer[Inst_p->BufferRowNumber];
    Character = Inst_p->NewChar2;
    /* Look for a possible indent PAC */
    if (Character < 0x50)
    {
        /* It is not an indent PAC, but it is an italics  */
        SetAttribute(Inst_p);

        Inst_p->CursorPosition = 0;
        ptr2->CharCount = Inst_p->CursorPosition;
        /* Initialize row attributes for the new row */

        ptr2->Background = DEFAULT_BACKGROUND;
        if(Inst_p->DisplayModeFlags & POP_ON)
        {
            ptr2->RowFlags &=0x0;   /* make sure only ready bit is set */
            ptr2->RowFlags |=ROW_READY;
        }
        else
        { /* if not pop_on, then we want characters to appear as they arrive */
            ptr2->RowFlags = (ROW_ON | ROW_READY);
        }
    }
    else
    {

        /* Keep indent offset, CursorPosition = (2*indent) + 1 */
        Inst_p->CursorPosition = ((Character & 0x0E) << 1) + 1;
        ptr2->CharCount = Inst_p->CursorPosition;

        Inst_p->NewChar2 &=0x01; /* keep underline info */
        SetAttribute(Inst_p);   /* Set default attributes except the underline */

    }
}

/*
-----------------------------------------------------------
CopyChar(): Checks parity and copies content to instance space
-----------------------------------------------------------
*/
void CopyChar(STCC_InstanceData_t* Inst_p, U8* NTSC)
{
    if(!(IsParityValid((U8)NTSC[0])))
   {
        NTSC[0] &=PARITY_RESET;
        Inst_p->NewChar1 = (U8)NTSC[0];
   }
   else
   {
        Inst_p->NewChar1 = ERROR;
   }

   if(!(IsParityValid((U8)NTSC[1])))
   {
        NTSC[1] &=PARITY_RESET;
        Inst_p->NewChar2 = (U8)NTSC[1];
   }
   else
   {
        Inst_p->NewChar2 = ERROR;
   }
}


/*
----------------------------------------------------------
SetScreenMap(): This function maps the first 4 rows of the
caption buffer into the appropraite screen rows according
to the base row defined by the PAC
----------------------------------------------------------
*/
void SetScreenMap(STCC_InstanceData_t* Inst_p, U8 BaseRow)
{

    U8 i, temp1;
    STCC_CaptionRow_t* ptr1;
    U8 NumberOfRows;

    NumberOfRows = Inst_p->WindowSize;

    temp1 = Inst_p->WindowSize;

  if(FirstRollUp == TRUE )
  {
      FirstRollUp = FALSE;
      ptr1 = &Inst_p->CaptionBuffer[(Inst_p->WindowSize - 1)];
      ptr1->RowFlags = (ROW_ON | ROW_READY);
      ptr1->ScreenRowNumber = BaseRow;
  }
  else if(FirstRollUp == FALSE)
  {
        for (i=0;i<Inst_p->WindowSize;i++)
        {
            /* Point to the proper CaptionBuffer row */
            ptr1 = &Inst_p->CaptionBuffer[i];

            ptr1->RowFlags =(ROW_ON | ROW_READY); /* Enable Row */

            ptr1->ScreenRowNumber = ((BaseRow - NumberOfRows)+1);
            NumberOfRows--;
        }


   }

    if (Inst_p->UserInputFlags & TEXT_ENABLE)
    {
        ptr1=&Inst_p->CaptionBuffer[0];
        /* Do not display the first row in Text mode */
        ptr1->RowFlags &=~(ROW_ON |ROW_READY);

        /* Point the first location of the Display Buffer */
        Inst_p->BufferRowNumber = 1;
        Inst_p->CursorPosition = 0;
    }


return;

}

/*
----------------------------------------------------------
IsParityValid(): checks if the parity is correct.

----------------------------------------------------------
*/
boolean IsParityValid(U8 character)
{
 int  number_of_ones=0;

    if(character & 0x01)
    number_of_ones++;

   if(character & 0x02)
    number_of_ones++;

    if(character & 0x04)
    number_of_ones++;

    if(character & 0x08)
    number_of_ones++;

    if(character & 0x10)
    number_of_ones++;

    if(character & 0x20)
    number_of_ones++;

    if(character & 0x40)
    number_of_ones++;

   if(character & 0x80)
    number_of_ones++;

 if((number_of_ones == 1)||(number_of_ones == 3)||(number_of_ones == 5)||
    (number_of_ones == 7))
 {
    return(FALSE);
 }
 else

 return(TRUE);

}


/*
-----------------------------------------------------------
SetXdsFlags(): This function decodes the received character
sets flags some FLAGS about XDS, and store XDS data in an array.
-----------------------------------------------------------
*/
void SetXdsFlags(STCC_InstanceData_t* Inst_p)
{

    if (!(Inst_p->XdsFlag & XDS))
    { /* first XDS data so set flags */
        switch (Inst_p->NewChar1)
        {
            case 0x01:
            {
                Inst_p->XdsFlag |= XDS; /* XDS code : Current Class Start */
                Inst_p->XdsFlag |= CURRENT_CLASS;   /* Current Class data */
                Inst_p->XdsFlag &= ~PACKET_START;   /* reset bit */
            }
            break;
            case 0x07:
            {
                Inst_p->XdsFlag |= XDS; /* XDS code : Current Class Start */
                Inst_p->XdsFlag |= MISCELLANEOUS_CLASS; /* Miscellaneous Class data */
                Inst_p->XdsFlag &= ~PACKET_START;   /* reset bit */
            }
            break;
            default :
            {
                Inst_p->XdsFlag &= ~XDS;                /* XDS code : All Class End */
                Inst_p->XdsFlag &= ~CURRENT_CLASS;      /* reset bit */
                Inst_p->XdsFlag &= ~PACKET_START;       /* reset bit */
                Inst_p->XdsFlag &= ~PROGRAM_RATING; /* reset bit */
                Inst_p->XdsFlag &= ~TOD;                /* reset bit */
                Inst_p->XdsFlag &= ~MISCELLANEOUS_CLASS;    /* reset bit */
                Inst_p->XdsFlag1 &= ~LOCAL_TIME_ZONE;   /* reset bit */
            }
            break;
        }

        if (!(Inst_p->XdsFlag & PACKET_START))
        {  /* fisrt byte received. Define now the data type */
            switch (Inst_p->NewChar2)
            {
                case 0x01:
                {
                    if (Inst_p->XdsFlag & MISCELLANEOUS_CLASS)
                    {
                        Inst_p->XdsFlag |= PACKET_START;    /* set bit */
                        Inst_p->XdsFlag |= TOD;         /* set bit */
                        Inst_p->XdsFlag &= ~TOD_VALID;      /* reset bit */
                        Inst_p->PntrToTable = &Inst_p->TimeOfTheDay[0]; /* set pointer */
                    }
                }
                break;

                case 0x04:
                {
                    if (Inst_p->XdsFlag & MISCELLANEOUS_CLASS)
                    {
                        Inst_p->XdsFlag |= PACKET_START;    /* set bit */
                        Inst_p->XdsFlag1 |= LOCAL_TIME_ZONE;    /* set bit */
                        Inst_p->XdsFlag1 &= ~LTZ_VALID;     /* reset bit */
                        Inst_p->PntrToTable = &Inst_p->LocalTimeZone[0]; /* set pointer */
                    }
                }
                break;

                case 0x05:
                {
                    if (Inst_p->XdsFlag & CURRENT_CLASS)
                    {
                        Inst_p->XdsFlag |= PACKET_START;    /* set bit */
                        Inst_p->XdsFlag |= PROGRAM_RATING;  /* set bit */
                        Inst_p->XdsFlag &= ~PR_VALID;       /* reset bit */
                        Inst_p->PntrToTable = &Inst_p->ProgRating[0]; /* set pointer */
                    }
                }
                break;

                default :
                {
                    Inst_p->XdsFlag &= ~XDS;                /* XDS code : All Class End */
                    Inst_p->XdsFlag &= ~CURRENT_CLASS;      /* reset bit */
                    Inst_p->XdsFlag &= ~PACKET_START;       /* reset bit */
                    Inst_p->XdsFlag &= ~MISCELLANEOUS_CLASS;/* reset bit */
                    Inst_p->XdsFlag &= ~PROGRAM_RATING; /* set bit */
                    Inst_p->XdsFlag &= ~TOD;                /* reset bit */
                    Inst_p->XdsFlag1 &= ~LOCAL_TIME_ZONE;   /* reset bit */
                }
                break;
            }
        }
    }
    else
    { /* an XDS data already received */
        if (Inst_p->NewChar1 == 0x0F)
        {   /* end of the XDS transmission */
            Inst_p->XdsFlag &= ~XDS;                /* XDS code : All Class End */
            Inst_p->XdsFlag &= ~CURRENT_CLASS;      /* reset bit */
            Inst_p->XdsFlag &= ~PACKET_START;       /* reset bit */
            Inst_p->XdsFlag &= ~PROGRAM_RATING; /* reset bit */
            Inst_p->XdsFlag &= ~TOD;                /* reset bit */
            Inst_p->XdsFlag &= ~MISCELLANEOUS_CLASS;    /* reset bit */
            Inst_p->XdsFlag1 &= ~LOCAL_TIME_ZONE;   /* reset bit */
        }
        else
        {
            if (Inst_p->XdsFlag & PROGRAM_RATING)
            { /* program rating data reception */
                if (Inst_p->XdsFlag1 & PR_RECEIVED_ONCE)
                {   /* PR packet already received, so check if it is the same */
                    Inst_p->XdsFlag1 &= ~PR_RECEIVED_ONCE;
                    if ((Inst_p->NewChar1 == Inst_p->ProgRating[0]) && (Inst_p->NewChar2 == Inst_p->ProgRating[1]))
                    {   /* same packet, so PR is Valid */
                        Inst_p->XdsFlag |= PR_VALID;            /* set flag for display */
                    }
                    else
                    {   /* PR packet is not the same, so proceed as if it was the 1st time */
                        Inst_p->XdsFlag1 |= PR_RECEIVED_ONCE;
                        *(Inst_p->PntrToTable++) = Inst_p->NewChar1;
                        *(Inst_p->PntrToTable) = Inst_p->NewChar2;
                    }
                }
                else
                {
                    Inst_p->XdsFlag1 |= PR_RECEIVED_ONCE;
                    *(Inst_p->PntrToTable++) = Inst_p->NewChar1;
                    *(Inst_p->PntrToTable) = Inst_p->NewChar2;
                }
                Inst_p->XdsFlag &= ~PACKET_START;       /* reset bit */
                Inst_p->XdsFlag &= ~PROGRAM_RATING; /* set bit */
            }

            if (Inst_p->XdsFlag & TOD)
            { /* time of the day data reception */
                *(Inst_p->PntrToTable++) = Inst_p->NewChar1;
                *(Inst_p->PntrToTable++) = Inst_p->NewChar2;

                if (Inst_p->PntrToTable == &Inst_p->TimeOfTheDay[5] + 1)
                {   /* all bytes received */
                    Inst_p->XdsFlag |= TOD_VALID;           /* set flag for display */
                    Inst_p->XdsFlag &= ~PACKET_START;       /* reset bit */
                    Inst_p->XdsFlag &= ~TOD;    /* set bit */
                }
            }

            if (Inst_p->XdsFlag1 & LOCAL_TIME_ZONE)
            { /* local time zone data reception */
                if (Inst_p->XdsFlag1 & LTZ_RECEIVED_ONCE)
                {   /* LTZ paquet already received, so check if it is the same */
                    Inst_p->XdsFlag1 &= ~LTZ_RECEIVED_ONCE;
                    if ((Inst_p->NewChar1 == Inst_p->LocalTimeZone[0]) && (Inst_p->NewChar2 == Inst_p->LocalTimeZone[1]))
                    {   /* same paquet, so LTZ is valid */
                        Inst_p->XdsFlag1 |= LTZ_VALID;          /* set flag for display */
                    }
                    else
                    {   /* LTZ paquet is not the same, so proceed as if it was the 1st time */
                        Inst_p->XdsFlag1 |= LTZ_RECEIVED_ONCE;
                        *(Inst_p->PntrToTable++) = Inst_p->NewChar1;
                        *(Inst_p->PntrToTable) = Inst_p->NewChar2;
                    }
                }
                else
                {
                    Inst_p->XdsFlag1 |= LTZ_RECEIVED_ONCE;
                    *(Inst_p->PntrToTable++) = Inst_p->NewChar1;
                    *(Inst_p->PntrToTable) = Inst_p->NewChar2;
                }

                Inst_p->XdsFlag &= ~PACKET_START;       /* reset bit */
                Inst_p->XdsFlag1 &= ~LOCAL_TIME_ZONE;   /* reset bit */
            }
        }
    }
}


/*
-----------------------------------------------------------
UpdateDisplayBuffer(): This function updates the content
of the DisplayBuffer with new data
-----------------------------------------------------------
*/
void UpdateDisplayBuffer(STCC_InstanceData_t* Inst_p)
{
    int i;

    for(i=0;i<16;i++)
    {
        Inst_p->DisplayData.DisplayBuffer[i] = Inst_p->CaptionBuffer[i];
        if(i < 4)
        {
            Inst_p->DisplayData.ProgRating[i] = Inst_p->ProgRating[i];
        }
        if(i <2)
        {
            Inst_p->DisplayData.LocalTimeZone[i] = Inst_p->LocalTimeZone[i];
        }
        if(i < 6)
        {
            Inst_p->DisplayData.TimeOfTheDay[i] = Inst_p->TimeOfTheDay[i];
        }
    }
}

