/*
    File:           sttxt_metrics.c
    Version:        1.00.03
    Author:         Gianluca Carcassi
    Creation date:  17.01.2001
    Description:    Teletext fonts metrics routines

    Date            Modification            Name
    ----            ------------            ----
    17.01.2001      Creation                Gianluca Carcassi
    22.02.2001      Fix Zoom factor         Gianluca Carcassi
*/

#include "gfx_tools.h" /*stgfx_init.h*/
#include "sttxt.h"


ST_ErrorCode_t GetTxtCharWidth(const STGFX_TxtFont_t* Font_p,
                               U8 Zoom, U16 Idx, U16* Width_p)
{
    if (Idx >= Font_p->NumOfChars) /*MAX_SUPPORTED_CHAR*/
      return STGFX_ERROR_NOT_CHARACTER;
      
    *Width_p = Font_p->FontWidth * Zoom;
    return ST_NO_ERROR;
}



ST_ErrorCode_t GetTxtCharAscent(const STGFX_TxtFont_t* Font_p,
                                U8 Zoom, U16 Idx, S16* Ascent_p)
{
    if (Idx >= Font_p->NumOfChars) /*MAX_SUPPORTED_CHAR*/
      return STGFX_ERROR_NOT_CHARACTER;
      
    /* TXT chars are considered to be all beneath the baseline */
    
    *Ascent_p = 0;
    return ST_NO_ERROR;
}



ST_ErrorCode_t GetTxtCharDescent(const STGFX_TxtFont_t* Font_p,
                                 U8 Zoom, U16 Idx, S16* Descent_p)
{
    if (Idx >= Font_p->NumOfChars) /*MAX_SUPPORTED_CHAR*/
      return STGFX_ERROR_NOT_CHARACTER;
      
    /* TXT chars are considered to be all beneath the baseline */
    
    *Descent_p = Font_p->FontHeight * Zoom;
    return ST_NO_ERROR;
}



ST_ErrorCode_t GetTxtFontAscent(const STGFX_TxtFont_t* Font_p,
                                U8 Zoom, S16* Ascent_p)
{
    /* TXT chars are considered to be all beneath the baseline */
    
    *Ascent_p = 0;
    return ST_NO_ERROR;
}



ST_ErrorCode_t GetTxtFontHeight(const STGFX_TxtFont_t* Font_p,
                                U8 Zoom, U16* Height_p)
{
    *Height_p = Font_p->FontHeight * Zoom;
    return ST_NO_ERROR;
}



ST_ErrorCode_t GetTxtPrintableChars(const STGFX_TxtFont_t* Font_p,
                                    const STGFX_CharMapping_t* CharMapping_p,
                                    const ST_String_t String,
                                    const U16* WString,
                                    U8   Zoom,
                                    U32  AvailableLength,
                                    U32* NumOfChars_p)
{
    U32 value = 0;
    U16 increment = Font_p->FontWidth * Zoom;
    U16 c;
    U32 i;
    
    for(i=0; 1 /* breaks below */; i++)
    {
        if(String != NULL)
        {
          /* narrow-character string */
          if(!String[i]) break;
          c = (U16)(U8) String[i];
        }
        else
        {
          /* wide-character string */
          if(!WString[i]) break;
          c = WString[i];
        }

        if (CharMapping_p != NULL)
        {
          if(ST_NO_ERROR != stgfx_CharCodeToIdx(CharMapping_p,c,&c))
          {
            return STGFX_ERROR_NOT_CHARACTER;
          }
        }

        if ( c >= Font_p->NumOfChars ) /*MAX_SUPPORTED_CHAR*/
        {
            *NumOfChars_p = 0;
            return STGFX_ERROR_NOT_CHARACTER;
        }
        if ((value += increment) > AvailableLength) break;
    }

    *NumOfChars_p = i; /* chars before the one that exceeded AvailableLength */
    return ST_NO_ERROR;
}



ST_ErrorCode_t GetTxtTextWidth(const STGFX_TxtFont_t* Font_p,
                               const STGFX_CharMapping_t* CharMapping_p,
                               const ST_String_t String,
                               const U16* WString,
                               U8 Zoom,
                               U32* Width_p)
{
    U16 c;
    U32 i;
    
    for(i=0; 1 /* breaks below */; i++)
    {
        if(String != NULL)
        {
          /* narrow-character string */
          if(!String[i]) break;
          c = (U16)(U8) String[i];
        }
        else
        {
          /* wide-character string */
          if(!WString[i]) break;
          c = WString[i];
        }

        if (CharMapping_p != NULL)
        {
          if(ST_NO_ERROR != stgfx_CharCodeToIdx(CharMapping_p,c,&c))
          {
            return STGFX_ERROR_NOT_CHARACTER;
          }
        }

        if ( c >= Font_p->NumOfChars ) /*MAX_SUPPORTED_CHAR*/
        {
            *Width_p = 0;
            return STGFX_ERROR_NOT_CHARACTER;
        }
    }

    /* -> i: number of chars before terminating 0 */
    *Width_p = i * Font_p->FontWidth * Zoom;
    return ST_NO_ERROR;
}



ST_ErrorCode_t GetTxtTextHeight(const STGFX_TxtFont_t* Font_p,
                                const STGFX_CharMapping_t* CharMapping_p,
                                const ST_String_t String,
                                const U16* WString,
                                U8 Zoom,
                                U32* Height_p)
{
    U16 c;
    U32 i;
    
    for(i=0; 1 /* breaks below */; i++)
    {
        if(String != NULL)
        {
          /* narrow-character string */
          if(!String[i]) break;
          c = (U16)(U8) String[i];
        }
        else
        {
          /* wide-character string */
          if(!WString[i]) break;
          c = WString[i];
        }

        if (CharMapping_p != NULL)
        {
          if(ST_NO_ERROR != stgfx_CharCodeToIdx(CharMapping_p,c,&c))
          {
            return STGFX_ERROR_NOT_CHARACTER;
          }
        }

        if ( c >= Font_p->NumOfChars ) /*MAX_SUPPORTED_CHAR*/
        {
            *Height_p = 0;
            return STGFX_ERROR_NOT_CHARACTER;
        }
    }

    *Height_p = Font_p->FontHeight * Zoom;
    return ST_NO_ERROR;
}

/* end of sttxt_metrics.c */
