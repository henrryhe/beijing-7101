/*****************************************************************************

File name   : tt_ttx.c

Description : Testtool Teletext Commands

COPYRIGHT (C) STMicroelectronics 1999.

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include <stdio.h>
#include <debug.h>

#include "stddefs.h"
#include "stlite.h"
#include "stdevice.h"

#include "stttx.h"
#include "sttbx.h"
#include "testtool.h"

/*#include "app_data.h"
#include "tt_api.h"
#include "tt_ttx.h"*/


/*#define ALE_DEBUG 1*/

#ifdef ALE_DEBUG
#define ALE_BUFF_SIZE   100000
    static char alebuf0[ALE_BUFF_SIZE];
    static char *alebuf = alebuf0;
    static BOOL AlePrintEnabled = TRUE;
    #define ALE_PRINT_ENABLE  AlePrintEnabled = TRUE
    #define ALE_PRINT_DISABLE AlePrintEnabled = FALSE
    #define ALE_Print(s,X) {if (AlePrintEnabled) {char s[128];sprintf X;if (((U32)alebuf - (U32)alebuf0) < (ALE_BUFF_SIZE - 1000)) {memcpy(alebuf,s,strlen(s));alebuf += strlen(s);}}}
#else
    #define ALE_Print(s,X)
    #define ALE_PRINT_ENABLE
    #define ALE_PRINT_DISABLE
#endif

/* external function prototype -------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

STTTX_Handle_t          TTXVBIHandle;
STTTX_Handle_t          TTXSTBHandle;
STTTX_Handle_t          TTXHandle;
ST_DeviceName_t         TTX_DeviceName = "STTTX";
U32 TTXMode = 0;
static U32 API_ErrorCount = 0;
static U32 API_EnableError = 0;
extern partition_t *ncache_partition;

/* Local Variables -------------------------------------------------------- */

/* PTI slots for teletext data */
#if !defined(TTXSTPTI_TEST)
slot_t vbi_slot = 0;
slot_t stb_slot = 0;
#else
extern STPTI_Slot_t        STBSlot;
#endif



/* Private types/constants ------------------------------------------------ */

/* Functions -------------------------------------------------------------- */
/****************************************************************************
Name         : AleCaptureEnd

Description  :

Parameters   :

Return Value : 0        success
               -1       failure (couldn't open file)

See Also     : Nothing.
****************************************************************************/
#ifdef ALE_DEBUG
static S32 AleCaptureEnd(void)
{
    S32 file, written;
    U32 count;

    count = (U32)((U32)alebuf - (U32)alebuf0);

    /* Write the thing out :) */
    file = (S32)debugopen("AleDecoder.txt", "wt");
    if (file != -1)
    {
        debugmessage("Writing ALE file\n");
        written = (S32)debugwrite(file, alebuf0, count);
        if (written != count)
        {
            debugmessage("Error writing to file\n");
        }
        debugflush(file);
        debugclose(file);
        debugmessage("Done\n");
    }
    else
    {
        debugmessage("Error opening file\n");
        return -1;
    }

    return 0;
}
#endif


/*-------------------------------------------------------------------------
 * Function : GetErrorText
 *            Return  error descriptions (wrapper)
 * Input    : Error Code
 * Output   :
 * Return   : pointer to error description
 * ----------------------------------------------------------------------*/
char *GetErrorText( ST_ErrorCode_t Error )
{
    int          Driver_Id;
    char         *ErrorTxt_p;
    static char  ErrorText[40];

    sprintf (ErrorText,"%u",Error);

    return (ErrorText);
}


/*-------------------------------------------------------------------------
 * Function : TTTTX_Close
 *            Close teletext driver
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_Close( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;

    if (( ErrCode = TTX_Close()) != ST_NO_ERROR)
        API_ErrorCount++;

    #ifdef ALE_DEBUG
        ALE_PRINT_DISABLE;
        AleCaptureEnd();
        alebuf = alebuf0;
    #endif

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTTX_Close */

/*-------------------------------------------------------------------------
 * Function : TTTTX_Init
 *            Initialise teletext
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_Init( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;

    if (( ErrCode = TTX_Init()) != ST_NO_ERROR)
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTTX_Init */

/*-------------------------------------------------------------------------
 * Function : TTTTX_Open
 *            Open teletext driver
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_Open( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;

    if (( ErrCode = TTX_Open()) != ST_NO_ERROR)
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTTX_Open */

/*-------------------------------------------------------------------------
 * Function : TTTTX_SetStreamId
 *            Set teletext Stream Id
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_SetStreamId( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;
    U32                     Pid;

    /* get boolean flag to force termination */
    cget_integer( pars_p, 0, (long *)&Pid );

    if (( ErrCode = TTX_SetStreamId( Pid )) != ST_NO_ERROR)
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTTX_SetStreamId */

/*-------------------------------------------------------------------------
 * Function : TTTTX_Start
 *            Start teletext
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_Start( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;

    if (( ErrCode = TTX_Start()) != ST_NO_ERROR)
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTTX_Start */

/*-------------------------------------------------------------------------
 * Function : TTTTX_Stop
 *            Stop teletext
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_Stop( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;

    if (( ErrCode = TTX_Stop()) != ST_NO_ERROR)
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTTX_Stop */

/*-------------------------------------------------------------------------
 * Function : TTTTX_Term
 *            Term teletext driver
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_Term( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    Temp;

    /* get boolean flag to force termination */
    cget_integer( pars_p, FALSE, (long *)&Temp );

    if (( ErrCode = TTX_Term( Temp )) != ST_NO_ERROR)
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTTX_Term */


/*-------------------------------------------------------------------------
 * Function : TTX_Close
 *            Close teletext driver
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TTX_Close( void )
{
    ST_ErrorCode_t          ErrCode;

    if (TTXMode == 0)
        ErrCode = STTTX_Close(TTXVBIHandle);
    else
        ErrCode = STTTX_Close(TTXSTBHandle);

    STTBX_Print(("STTTX_Close()=%s\n", GetErrorText(ErrCode) ));

    return ( ErrCode );

} /* end TTX_Close */

/*-------------------------------------------------------------------------
 * Function : TTX_Init
 *            Initialise teletext
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TTX_Init( void )
{
    ST_ErrorCode_t          ErrCode;
    STTTX_InitParams_t      InitParams_s;

    #if !defined(TTXSTPTI_TEST)
    if ( vbi_slot == 0 )
        pti_allocate_dynamic_slot(&vbi_slot);
    if ( stb_slot == 0 )
        pti_allocate_dynamic_slot(&stb_slot);
    #endif

    /*InitParams_s.DeviceType = STTTX_DEVICE_OUTPUT*/
    InitParams_s.DriverPartition = ncache_partition;  /* the memory area should not be cached */
    InitParams_s.BaseAddress = (U32*)TTXT_BASE_ADDRESS;
    InitParams_s.InterruptNumber = TELETEXT_INTERRUPT;
    InitParams_s.InterruptLevel = TTXT_INTERRUPT_LEVEL;
    InitParams_s.NumVBIObjects = 1;                 /* assumed to be one */
    InitParams_s.NumVBIBuffers = 1;                 /* not used by driver */
    InitParams_s.NumSTBObjects = 1;
    InitParams_s.NumRequestsAllowed = 4;
    strcpy( InitParams_s.EVTDeviceName, "STEVT" );

    ErrCode = STTTX_Init( TTX_DeviceName, &InitParams_s );

    STTBX_Print(("STTTX_Init(%s)=%s\n", TTX_DeviceName, GetErrorText(ErrCode)));

    return ( ErrCode );

} /* end TTX_Init */

/*-------------------------------------------------------------------------
 * Function : TTX_Open
 *            Open teletext driver
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TTX_Open( void )
{
    ST_ErrorCode_t          ErrCode;
    STTTX_OpenParams_t      OpenParams_s;
    STTTX_SourceParams_t    SourceParams;
    extern STPTI_Signal_t   STBSignal;

    if (TTXMode == 0)
    {
        OpenParams_s.Type = STTTX_VBI;
        #if !defined(TTXSTPTI_TEST)
        OpenParams_s.Slot = vbi_slot;
        #endif

        ErrCode = STTTX_Open( TTX_DeviceName, &OpenParams_s, &TTXVBIHandle );

        STTBX_Print(("STTTX_Open(VBI)=%s\n", GetErrorText(ErrCode)));
    }
    else
    {
        OpenParams_s.Type = STTTX_STB;
        #if !defined(TTXSTPTI_TEST)
        OpenParams_s.Slot = stb_slot;
        #endif
        ErrCode = STTTX_Open( TTX_DeviceName, &OpenParams_s, &TTXSTBHandle );

        STTBX_Print(("STTTX_Open(STB)=%s\n", GetErrorText(ErrCode)));

        /* Set the source */
        SourceParams.DataSource = STTTX_SOURCE_STPTI_SIGNAL;
        SourceParams.Source_u.STPTISignal_s.Signal = STBSignal;
        ErrCode = STTTX_SetSource(TTXSTBHandle, &SourceParams);
        STTBX_Print(("STTTX_SetSource(STB)=%s\n", GetErrorText(ErrCode)));
    }

    return ( ErrCode );

} /* end TTX_Open */

/*-------------------------------------------------------------------------
 * Function : TTX_SetStreamId
 *            Set teletext Stream Id
 * Input    : Teletext Pid
 * Output   :
 * Return   : Error code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TTX_SetStreamId( U32 Pid )
{
    ST_ErrorCode_t          ErrCode;

    if (TTXMode == 0)
        ErrCode = STTTX_SetStreamID( TTXVBIHandle, Pid );
    else
    {
        #if defined(TTXSTPTI_TEST)
            ErrCode = STPTI_SlotSetPid(STBSlot, Pid);
        #else
            ErrCode = STTTX_SetStreamID( TTXSTBHandle, Pid );
        #endif
    }

    #if defined(TTXSTPTI_TEST)
        STTBX_Print(("STPTI_SlotSetPid(%d)=%s\n", Pid, GetErrorText(ErrCode)));
    #else
        STTBX_Print(("STTTX_SetStreamId(%d)=%s\n", Pid, GetErrorText(ErrCode)));
    #endif

    return ( ErrCode );

} /* end TTX_Stream_Id */

/*-------------------------------------------------------------------------
 * Function : TTX_Start
 *            Start teletext
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TTX_Start( void )
{
    ST_ErrorCode_t          ErrCode;

    if (TTXMode ==  0)
        ErrCode = STTTX_Start( TTXVBIHandle );
    else
        ErrCode = STTTX_Start( TTXSTBHandle );

    STTBX_Print(("STTTX_Start()=%s\n", GetErrorText(ErrCode)));

    return ( ErrCode );

} /* end TTX_Start */

/*-------------------------------------------------------------------------
 * Function : TTX_Stop
 *            Stop teletext
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TTX_Stop( void )
{
    ST_ErrorCode_t          ErrCode;

    if (TTXMode ==  0)
        ErrCode = STTTX_Stop( TTXVBIHandle );
    else
        ErrCode = STTTX_Stop( TTXSTBHandle );

    STTBX_Print(("STTTX_Stop()=%s\n", GetErrorText(ErrCode)));

    return ( ErrCode );

} /* end TTX_Stop */

/*-------------------------------------------------------------------------
 * Function : TTX_Term
 *            Term teletext driver
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TTX_Term( BOOL ForceTerminate )
{
    ST_ErrorCode_t          ErrCode;
    STTTX_TermParams_t      TermParams;

    TermParams.ForceTerminate = ForceTerminate;

    ErrCode = STTTX_Term( TTX_DeviceName, &TermParams );

    STTBX_Print(("STTTX_Term(%s)=%s\n", TTX_DeviceName, GetErrorText(ErrCode)));

    return ( ErrCode );

} /* end TTX_Term */



/*-------------------------------------------------------------------------
 * Function : TTX_STB_Test
 *            Open teletext driver
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/

#define PAGE_BUFFER_SIZE 50
#define NUMBER_CACHEABLE_PAGES  800
#define PAGE_NO_UNIT_POS        6                   /* LO hex nybble, hammed */
#define PAGE_NO_TENS_POS        7                   /* HO hex nybble, hammed */
#define SUBPAGE_UNIT_POS        8                   /* LO hex nybble, hammed */
#define SUBPAGE_TENS_POS        9
#define SUBPAGE_HUND_POS        10
#define SUBPAGE_THOU_POS        11                  /* HO hex nybble, hammed */
#define SUBCODE2_MASK           0x07
#define SUBCODE4_MASK           0x03
#define NYBBLE_SHIFT_CNT        4

/* Page buffers for odd/even fields, used for temporary storage of raw teletext data */
STTTX_Page_t Odd[PAGE_BUFFER_SIZE];
STTTX_Page_t Even[PAGE_BUFFER_SIZE];

/* Circular buffer for odd/even page storage */
STTTX_Page_t *OddPage     = Odd;
STTTX_Page_t *EvenPage    = Even;
STTTX_Page_t *OddPage_wr  = Odd;
STTTX_Page_t *EvenPage_wr = Even;

/* Request for pages structure */
STTTX_Request_t Request;
STTTX_Page_t PageCache[NUMBER_CACHEABLE_PAGES+100];

/* Wait teletext datas semaphpre */
semaphore_t FilterSemaphore;

/* Stack size for teletext task */
task_t  CacheTask;
tdesc_t CacheTaskDescriptor;
U8      CacheTaskStack[6*1024];
BOOL    IsSerial = TRUE;

/* 8/4 hamming table used for computing value of hammed bytes */
static U8 InvHamming8_4Tab[256] =
        { 0x01,0xFF,0xFF,0x08,0xFF,0x0C,0x04,0xFF,
          0xFF,0x08,0x08,0x08,0x06,0xFF,0xFF,0x08,
          0xFF,0x0A,0x02,0xFF,0x06,0xFF,0xFF,0x0F,
          0x06,0xFF,0xFF,0x08,0x06,0x06,0x06,0xFF,
          0xFF,0x0A,0x04,0xFF,0x04,0xFF,0x04,0x04,
          0x00,0xFF,0xFF,0x08,0xFF,0x0D,0x04,0xFF,
          0x0A,0x0A,0xFF,0x0A,0xFF,0x0A,0x04,0xFF,
          0xFF,0x0A,0x03,0xFF,0x06,0xFF,0xFF,0x0E,
          0x01,0x01,0x01,0xFF,0x01,0xFF,0xFF,0x0F,
          0x01,0xFF,0xFF,0x08,0xFF,0x0D,0x05,0xFF,
          0x01,0xFF,0xFF,0x0F,0xFF,0x0F,0x0F,0x0F,
          0xFF,0x0B,0x03,0xFF,0x06,0xFF,0xFF,0x0F,
          0x01,0xFF,0xFF,0x09,0xFF,0x0D,0x04,0xFF,
          0xFF,0x0D,0x03,0xFF,0x0D,0x0D,0xFF,0x0D,
          0xFF,0x0A,0x03,0xFF,0x07,0xFF,0xFF,0x0F,
          0x03,0xFF,0x03,0x03,0xFF,0x0D,0x03,0xFF,
          0xFF,0x0C,0x02,0xFF,0x0C,0x0C,0xFF,0x0C,
          0x00,0xFF,0xFF,0x08,0xFF,0x0C,0x05,0xFF,
          0x02,0xFF,0x02,0x02,0xFF,0x0C,0x02,0xFF,
          0xFF,0x0B,0x02,0xFF,0x06,0xFF,0xFF,0x0E,
          0x00,0xFF,0xFF,0x09,0xFF,0x0C,0x04,0xFF,
          0x00,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x0E,
          0xFF,0x0A,0x02,0xFF,0x07,0xFF,0xFF,0x0E,
          0x00,0xFF,0xFF,0x0E,0xFF,0x0E,0x0E,0x0E,
          0x01,0xFF,0xFF,0x09,0xFF,0x0C,0x05,0xFF,
          0xFF,0x0B,0x05,0xFF,0x05,0xFF,0x05,0x05,
          0xFF,0x0B,0x02,0xFF,0x07,0xFF,0xFF,0x0F,
          0x0B,0x0B,0xFF,0x0B,0xFF,0x0B,0x05,0xFF,
          0xFF,0x09,0x09,0x09,0x07,0xFF,0xFF,0x09,
          0x00,0xFF,0xFF,0x09,0xFF,0x0D,0x05,0xFF,
          0x07,0xFF,0xFF,0x09,0x07,0x07,0x07,0xFF,
          0xFF,0x0B,0x03,0xFF,0x07,0xFF,0xFF,0x0E };

U8 ReverseBits(U8 b)
{
    U8 n = 0, i;

    for (i = 0; i < 8; i++)
        if ( (b & (1 << i)) != 0)
            n |= ( 0x80 >> i );

    return n;
}

void CachePageData(void)
{
    S32 i, j, CacheIndex;
    STTTX_Page_t *pCache;
    static U8 Page;
    U8 PrevPage;
    U8 *CurrLine;

    ALE_Print (st,(st,"\nALE: IN CachePageData"));

    for (i = 0; i < STTTX_MAXLINES; i++)
    {
        if ( ( OddPage->ValidLines & (1 << i) ) != 0 )
        {
            S32 PacketNo, PcktAddr1, PcktAddr2;

            PcktAddr1 = InvHamming8_4Tab[OddPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[OddPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = OddPage->Lines[i];
            ALE_Print (st,(st,"\nALE: OddPage, Line %u, Packet %u, Magazine %u",
                              i,PacketNo,(PcktAddr1 & 0x07)));


            if (PacketNo == 0)
            {
                U8 PageNumb1, PageNumb2;

                /* Get page number */
                PrevPage = Page;
                PageNumb1 = InvHamming8_4Tab[       /* LO nybble */
                           CurrLine[PAGE_NO_UNIT_POS]];
                PageNumb2 = InvHamming8_4Tab[       /* HO nybble */
                            CurrLine[PAGE_NO_TENS_POS]];
                Page = PageNumb1 |
                               ( PageNumb2 <<
                                 NYBBLE_SHIFT_CNT );

                ALE_Print (st,(st,"\nALE: OddPage, TTX Page %u, Magazine %u",
                          (((Page >> 4) & 0x0F) * 10) + (Page & 0x0F),
                           (PcktAddr1 & 0x07)));

                IsSerial = (CurrLine[13] & 0x02) ? TRUE : FALSE;

                if (IsSerial)
                {
                    /* Previous page has finished */
                }


                /* Ensure this page is not used for time filling */
                if (Page > 0x99)
                    continue;               /* Try next line */

                /* Get magazine number */

                /* Get transmission mode */

                /* Get subpage number */

                continue;
            }

            if (PacketNo > 0 && PacketNo < 26)
            {
                if ((PcktAddr1 & 0x07) == 0)
                    CacheIndex = (8 * 100) +
                             (((Page >> 4) & 0x0F) * 10) +
                             (Page & 0x0F) - 100;
                else
                    CacheIndex = ((PcktAddr1 & 0x07) * 100) +
                             (((Page >> 4) & 0x0F) * 10) +
                             (Page & 0x0F) - 100;

                ALE_Print (st,(st,"\nALE: CacheIndex + 100 = %u",CacheIndex+100));
                if (CacheIndex >= 0)
                {
                    pCache = &PageCache[CacheIndex];

                    ALE_Print (st,(st,"\nALE: Storing TTX line %u in Page %u",
                                      PacketNo, CacheIndex+100));

                    pCache->ValidLines |= (1 << PacketNo);
                    for (j = 6; j < 46; j++)
                    {
                        U8 c = ReverseBits(OddPage->Lines[i][j]) & 0x7F;
                        c = (c < 32 || c > 126) ? 32 : c;
                        pCache->Lines[PacketNo][j-6] = c;
                    }
                    pCache->Lines[PacketNo][j-6] = '\n';
                    pCache->Lines[PacketNo][j-5] = 0;
                }
            }
        }
    }

    for (i = 0; i < STTTX_MAXLINES; i++)
    {
        if ( ( EvenPage->ValidLines & (1 << i) ) != 0 )
        {
            S32 PacketNo, PcktAddr1, PcktAddr2;

            PcktAddr1 = InvHamming8_4Tab[EvenPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[EvenPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = EvenPage->Lines[i];
            ALE_Print (st,(st,"\nALE: EvenPage, Line %u, Packet %u, Magazine %u",
                              i,PacketNo,(PcktAddr1 & 0x07)));


            if (PacketNo == 0)
            {
                U8 PageNumb1, PageNumb2;

                /* Get page number */
                PageNumb1 = InvHamming8_4Tab[       /* LO nybble */
                           CurrLine[PAGE_NO_UNIT_POS]];
                PageNumb2 = InvHamming8_4Tab[       /* HO nybble */
                            CurrLine[PAGE_NO_TENS_POS]];
                Page = PageNumb1 |
                               ( PageNumb2 <<
                                 NYBBLE_SHIFT_CNT );

                ALE_Print (st,(st,"\nALE: Even Page, TTX Page %u, Magazine %u",
                                  (((Page >> 4) & 0x0F) * 10) + (Page & 0x0F),
                                  (PcktAddr1 & 0x07)));

                /* Ensure this page is not used for time filling */
                if (Page > 0x99)
                    continue;               /* Try next line */

                /* Get magazine number */

                /* Get transmission mode */

                /* Get subpage number */

                continue;
            }

            if (PacketNo > 0 && PacketNo < 26)
            {
                if ((PcktAddr1 & 0x07) == 0)
                    CacheIndex = (8 * 100) +
                             (((Page >> 4) & 0x0F) * 10) +
                             (Page & 0x0F) - 100;
                else
                    CacheIndex = ((PcktAddr1 & 0x07) * 100) +
                             (((Page >> 4) & 0x0F) * 10) +
                             (Page & 0x0F) - 100;

                ALE_Print (st,(st,"\nALE: CacheIndex + 100 = %u",CacheIndex+100));

                if (CacheIndex >= 0)
                {
                    pCache = &PageCache[CacheIndex];

                    ALE_Print (st,(st,"\nALE: Storing TTX line %u in Page %u",
                                      PacketNo, CacheIndex+100));

                    pCache->ValidLines |= (1 << PacketNo);
                    for (j = 6; j < 46; j++)
                    {
                        U8 c = ReverseBits(EvenPage->Lines[i][j]) & 0x7F;
                        c = (c < 32 || c > 126) ? 32 : c;
                        pCache->Lines[PacketNo][j-6] = c;
                    }
                    pCache->Lines[PacketNo][j-6] = '\n';
                    pCache->Lines[PacketNo][j-5] = 0;
                }
            }
        }
    }

    ALE_Print (st,(st,"\nALE: OUT CachePageData"));

}



BOOL GetPageDetails(STTTX_Page_t *pOddPage, STTTX_Page_t *pEvenPage,
                    U8 *CurrentMagazine, U8 *CurrentPage,
                    U32 *CurrentSubpage, BOOL *bSerial)
{
    U32 PacketNo, PcktAddr1, PcktAddr2;
    U32 i;
    U8 *CurrLine;

    for (i = 0; i < STTTX_MAXLINES; i++)
    {
        PacketNo = -1;

        if ( ( pOddPage->ValidLines & (1 << i) ) != 0 )
        {
            PcktAddr1 = InvHamming8_4Tab[pOddPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[pOddPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = pOddPage->Lines[i];
        }

        if ( ( pEvenPage->ValidLines & (1 << i) ) != 0 && PacketNo != 0 )
        {
            PcktAddr1 = InvHamming8_4Tab[pEvenPage->Lines[i][4]];
            PcktAddr2 = InvHamming8_4Tab[pEvenPage->Lines[i][5]];
            PacketNo = ( PcktAddr1 >> 3 ) | /* range 0..31 (should = i) */
                       ( PcktAddr2 << 1 );
            CurrLine = pEvenPage->Lines[i];
        }

        if (PacketNo == 0)
        {
            U8 PageNumb1, PageNumb2, SubPCode1, SubPCode2, SubPCode3,
            SubPCode4;

            /* Get page number */
            PageNumb1 = InvHamming8_4Tab[       /* LO nybble */
                        CurrLine[PAGE_NO_UNIT_POS]];
            PageNumb2 = InvHamming8_4Tab[       /* HO nybble */
                        CurrLine[PAGE_NO_TENS_POS]];
            *CurrentPage = PageNumb1 |
                           ( PageNumb2 <<
                             NYBBLE_SHIFT_CNT );

            /* Ensure this page is not used for time filling */
            if (*CurrentPage > 0x99)
                continue;               /* Try next line */

            /* Get magazine number */
            *CurrentMagazine = PcktAddr1 & 0x07;

            /* Get transmission mode */
            *bSerial = (CurrLine[13] & 0x02) ? TRUE : FALSE;

            /* Get subpage number */
            SubPCode4 =                     /* HI nybble */
                       InvHamming8_4Tab[CurrLine[SUBPAGE_THOU_POS]];
            SubPCode3 =
                       InvHamming8_4Tab[CurrLine[SUBPAGE_HUND_POS]];
            SubPCode2 =
                       InvHamming8_4Tab[CurrLine[SUBPAGE_TENS_POS]];
            SubPCode1 =                     /* LO nybble */
                       InvHamming8_4Tab[CurrLine[SUBPAGE_UNIT_POS]];
            *CurrentSubpage = ( SubPCode4 &
                                SUBCODE4_MASK ) <<
                              NYBBLE_SHIFT_CNT;
            *CurrentSubpage = ( SubPCode3 |
                                *CurrentSubpage ) <<
                              NYBBLE_SHIFT_CNT;
            *CurrentSubpage = ( ( SubPCode2 &
                                  SUBCODE2_MASK ) |
                                *CurrentSubpage ) <<
                              NYBBLE_SHIFT_CNT;
            *CurrentSubpage |= SubPCode1;

            return TRUE;
        }
    }

    return FALSE;
}



void TTX_CallbackFunction(U32 Tag, STTTX_Page_t *p1, STTTX_Page_t *p2 )
{
 ST_ErrorCode_t ErrCode;

 /* Increment buffer counter ready for next page */
 /* -------------------------------------------- */
 OddPage_wr++;
 EvenPage_wr++;
 if (OddPage_wr >= &Odd[PAGE_BUFFER_SIZE])  /* Buffer wrapped? */
  {
   OddPage_wr =  Odd;
   EvenPage_wr = Even;
  }

 /* Check for buffer overflow */
 /* ------------------------- */
 if (OddPage_wr == OddPage)
  {
   STTBX_Print(("--> There is an overflow !!!"));
  }

 /* Request for next page */
 /* --------------------- */
#if 1
 Request.RequestTag     = 0;
 Request.RequestType    = STTTX_FILTER_ANY;
 Request.MagazineNumber = 1;
 Request.PageNumber     = 100;
 Request.OddPage        = OddPage_wr;
 Request.EvenPage       = EvenPage_wr;
 Request.NotifyFunction = TTX_CallbackFunction;
 ErrCode = STTTX_SetRequest(TTXSTBHandle,&Request);
 if (ErrCode != ST_NO_ERROR)
  {
   STTBX_Print(("STTTX_SetRequeset()=%s\n",GetErrorText(ErrCode)));
  }
#endif

 /* Signal a semaphore to wake-up a waiting task for teletext datas */
 /* --------------------------------------------------------------- */
 semaphore_signal(&FilterSemaphore);
}

void TeletextCacheTask(void *pDummy)
{
 U8   CurrentPage = 0xFF;
 U8   CurrentMagazine = 0;
 U32  CurrentSubpage = 0;
 BOOL SerialMode, NewPage;

 while(1)
  {
   /* Wait for new teletext datas */
   semaphore_wait(&FilterSemaphore);

    CachePageData();

#if 0
   NewPage = GetPageDetails(OddPage, EvenPage,
                            &CurrentMagazine, &CurrentPage,
                            &CurrentSubpage, &SerialMode);

   /* Is new page details valid */
   if (CurrentPage<=0x99)
    {
     if (!NewPage) CachePageData(OddPage,EvenPage,CurrentPage);
    }
#endif

   /* Increment consumer pointers */
   OddPage++;
   EvenPage++;

   /* Check for read pointer wrap */
   if (OddPage>=&Odd[PAGE_BUFFER_SIZE])
    {
     OddPage  = Odd;
     EvenPage = Even;
    }
  }
}


ST_ErrorCode_t TTX_Flush()
{
    U32 i;

    for (i = 100; i < NUMBER_CACHEABLE_PAGES + 100 ; i++)
        PageCache[i-100].ValidLines = 0;
}


/*-------------------------------------------------------------------------
 * Function : TTTTX_Flush
 *
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_Flush( parse_t *pars_p, char *Result )
{
 ST_ErrorCode_t ErrCode;
 ErrCode=TTX_Flush();
 if (ErrCode != ST_NO_ERROR)
  {
   STTBX_Print(("--> TTTTX_Flush()=%s\n",GetErrorText(ErrCode)));
   return(TRUE);
  }
 return(FALSE);
}


/* Start TXT Test */
ST_ErrorCode_t TTX_STB_Test(void)
{
 ST_ErrorCode_t     ErrCode;
 STTTX_OpenParams_t OpenParams_s;
 STTTX_InitParams_t InitParams_s;
 U32                Pid;
 U32                i;

 /* Initialise semaphore */
 /* -------------------- */
 semaphore_init_fifo(&FilterSemaphore,0);


 for (i = 100; i < 800 ; i++)
    PageCache[i-100].ValidLines = 0;

 /* Set data source */
 /* --------------- */
 Request.RequestTag     = 0;
 Request.RequestType    = STTTX_FILTER_PACKET_0;
 Request.MagazineNumber = 1;
 Request.PageNumber     = 100;
 Request.OddPage        = OddPage_wr;
 Request.EvenPage       = EvenPage_wr;
 Request.NotifyFunction = TTX_CallbackFunction;
 ErrCode = STTTX_SetRequest(TTXSTBHandle,&Request);
 if (ErrCode != ST_NO_ERROR)
  {
   STTBX_Print(("STTTX_SetRequeset()=%s\n",GetErrorText(ErrCode)));
   return(ErrCode);
  }
 /* Create task to catch teletext datas */
 /* ----------------------------------- */
 if (task_init((void(*)(void *))TeletextCacheTask,
               NULL,
               CacheTaskStack,
               6*1024,
               &CacheTask,
               &CacheTaskDescriptor,
               MAX_USER_PRIORITY,
               "Teletext Cache",
               0))
  {
   STTBX_Print(("--> Error trying to spawn cache task\n"));
  }

 /* Allocate slot if this not done */
 /* ------------------------------ */
 /*
 if (vbi_slot==0) pti_allocate_dynamic_slot(&vbi_slot);
 */
 /* Initialize Teletext */
 /* ------------------- */
 /*InitParams_s.DriverPartition    = NcachePartition;
 InitParams_s.BaseAddress        = (U32*)TTXT_BASE_ADDRESS;
 InitParams_s.InterruptNumber    = TELETEXT_INTERRUPT;
 InitParams_s.InterruptLevel     = TTXT_INTERRUPT_LEVEL;
 InitParams_s.NumVBIObjects      = 1;
 InitParams_s.NumVBIBuffers      = 1;
 InitParams_s.NumSTBObjects      = 1;
 InitParams_s.NumRequestsAllowed = 4;
 strcpy(InitParams_s.EVTDeviceName,EVTDeviceName);
 ErrCode = STTTX_Init(TTX_DeviceName,&InitParams_s);
 if (ErrCode != ST_NO_ERROR)
  {
   STTBX_Print(("STTTX_Init(%s)=%s\n", TTX_DeviceName, GetErrorText(ErrCode)));
   return(ErrCode);
  }*/

 /* Open Teletext */
 /* ------------- */
 /*
 OpenParams_s.Type = STTTX_STB;
 OpenParams_s.Slot = stb_slot;
 ErrCode = STTTX_Open(TTX_DeviceName,&OpenParams_s,&TTXHandle);
 if (ErrCode != ST_NO_ERROR)
  {
   STTBX_Print(("STTTX_Open(STB,%d)=%s\n", vbi_slot, GetErrorText(ErrCode)));
   return(ErrCode);
  }

 /* Set stream identifier */
 /* --------------------- */
 /*
 Pid = 32;
 ErrCode = STTTX_SetStreamID(TTXHandle,Pid);
 if (ErrCode != ST_NO_ERROR)
  {
   STTBX_Print(("STTTX_SetStreamId(%d)=%s\n", Pid, GetErrorText(ErrCode)));
   return(ErrCode);
  }
  */

 /* Start the teletext */
 /* ------------------ */
/*
 ErrCode = STTTX_Start(TTXHandle);
 if (ErrCode != ST_NO_ERROR)
  {
   STTBX_Print(("STTTX_Start()=%s\n", GetErrorText(ErrCode)));
   return(ErrCode);
  }
*/
 /* End of function */
 /* --------------- */
 return(ErrCode);
}

/*-------------------------------------------------------------------------
 * Function : TTTTX_STB
 *            STB teletext
 * Input    : None
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTTX_STB( parse_t *pars_p, char *Result )
{
 ST_ErrorCode_t ErrCode;
 ErrCode=TTX_STB_Test();
 if (ErrCode != ST_NO_ERROR)
  {
   STTBX_Print(("--> TTTTX_STB()=%s\n",GetErrorText(ErrCode)));
   return(TRUE);
  }
 return(FALSE);
}

static BOOL TTTTX_GetPage( parse_t *pars_p, char *Result )
{
 U32 PageNumber,i;

 cget_integer( pars_p, 100, (long *)&PageNumber );

 if ((PageCache[PageNumber-100].ValidLines != 0) && (PageNumber >= 100) && (PageNumber< 800))
  {
    STTBX_Print(("\nPageNumber : %d - ValidLines: %08x\n", PageNumber,PageCache[PageNumber-100].ValidLines));
    for (i = 1; i < 26; i++)
    {
        if ((PageCache[PageNumber-100].ValidLines & (1 << i)) != 0)
            debugmessage((char *)PageCache[PageNumber-100].Lines[i]);
        else
            debugmessage("\n");
    }
  }
  else
  {
    debugmessage("No data yet\n");
    for (i = 100; i< 800; i++)
    {
        if (PageCache[i-100].ValidLines != 0)
        {
            STTBX_Print ((" %i",i));
        }
    }
    STTBX_Print (("\n"));
  }


return(FALSE);
}


static BOOL TTTTX_GetAny( parse_t *pars_p, char *Result )
{
 U32 PageNumber,i;
 BOOL DataAvailable = FALSE;

    for (PageNumber = 100; PageNumber < NUMBER_CACHEABLE_PAGES+100; PageNumber++)
    {
        if (PageCache[PageNumber-100].ValidLines != 0)
        {
            STTBX_Print(("\nPageNumber : %d - ValidLines: %08x\n", PageNumber,PageCache[PageNumber-100].ValidLines));
            for (i = 1; i < 26; i++)
            {
                if ((PageCache[PageNumber-100].ValidLines & (1 << i)) != 0)
                    debugmessage((char *)PageCache[PageNumber-100].Lines[i]);
                else
                    debugmessage("\n");
            }
            DataAvailable = TRUE;
            break;
        }
    }

    if (!DataAvailable)
    {
        STTBX_Print (("No data yet\n"));
    }


return(FALSE);
}

static BOOL TTTTX_ModeChange( parse_t *pars_p, char *Result )
{
    if (TTXMode == 0)
    {
        STTBX_Print (("Set to STB mode\n"));
        TTXMode = 1;
    }
    else
    {
        STTBX_Print (("Set to VBI mode\n"));
        TTXMode = 0;
    }
}

static BOOL TTTTX_SetVBI( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrorCode;

    ErrorCode = STTTX_ModeChange( TTXSTBHandle,STTTX_VBI);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("STTTX_ModeChange()=%s\n",GetErrorText(ErrorCode)));
        return(ErrorCode);
    }
}

static BOOL TTTTX_SetSTB( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrorCode;

    ErrorCode = STTTX_ModeChange( TTXSTBHandle,STTTX_STB);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("STTTX_ModeChange()=%s\n",GetErrorText(ErrorCode)));
        return(ErrorCode);
    }
}

static BOOL TTTTX_SetBOTH( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrorCode;

    ErrorCode = STTTX_ModeChange( TTXSTBHandle,STTTX_STB_VBI);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("STTTX_ModeChange()=%s\n",GetErrorText(ErrorCode)));
        return(ErrorCode);
    }
}


/*-------------------------------------------------------------------------
 * Function : TTX_InitCommands
 *            Definition of the macros
 *            (commands and constants to be used with Testtool)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TTX_InitCommand( void )
{
    BOOL RetErr;

    RetErr = FALSE;
    RetErr |= register_command("TTX_INIT",   TTTTX_Init,  "Init Teletext");
    RetErr |= register_command("TTX_OPEN",   TTTTX_Open,  "Open Teletext");
    RetErr |= register_command("TTX_CLOSE",  TTTTX_Close, "Close Teletext");
    RetErr |= register_command("TTX_TERM",   TTTTX_Term,  "<Force> Term Teletext");

    RetErr |= register_command("TTX_SETPID", TTTTX_SetStreamId, "<pid> Set the Teletext Pid");
    RetErr |= register_command("TTX_START",  TTTTX_Start, "Start the decoding of input stream");
    RetErr |= register_command("TTX_STOP",   TTTTX_Stop, "Stop the decoding of input stream");
    RetErr |= register_command("TTX_STB",   TTTTX_STB, "Test teletext in STB mode");
    RetErr |= register_command("TTX_GETPAGE",   TTTTX_GetPage,"Display a page");
    RetErr |= register_command("TTX_GETANY",   TTTTX_GetAny,"Display the first page in the cache.");
    RetErr |= register_command("TTX_FLUSH",   TTTTX_Flush,"Flush the cache.");
    RetErr |= register_command("TTX_MODECHANGE",   TTTTX_ModeChange,"Change the TTX mode.");
    RetErr |= register_command("TTX_SETSTB",   TTTTX_SetSTB,"Set to STB");
    RetErr |= register_command("TTX_SETVBI",   TTTTX_SetVBI,"Set to VBI");
    RetErr |= register_command("TTX_SETBOTH",   TTTTX_SetBOTH,"Set to STB_VBI");

    STTBX_Print(("TTX_InitCommand() : macros registrations "));
    if ( RetErr == TRUE )
        STTBX_Print(("failure !\n"));
    else
        STTBX_Print(("ok\n"));

    return( RetErr );
}


/* EOF */


