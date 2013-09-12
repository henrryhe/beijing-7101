/*******************************************************************************

File name   : parsers.c

Description :
- STCC_Detect
- STCC_ParseEIA708
- STCC_ParseDTVvid21
- STCC_ParseUdata130
- STCC_ParseDVS157

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
25 June 2001       Created                                     Michel Bruant
17 Jan  2002       Add robustness over array indexes                 HS
15 May  2002       Parsing under CC task context. Debug 608    Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
/* Include system files only if not in Kernel mode */
#include <string.h>
#endif

#include "stddefs.h"
#include "stvid.h"
#include "stccinit.h"
#include "parsers.h"
#include "ccslots.h"

/* Constants ---------------------------------------------------------------- */

enum
{
    FORBIDDEN,
    RESERVED_1,
    PRESENTATION_TIME_STAMP,
    RESERVED_3,
    DECODE_TIME_STAMP,
    RESERVED_5,
    PAN_AND_SCAN,
    FIELD_DISPLAY_FLAGS,
    RESERVED_8,
    CLOSED_CAPTION_DTV,
    EXTENDED_DATA_SERVICES,
    ESC_TO_EXTENDED_DATA_SERVICES = 0xff
};

enum
{
    FORBIDDEN_0,
    SA_CUE_TRIGGER,
    REESSERVED_2,
    CLOSED_CAPTION_EIA,
    ADDITIONAL_EIA_608,
    LUMA_PAM
};

#define ATSC_IDENTIFIER     0x47413934
#define SA_IDENTIFIER       0x53415544

#define PROCESS_CC_DATA     (1<<6)
#define ADDITIONAL_DATA     (1<<5)
#define CC_COUNT            (31<<0)
#define CC_TYPE             (3<<0)
#define CC_VALID            (1<<2)

/* Private functions -------------------------------------------------------- */

static U32  GetBits ( U32 NbRequestedBits, U8* DataBuff_p, BOOL Reset);
static U8   ReverseBits (U8 Data);

/*------------------------------------------------------------------------------
Name:   STCC_Detect
Purpose:
Notes:
------------------------------------------------------------------------------
*/
STCC_FormatMode_t STCC_Detect(stcc_Device_t * const Device_p, U32 i)
{
    U8 *            DataBuff_p;
    U32             Identifier;
    U32             Index;
    U8              Length;

    DataBuff_p  = Device_p->DeviceData_p->Copies[i].Data;

    if (Device_p->DeviceData_p->Copies[i].Length < 4)
    {
        /* default value */
        CC_Trace("User Data No Detected ");
        return(STCC_FORMAT_MODE_DETECT);
    }
    
    Identifier  = DataBuff_p[0] << 24;
    Identifier |= DataBuff_p[1] << 16;
    Identifier |= DataBuff_p[2] <<  8;
    Identifier |= DataBuff_p[3];

    /* Detect EIA-708 */
    if(Identifier == ATSC_IDENTIFIER)
    {
        CC_Trace("User Data Detected : EIA-708");
        return(STCC_FORMAT_MODE_EIA708);
    }
    
    /* Detect UDATA-130 */
    if(Identifier == SA_IDENTIFIER)
    {
        CC_Trace("User Data Detected : Udata130");
        return(STCC_FORMAT_MODE_UDATA130);
    }
    
    /* Detect EIA-608 (=DVS-157) */
    if((DataBuff_p[0]== CLOSED_CAPTION_EIA)
        &&(GetBits(7,&DataBuff_p[1],TRUE ) == 0x0)
        &&(GetBits(1,&DataBuff_p[1],FALSE) == 0x1))
    {
        CC_Trace("User Data Detected : EIA-608");
        return(STCC_FORMAT_MODE_EIA608);
    }
    
    /* Detect DTV-Vid-21 */
    Index = 0;                                          /* point to length */
    while (((Index + 3) < Device_p->DeviceData_p->Copies[i].Length) 
            && (DataBuff_p[Index] != 0)
            && (DataBuff_p[Index+1] != FORBIDDEN))
    {
        Length = DataBuff_p[Index];
        Length--; /* direcTV counts the data type in length */
        Index ++;                                       /* point to data type */
        switch(DataBuff_p[Index])
        {
            case CLOSED_CAPTION_DTV:
            case EXTENDED_DATA_SERVICES:
                CC_Trace("User Data Detected : DTVVID-21");
                return(STCC_FORMAT_MODE_DTVVID21);
            default:
                /* skip the data */
                Index += (Length);
                break;
        } /* switch */
        Index ++;                                     /* point to next length */
    }/* while */
  
    /* Detect DVB */
    Index = 0;                                          /* point to length */
    while (((Index + 3) < Device_p->DeviceData_p->Copies[i].Length) 
            && (DataBuff_p[Index] != 0)
            && (DataBuff_p[Index+1] != FORBIDDEN))
    {
        Length = DataBuff_p[Index];
        Index ++;                                       /* point to data type */
        switch(DataBuff_p[Index])
        {
            case CLOSED_CAPTION_DTV:
            case EXTENDED_DATA_SERVICES:
                CC_Trace("User Data Detected : DVB");
                return(STCC_FORMAT_MODE_DVB);
            default:
                /* skip the data */
                Index += (Length);
                break;
        } /* switch */
        Index ++;                                     /* point to next length */
    }/* while */
  
    /* default value */
    CC_Trace("User Data No Detected ");
    return(STCC_FORMAT_MODE_DETECT);
}

/*------------------------------------------------------------------------------
Name:    STCC_ParseEIA708
Purpose: EIA-708; ATSC
Notes:
------------------------------------------------------------------------------
*/
void STCC_ParseEIA708(stcc_Device_t * const Device_p,U32 i)
{
    U32             SlotIndex;
    U8 *            DataBuff_p;
    U32             Index;
    U32             j;
    U8              process_cc_data,additional_data,cc_count;
    U8              cc_valid,cc_type;
    stcc_Slot_t *   CurrentSlot_p;
    U32             Identifier;

    DataBuff_p  = Device_p->DeviceData_p->Copies[i].Data;
    if (Device_p->DeviceData_p->Copies[i].Length < 5)
    {
        return; /* just return, something is wrong */
    }
    Index = 0;                                         /* point to identifier */
    Identifier  = DataBuff_p[Index] << 24; Index ++;
    Identifier |= DataBuff_p[Index] << 16; Index ++;
    Identifier |= DataBuff_p[Index] <<  8; Index ++;
    Identifier |= DataBuff_p[Index];
    if(Identifier != ATSC_IDENTIFIER)
    {
        return; /* just return, something is wrong */
    }
    Index ++;                                           /* point to data type */
    if(DataBuff_p[Index] == CLOSED_CAPTION_EIA)
    {
        if (Device_p->DeviceData_p->Copies[i].Length < 8)
        {
            return; /* just return, something is wrong */
        }
        Index ++;                                           /* point to flags */
        process_cc_data  = DataBuff_p[Index] & PROCESS_CC_DATA;
        additional_data  = DataBuff_p[Index] & ADDITIONAL_DATA;
        cc_count         = DataBuff_p[Index] & CC_COUNT;
        Index ++;                                         /* point to em_data */
        Index ++;                                         /* point to marker  */
        if (process_cc_data == PROCESS_CC_DATA)
        {
            SlotIndex = stcc_GetSlotIndex(Device_p);
            CurrentSlot_p = &(Device_p->DeviceData_p->CaptionSlot[SlotIndex]);
            CurrentSlot_p->DataLength   = 0;
            CurrentSlot_p->PTS          = Device_p->DeviceData_p->Copies[i].PTS;
            /* Process cc data */
            for (j=0;
                   (j < cc_count) 
                   && ((Index + 2) < Device_p->DeviceData_p->Copies[i].Length); 
                 j++)
            {
                cc_valid = DataBuff_p[Index] & CC_VALID;
                cc_type  = DataBuff_p[Index] & CC_TYPE;

                if(cc_valid == CC_VALID)/* cases: valid just send field=type */
                {
                    CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Field 
                                        = cc_type;
                    Index ++;                               /* point to data1 */
                    CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte1
                                        = DataBuff_p[Index];
                    Index ++;                               /* point to data2 */
                    CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte2 
                                        = DataBuff_p[Index];
                    CurrentSlot_p->DataLength++;
                    if(CurrentSlot_p->DataLength == NB_CCDATA)
                    { 
                        CurrentSlot_p->DataLength = (NB_CCDATA - 1);
                        CC_Trace("Closed caption : slot full !!");
                    }
                    /* Remember if we send a 708 */
                    if(cc_type == 2) 
                    {
                        Device_p->DeviceData_p->LastPacket708 = TRUE;
                    }
                    else
                    {
                        Device_p->DeviceData_p->LastPacket708 = FALSE;
                    }
                }
                else /* cases valid = 0: cc could be valid anyway in 708 */
                {
                    if((Device_p->DeviceData_p->LastPacket708)
                    &&((cc_type == 2)||(cc_type == 3)))
                    {
                        /* Force packet start */
                        CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Field 
                                            = 0x02;     
                        Index ++;                           /* point to data1 */
                        CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte1
                                            = DataBuff_p[Index];
                        Index ++;                           /* point to data2 */
                        CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte2 
                                            = DataBuff_p[Index];
                        CurrentSlot_p->DataLength++;
                        if(CurrentSlot_p->DataLength == NB_CCDATA)
                        { 
                            CurrentSlot_p->DataLength = (NB_CCDATA - 1);
                            CC_Trace("Closed caption : slot full !!");
                        }
                    }
                    else
                    {
                        /* skip */
                        Index ++;                          /* point to data1 */
                        Index ++;                          /* point to data2 */
                    }
                    Device_p->DeviceData_p->LastPacket708 = FALSE; 
                } /* end valid = 0 */
                Index ++;                                 /* point to marker  */
            } /* end for cc_count , reloop */
            if(CurrentSlot_p->DataLength != 0)
            {
                stcc_InsertSlot(Device_p,SlotIndex);
            }
            else
            {
                stcc_ReleaseSlot(Device_p,SlotIndex);
            }
                
        } /* end if process cc_data */
        if (additional_data == ADDITIONAL_DATA)
        {
            /* to be done : additional user data */
        }
    }
}

/*------------------------------------------------------------------------------
Name:    STCC_ParseDtvVid21()
Purpose: This function Parses user data looking for closed caption + XDS
Notes:
------------------------------------------------------------------------------
*/
void STCC_ParseDTVvid21(stcc_Device_t * const Device_p,U32 i,BOOL FlagDirecTV)
{
    U32                 SlotIndex;
    U32                 Index;
    U8                  Length;
    stcc_Slot_t         * CurrentSlot_p;
    U8 *                DataBuff_p;

    DataBuff_p = Device_p->DeviceData_p->Copies[i].Data;
    Index = 0;                                          /* point to length */
    SlotIndex = stcc_GetSlotIndex(Device_p);
    CurrentSlot_p=&(Device_p->DeviceData_p->CaptionSlot[SlotIndex]);
    CurrentSlot_p->DataLength   = 0;
    CurrentSlot_p->PTS          = Device_p->DeviceData_p->Copies[i].PTS;
    while (((Index + 3) < Device_p->DeviceData_p->Copies[i].Length) 
            && (DataBuff_p[Index] != 0)
            && (DataBuff_p[Index+1] != FORBIDDEN))
    {
        Length = DataBuff_p[Index];
        if (FlagDirecTV)
        {
            Length--; /* direcTV counts the data type in length */
        }
        Index ++;                                       /* point to data type */
        switch(DataBuff_p[Index])
        {
            case CLOSED_CAPTION_DTV:
            case EXTENDED_DATA_SERVICES:
                if(DataBuff_p[Index] == CLOSED_CAPTION_DTV)
                {
                    CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Field = 0;
                }
                else /* DataBuff_p[Index] == EXTENDED_DATA_SERVICES*/
                {
                    CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Field = 1;
                }
                Index ++;
                CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte1 
                                                        = DataBuff_p[Index];
                Index ++;
                CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte2 
                                                        = DataBuff_p[Index];
                CurrentSlot_p->DataLength++;
                if(CurrentSlot_p->DataLength >= NB_CCDATA)
                {
                    CurrentSlot_p->DataLength = NB_CCDATA - 1;
                    CC_Trace("Closed caption : slot full !!");
                }
                break;
            default:
                /* skip the data */
                Index += (Length);
                break;
        } /* switch */
        Index ++;                                     /* point to next length */
    }/* while */
    if(CurrentSlot_p->DataLength != 0)
    {
        stcc_InsertSlot(Device_p,SlotIndex);
    }
    else
    {
        stcc_ReleaseSlot(Device_p,SlotIndex);
    }
    
    return;
}
/*------------------------------------------------------------------------------
Name:    STCC_ParseUdata130
Purpose:
Notes:
------------------------------------------------------------------------------
*/
void STCC_ParseUdata130(stcc_Device_t * const Device_p,U32 i)
{
    U32             SlotIndex;
    U8 *            DataBuff_p;
    U32             Index;
    U32             j;
    U8              process_cc_data,additional_data,cc_count;
    U8              cc_valid,cc_type;
    stcc_Slot_t *   CurrentSlot_p;
    U32             Identifier;

    DataBuff_p  = Device_p->DeviceData_p->Copies[i].Data;
    Index       = 0;                                   /* point to identifier */
    if (Device_p->DeviceData_p->Copies[i].Length < 5)
    {
        return; /* just return, something is wrong */
    }
    Index = 0;                                         /* point to identifier */
    Identifier  = DataBuff_p[Index] << 24; Index ++;
    Identifier |= DataBuff_p[Index] << 16; Index ++;
    Identifier |= DataBuff_p[Index] <<  8; Index ++;
    Identifier |= DataBuff_p[Index];
    if(Identifier!= SA_IDENTIFIER)
    {
        return; /* just return, something is wrong */
    }
    Index ++;                                           /* point to data type */
    if(DataBuff_p[Index] == SA_CUE_TRIGGER)
    {
        /* no closed caption */
    }
    else if(DataBuff_p[Index] == CLOSED_CAPTION_EIA)
    {
        if (Device_p->DeviceData_p->Copies[i].Length < 8)
        {
            return; /* just return, something is wrong */
        }
        Index ++;                                           /* point to flags */
        process_cc_data  = DataBuff_p[Index] & PROCESS_CC_DATA;
        additional_data  = DataBuff_p[Index] & ADDITIONAL_DATA;
        cc_count         = DataBuff_p[Index] & CC_COUNT;
        Index ++;                                         /* point to em_data */
        Index ++;                                         /* point to cc_data */
        if (process_cc_data == PROCESS_CC_DATA)
        {
            SlotIndex = stcc_GetSlotIndex(Device_p);
            CurrentSlot_p = &(Device_p->DeviceData_p->CaptionSlot[SlotIndex]);
            CurrentSlot_p->DataLength   = 0;
            CurrentSlot_p->PTS          = Device_p->DeviceData_p->Copies[i].PTS;
            /* Process cc data */
            for (j=0; (j < cc_count) 
                    && ((Index + 2) < Device_p->DeviceData_p->Copies[i].Length);
                    j++)
            {
                cc_valid = DataBuff_p[Index] & CC_VALID;
                cc_type  = DataBuff_p[Index] & CC_TYPE;
                if(cc_valid != CC_VALID)
                {
                    /* skip */
                    Index ++;
                    Index ++;
                }
                else
                {
                    CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Field 
                                           = cc_type;
                    Index ++;                              /* point to data1 */
                    CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte1
                                           = DataBuff_p[Index];
                    Index ++;                              /* point to data2 */
                    CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte2 
                                              = DataBuff_p[Index];
                    CurrentSlot_p->DataLength++;
                    if(CurrentSlot_p->DataLength > 50)
                    {
                        CC_Trace("Slot overflow !!");
                    }
                }
                Index ++;
            } /* end for cc_count */
            if(CurrentSlot_p->DataLength != 0)
            {
                stcc_InsertSlot(Device_p,SlotIndex);
            }
            else
            {
                stcc_ReleaseSlot(Device_p,SlotIndex);
            }
        } /* end if process cc_data */
        if (additional_data == ADDITIONAL_DATA)
        {
            /* to be done : additional user data */
        }
    }
    else if(DataBuff_p[Index] == ADDITIONAL_EIA_608)
    {
        /* to be done : additional eia608 data */
    }
    else if(DataBuff_p[Index] == LUMA_PAM)
    {
        /* no closed caption */
    }
    else
    {
        /* to be done : additional SA user data */
    }
}

/*------------------------------------------------------------------------------
Name:    STCC_ParseDVS157
Purpose: = EIA-608
Notes:
------------------------------------------------------------------------------
*/
void STCC_ParseDVS157(stcc_Device_t * const Device_p,U32 i)
{
    U32             SlotIndex;
    U8 *            DataBuff_p;
    U32             j;
    stcc_Slot_t *   CurrentSlot_p;
    U32             vbi_data_flag,cc_count,user_data_type;
    U32             cc_prio,field_number,line_offset;
    U8              cc_data1,cc_data2;

    DataBuff_p  = Device_p->DeviceData_p->Copies[i].Data;
    if (Device_p->DeviceData_p->Copies[i].Length < 5)
    {
        return; /* just return, something is wrong */
    }

    user_data_type = GetBits(8, DataBuff_p, TRUE);
    if(user_data_type == CLOSED_CAPTION_EIA)
    {
                          GetBits(7, DataBuff_p, FALSE);
        vbi_data_flag   = GetBits(1, DataBuff_p, FALSE);
        if(vbi_data_flag == 1)
        {
            cc_count        = GetBits(5, DataBuff_p, FALSE);
            SlotIndex       = stcc_GetSlotIndex(Device_p);
            CurrentSlot_p   = &(Device_p->DeviceData_p->CaptionSlot[SlotIndex]);
            CurrentSlot_p->DataLength   = 0;
            CurrentSlot_p->PTS          = Device_p->DeviceData_p->Copies[i].PTS;
            for(j=0; j<cc_count; j++)
            {
                cc_prio                     = GetBits(2, DataBuff_p, FALSE);
                field_number                = GetBits(2, DataBuff_p, FALSE) -1;
                line_offset                 = GetBits(5, DataBuff_p, FALSE);
                cc_data1                    = GetBits(8, DataBuff_p, FALSE);
                cc_data2                    = GetBits(8, DataBuff_p, FALSE);
                /* marker */                  GetBits(1, DataBuff_p, FALSE);
                CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Field 
                                            = field_number;
                CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte1
                                            = ReverseBits(cc_data1);
                CurrentSlot_p->CC_Data[CurrentSlot_p->DataLength].Byte2
                                            = ReverseBits(cc_data2);
                CurrentSlot_p->DataLength++;
                if(CurrentSlot_p->DataLength > 50)
                {
                    CC_Trace("Slot overflow !!");
                }
            }
            if(CurrentSlot_p->DataLength != 0)
            {
                stcc_InsertSlot(Device_p,SlotIndex);
            }
            else
            {
                stcc_ReleaseSlot(Device_p,SlotIndex);
            }

        } /* end if process cc data */
    } /* end if user data == cc data */
}

/*------------------------------------------------------------------------------
Name:     GetBits
Purpose: returns nb first bits of the CC blocks, and shift
Notes: private
------------------------------------------------------------------------------
*/
U32 GetBits ( U32 NbRequestedBits, U8* DataBuff_p, BOOL Reset)
{
    U32 RequestedBits;
    static struct
    {
        U32 Value;
        U32 NumBits;
        U8 * DataPointer_p;
    }BitWindows;

    /* Reset the the bits window */
    if (Reset == TRUE)
    {
        BitWindows.Value            = 0;
        BitWindows.NumBits          = 0;
        BitWindows.DataPointer_p    = DataBuff_p;
    }

    /* Take bits in the bits window */
    RequestedBits  =  BitWindows.Value >> (32 - NbRequestedBits);
    if (NbRequestedBits <= BitWindows.NumBits)
    {
        /* If enough bits, return the bits */
        BitWindows.Value     = BitWindows.Value << (NbRequestedBits);
        /* and update the bits windows */
        BitWindows.NumBits  -= NbRequestedBits;
        return(RequestedBits);
    }
    /* If not enough bits, re-fill the bits window */
    NbRequestedBits            -= BitWindows.NumBits;
    BitWindows.NumBits          = 32;
    BitWindows.Value            = (*BitWindows.DataPointer_p ++) << 24;
    BitWindows.Value           |= (*BitWindows.DataPointer_p ++) << 16;
    BitWindows.Value           |= (*BitWindows.DataPointer_p ++) <<  8;
    BitWindows.Value           |= (*BitWindows.DataPointer_p ++) <<  0;
    RequestedBits              |= (BitWindows.Value >> (32-NbRequestedBits));
    BitWindows.Value            = BitWindows.Value << (NbRequestedBits);
    BitWindows.NumBits         -= NbRequestedBits;
    return(RequestedBits);
}

/*------------------------------------------------------------------------------
Name:     ReverseBits
Purpose:
Notes: private
------------------------------------------------------------------------------
*/
static U8  ReverseBits (U8 Data)
{
    U32 b0,b1,b2,b3,b4,b5,b6,b7;
    U8  Reversed;

    b0 = (Data >> 0) & 0x1 ;
    b1 = (Data >> 1) & 0x1 ;
    b2 = (Data >> 2) & 0x1 ;
    b3 = (Data >> 3) & 0x1 ;
    b4 = (Data >> 4) & 0x1 ;
    b5 = (Data >> 5) & 0x1 ;
    b6 = (Data >> 6) & 0x1 ;
    b7 = (Data >> 7) & 0x1 ;
    Reversed = (b7 << 0)
             | (b6 << 1)
             | (b5 << 2)
             | (b4 << 3)
             | (b3 << 4)
             | (b2 << 5)
             | (b1 << 6)
             | (b0 << 7);
    return(Reversed);
}   

