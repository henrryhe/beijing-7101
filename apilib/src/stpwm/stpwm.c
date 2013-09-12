/****************************************************************************

File Name   : stpwm.c

Description : Pulse Width Modulation Interface API Routines

Copyright (C) 2005, STMicroelectronics

References  :

$ClearCase (VOB: stpwm)

stpwm.fm "PWM API" Reference DVD-API-010 Revision 1.6

 ****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#endif
#include "stos.h"
#include "stpwm.h"
#include "stpwmdat.h"

/* Private Types ---------------------------------------------------------- */

/*#if !defined (PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device( device_access_t )
#endif
*/
typedef volatile U32 device_access_t;

/* Private Constants ------------------------------------------------------ */

#define STPWM_PIO_BIT_CNT       8    /* 8 PIO bits defined */
#define STPWM_UNUSED_MASK       (-1) /* unused BitMask value */
#define STPWM_BAD_BIT_MSK       (-2) /* illegal BitMask value */

#define STPWM_HANDLE_NONE       0    /* no channels allocated value   */
#define STPWM_HANDLE_NO_0       1    /* PWM channel 0 handle bit */
#define STPWM_HANDLE_NO_1       2    /* PWM channel 1 handle bit */
#define STPWM_HANDLE_NO_2       4    /* PWM channel 2 handle bit */
#define Nb_TIMES                3

#define STPWM_OFFSET_0        0
#define STPWM_OFFSET_1        1
#define STPWM_OFFSET_2        2

/* Encode array from PWM Channels to Handles,
   hidden from prying eyes....                */
static const U32 stpwm_PWMEnc[] 	  =  { STPWM_HANDLE_NO_0,
                                    	   STPWM_HANDLE_NO_1,
                                           STPWM_HANDLE_NO_2 };

/* Private Variables ------------------------------------------------------ */

device_access_t *stpwm_HWAccess;                /* U32* Hardware Access */

static ST_DeviceName_t stpwm_DeviceName = "\0"; /* ensure NUL before Init
                                                   because [0] is used to
                                                   flag whether initialised */

static STPWM_InitParams_t stpwm_InitParams_s;   /* nests STPWM_C4InitParams_t */
static STPWM_C4InitParams_t *stpwm_C4InPars_p;  /* pointer into above */
STPWM_Handle_t *PWMHandle = NULL;

static U32 Skip = 0;
#if defined (WA_GNBvd54182) || defined (WA_GNBvd56512)
static U8  IntFirstTime = 1;
static U32 InterTime=0;
#endif

/*************************************************************
STPWM Callback private structure : To store the information
of callback assocaited with the Mode
**************************************************************/
typedef struct STPWM_CallbackInfo_s
{
    STPWM_Handle_t          Handle;
    STPWM_Mode_t            PWMMode;
    STPWM_CallbackParam_t   CallbackParam;
}STPWM_CallbackInfo_t;

STPWM_CallbackInfo_t  CallbackInfo[8]={{0,0,{NULL,NULL}}}; /* max modes 8*/

static U32 stpwm_ChansOpen = STPWM_HANDLE_NONE; /* all Closed initially */

/* the following will hold bit subscripts
   formed from BitMask values for each of
   the STPWM_CHANNEL_CNT channels          */
static int PioBitSubs[STPWM_CHANNEL_CNT];

static STPWM_Mode_t stpwm_PWMModeUsed[STPWM_CHANNEL_CNT];

/* the following stores the Handles
   returned by STPIO_Open calls, one
   per PWM bit managed by the PIO    */
static BOOL           PioChannelInUse[STPWM_CHANNEL_CNT];
static STPIO_Handle_t PioHandle[STPWM_CHANNEL_CNT];

/* ISR */

STOS_INTERRUPT_DECLARE( PWMInterruptHandler , Param ) ;

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

static BOOL IsOutsideRange( U32 ValuePassed,
                            U32 LowerBound,
                            U32 UpperBound );

static int  MaskToIndex( STPIO_BitMask_t BitMask );


/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : STPWM_GetRevision()

Description  : Returns a pointer to the Driver Revision String.
               May be called at any time.

Parameters   : None

Return Value : ST_Revision_t

See Also     : ST_Revision_t
 ****************************************************************************/

ST_Revision_t STPWM_GetRevision( void )
{
      return ((ST_Revision_t)STPWM_REVISION);
}

/****************************************************************************
Name         : STPWM_Init()

Description  : Initializes the PWM Hardware before use.

Parameters   : STPWM_DeviceType_t non-NUL name needs to be supplied,
               STPWM_InitParams_t pointer holding Version, PWM Device
               and nested STPWM_C4InitParams_t structure.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_UNKNOWN_DEVICE         Invalid device name
               ST_ERROR_ALREADY_INITIALIZED    Device already initialized
               ST_ERROR_BAD_PARAMETER          One or more parameters invalid

See Also     : STPWM_InitParams_t
               STPWM_Open()
               STPWM_SetRatio()
               STPWM_GetRatio()
               STPWM_Close()
               STPWM_Term()
 ****************************************************************************/

ST_ErrorCode_t STPWM_Init( const ST_DeviceName_t    Name,
                           const STPWM_InitParams_t *InitParams )
{
    int   i;
    const STPWM_C4InitParams_t *C4Init_p;   /* temporary ptr for checks */
 
    /* Perform parameter validity checks */

    if( stpwm_DeviceName[0] != '\0' ) /* prior Init call without Term? */
    {
        return( ST_ERROR_ALREADY_INITIALIZED );
    }

    if( ( Name                == NULL     )   || /* NULL Name ptr? */
        ( InitParams          == NULL     )   || /* NULL structure ptr? */
        ( Name[0]             == '\0'     )   || /* Device Name undefined? */
        ( strlen( Name )      >=
                    ST_MAX_DEVICE_NAME    ) )    /* string too long? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    if( InitParams->Device  != STPWM_C4 )        /* unsupported device? */
    {
        return( ST_ERROR_FEATURE_NOT_SUPPORTED );
    }
    /* point to nested STPWM_C4InitParams_t for further checks */
    C4Init_p  = &InitParams->C4;
 
    if( C4Init_p->BaseAddress == NULL )  /* BaseAddress ptr omitted? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    
#if defined (ST_8010)

    /* UGLY, but ??? */

    /*FOR PWM 0 Channel
    to select pwm0 Output instead of pio2[3]*/
	*(volatile U32 *)(0x5000301c) = 0x8000;

   /* to select pwm0 compare instead of pio2[5]and
      pwm1 output instead of pio2[6]*/
   *(volatile U32 *)(0x47000028) |= 0xc0000000;

   /* FOR PWM 1 Channel to select pwm1 output instead of pio2[6]*/
   *(volatile U32 *)(0x50003018) = 0x20;

#endif

    /* the STPIO_PIOBit_t structures within
       STPWM_C4InitParams_t are converted
       from STPIO_BitMask_t values to index;
       NB - unused BitMasks should be zero,
       otherwise they will be range checked!  */

    PioBitSubs[STPWM_CHANNEL_0] =
        MaskToIndex( C4Init_p->PIOforPWM0.BitMask );
    PioBitSubs[STPWM_CHANNEL_1] =
        MaskToIndex( C4Init_p->PIOforPWM1.BitMask );
    PioBitSubs[STPWM_CHANNEL_2] =
        MaskToIndex( C4Init_p->PIOforPWM2.BitMask );

	
    for( i = STPWM_CHANNEL_0;
         i < STPWM_CHANNEL_CNT; i++ )
    {
        if( PioBitSubs[i] ==
            STPWM_BAD_BIT_MSK )
        {
            return( ST_ERROR_BAD_PARAMETER );
        }
    }
    
    /* Attempt to install the interrupt handler */
    STOS_InterruptLock();
    if (STOS_InterruptInstall(C4Init_p->InterruptNumber,
                              C4Init_p->InterruptLevel,
                              PWMInterruptHandler,
                              "PWM",
                              (void *) 0 ) == 0 /*STOS_SUCCESS*/ )
    {

       if(STOS_InterruptEnable(C4Init_p->InterruptNumber, 
       	  C4Init_p->InterruptLevel) == 0 /*STOS_SUCCESS*/)
       {
          STOS_InterruptUnlock(); /* Re-enable interrupts */

       }
       else
       {
          STOS_InterruptUnlock(); /* Re-enable interrupts */
          /* Error enabling interrupts */
          return( ST_ERROR_BAD_PARAMETER);
       }
    }
    else
    {
         STOS_InterruptUnlock(); /* Re-enable interrupts */
         /* Error installing interrupts */
         return(ST_ERROR_INTERRUPT_INSTALL);
    }

	for( i = STPWM_CHANNEL_0; i < STPWM_CHANNEL_CNT; i++ )
      {
		stpwm_PWMModeUsed[i]= 0xFF;
       }
 
    stpwm_InitParams_s   =  *InitParams;        /* structure copy data */
    stpwm_C4InPars_p     =  &stpwm_InitParams_s.C4;            /* store pointer to C4 */
   
    stpwm_HWAccess =
             stpwm_C4InPars_p->BaseAddress;         /* initialize H/W pointer */

    /* Ensure atomic operation between reading HW registers
       and writing back to them as these registers are shared
       with STCOUNT */

    STOS_InterruptLock() ;

#if !( defined (ST_5100)|| defined (ST_7710) ||defined(ST_7109)|| defined(ST_7100) ||\
                                                                    defined(ST_5525) ||defined(ST_7200))
    /* the following are precautionary:
       they should be reset on power-up */
    stpwm_HWAccess[STPWM_INT_ENAB] &=
                    ~STPWM_PWM_INT;            /* disable only PWM interrupt */
    stpwm_HWAccess[STPWM_INT_ACKN] =
                        STPWM_PWM_INT;         /* ackn. PWM interrupt to clr */
#else
    stpwm_HWAccess[STPWM_INT_ENAB] &=
                       STPWM_DISABLE_VALUE;            /* disable all interrupts */
    stpwm_HWAccess[STPWM_INT_ACKN] =
                       STPWM_INT_ACKN_CLEAR_ALL;       /* ackn. all interrupt to clr */
#endif  /* End ST_5100 */

    STOS_InterruptUnlock()  ;                        /* end of atomic region */

    strcpy( ( char* )stpwm_DeviceName, Name ); /* flag successful Init call */

    return( ST_NO_ERROR );
}


/****************************************************************************
Name         : STPWM_Open()

Description  : Opens the Pulse Width Modulated Interface for operation.
               MUST have been preceded by a successful Init call.

Parameters   : A STPWM_DeviceType_t name (matching name given to Init)
               needs to be supplied, a pointer to STPWM_OpenParams_t,
               plus a pointer to STPWM_Handle_t for return of the
               device access handle.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_UNKNOWN_DEVICE     Invalid Name/no prior Init
               ST_ERROR_BAD_PARAMETER      One or more invalid parameters
               ST_ERROR_NO_FREE_HANDLES    Device already open
               STPWM_ERROR_PIO             Error within PIO subcomponent

See Also     : STPWM_Handle_t
               STPWM_OpenParams_t
               STPIO_OpenParams_t
               STPWM_Init()
               STPWM_Close()
 ****************************************************************************/

ST_ErrorCode_t STPWM_Open( ST_DeviceName_t    Name,
                           STPWM_OpenParams_t *OpenParams,
                           STPWM_Handle_t     *Handle )
{
    U32  ChanToOpen;                         /* OpenParams extract */
#if ! (defined (ST_5100) || defined (ST_7710) || defined(ST_5525))
    U32   HWRead;
#endif
    U32   HWUpdt;                             /* H/W temporary access values */        
    int   Index;                              /* BitConfigure subscript */
    ST_DeviceName_t stpio_DeviceName;         /* C4InitParams_t extract */
    STPIO_OpenParams_t stpio_OpenParams;      /* STPIO_Open call param */
    ST_ErrorCode_t stpio_Return;              /* STPIO_Open return code */
    STPWM_Mode_t PWMUse = 0;
    
    /* Perform necessary validity checks */
    if(( Name       == NULL )           || /* Name   ptr uninitialised? */
       ( OpenParams == NULL )           || /* struct ptr uninitialised? */
       ( Handle     == NULL ))             /* Handle ptr uninitialised? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    ChanToOpen      = OpenParams->C4.ChannelNumber;
    PWMUse          = OpenParams->PWMUsedFor;
   
     /* Previous call to Init MUST have
     had the same PWM Device Name */
    if( ( stpwm_DeviceName[0] == '\0' )  || /* Init not previously called? */
        ( strcmp(( char* )stpwm_DeviceName,
                Name ) != 0 ) )             /* mismatch Name with Init? */
    {
        return( ST_ERROR_UNKNOWN_DEVICE );
    }
    
#if defined(ST_5100)
	/* No support of compare and capture*/
    if(PWMUse != STPWM_Timer)
    {
   	    return( ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
        
    /* Previous call to Init MUST have
       had the same PWM Device Name */
    if( ( stpwm_DeviceName[0] == '\0' )  || /* Init not previously called? */
        ( strcmp(( char* )stpwm_DeviceName,
                Name ) != 0 ) )             /* mismatch Name with Init? */
    {
        return( ST_ERROR_UNKNOWN_DEVICE );
    }

    if( ( stpwm_ChansOpen &
        stpwm_PWMEnc[ChanToOpen] ) != 0 )   /* this channel already open? */
    {
        return( ST_ERROR_NO_FREE_HANDLES );
    }
    
    if(PWMUse == STPWM_Timer)
    {
	    if(IsOutsideRange(OpenParams->C4.PrescaleFactor,
	    				  STPWM_PRESCALE_MIN,
	    				  STPWM_PRESCALE_MAX ))
	    {
	    	 return( ST_ERROR_BAD_PARAMETER );
	    }       
	    /* Channel No. o/s range? */
		if (IsOutsideRange(
		        ChanToOpen,
		        STPWM_CHAN_NO_MIN,
		        STPWM_CHAN_NO_MAX ))
		{
		    return( ST_ERROR_BAD_PARAMETER );
		}
 	
    }
    else
    {
    	if(IsOutsideRange(OpenParams->C4.PrescaleFactor,
	    				  STPWM_CPTCMP_PRESCALE_MIN,
	    				  STPWM_CPTCMP_PRESCALE_MAX ))
        {
	    	 return( ST_ERROR_BAD_PARAMETER );
	    } 
	    if (IsOutsideRange(
		        ChanToOpen,
		        STPWM_CHAN_NO_MIN,
		        STPWM_CHAN_NO_CPT_MAX ))
		{
		    return( ST_ERROR_BAD_PARAMETER );
		}
    } 
    
    /* Only Open a PIO connection if PioBitSubs
       for this channel was set at Init time */

    Index = PioBitSubs[ChanToOpen];         /* BitConfigure subscript */

    PioChannelInUse[ChanToOpen] = FALSE;     /* Default setting */

    if( Index != STPWM_UNUSED_MASK )
    {
    	 stpio_OpenParams.IntHandler = NULL;  /* this is not used, hence NULL */
        if((OpenParams->PWMUsedFor == STPWM_Capture0 ) ||\
					(OpenParams->PWMUsedFor == STPWM_Capture1))
        {
    		 stpio_OpenParams.BitConfigure[Index] =
 		          			   STPIO_BIT_INPUT;     /* program PIO for PWM capture input*/
        }
        else
        {
        	stpio_OpenParams.BitConfigure[Index] =
		     		           STPIO_BIT_ALTERNATE_OUTPUT;        
		     		 								/* program PIO for PWM output or compare output*/ 
	    }
        /* switch on PWM channel; default case
           omitted as Range is checked above */
        switch( ChanToOpen )
        {
            case STPWM_CHANNEL_0:

                strcpy( (char*)stpio_DeviceName,
                        stpwm_C4InPars_p->
                        PIOforPWM0.PortName );      /* copy PIO DeviceName */

                stpio_OpenParams.ReservedBits =
                  stpwm_C4InPars_p->
                  PIOforPWM0.BitMask;               /* copy PIO BitMask */

                break;


            case STPWM_CHANNEL_1:

                strcpy( (char*)stpio_DeviceName,
                        stpwm_C4InPars_p->
                        PIOforPWM1.PortName );      /* copy PIO DeviceName */

                stpio_OpenParams.ReservedBits =
                  stpwm_C4InPars_p->
                  PIOforPWM1.BitMask;               /* copy PIO BitMask */

                break;

            case STPWM_CHANNEL_2:

                strcpy( (char*)stpio_DeviceName,
                        stpwm_C4InPars_p->
                        PIOforPWM2.PortName );     /* copy PIO DeviceName */

                stpio_OpenParams.ReservedBits =
                  stpwm_C4InPars_p->
                  PIOforPWM2.BitMask;              /* copy PIO BitMask */

                break;

              /* no default case required */
        }

        /* ask PIO to Open PWM channel */
        stpio_Return = STPIO_Open( stpio_DeviceName,
                                   &stpio_OpenParams,
                                   &PioHandle[ChanToOpen] );

        /* if STPIO_Open returns an error,
           this is returned to STPWM_Open
           caller - the PWM Open has failed */
        if( stpio_Return != ST_NO_ERROR )
        {
            return( STPWM_ERROR_PIO );         /* return generic PIO error */
        }

        PioChannelInUse[ChanToOpen] = TRUE;
    }

    /* Preload PreScaleFactor before disabling interrupts */
 
    HWUpdt = ( OpenParams->C4.PrescaleFactor - 1 );  /*  DISABLES b9 PWM counter */
  
#if defined (ST_7710)
    /* [31:17] Reserved
       [16:5] PWMCLKVALUE_A[15:4] prescalar high order bits
       [4] PWMENABLE_A
       [3:0] PWMCLKVALUE_A[3:0]: prescalar low order bits
     */
    HWUpdt = ((HWUpdt << 1) & 0xFFFFFFE0) | (HWUpdt & 0x0F);

#elif  defined(ST_5525)
     /* [31:15] Reserved
       [14:11] PWMCLKVALUE_A prescalar high order bits
       [10] CPT_EN
       [9]  PWMENABLE_A
       [8:4]CPT_CLK_Prescalar value
       [3:0] PWMCLKVALUE_A[3:0]: prescalar low order bits
     */
    HWUpdt = ((HWUpdt << 1) & 0xFFFFF800) | (HWUpdt & 0x0F);
    
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    if(PWMUse == STPWM_Timer)
    {
        HWUpdt = ((HWUpdt << 7) & 0xFFFFF800) | (HWUpdt & 0x0F);
    }
    else /* capture and compare prescaler bit Bit [8:4] */
    {
   	    HWUpdt = ((HWUpdt << 4) & 0x000001F0);  
    }
#endif

    /* Ensure atomic operation between reading HW register
       and writing back as this registers is shared
       with STCOUNT */
    STOS_InterruptLock();
	
    /* program the PWM channel */
    if(OpenParams->PWMUsedFor == STPWM_Timer)
    {     
		stpwm_HWAccess[STPWM_CONTROL] |=  
            				        STPWM_PWM_ENAB;          /* set (if not on already) */
    }
    else
    {
		 stpwm_HWAccess[STPWM_CONTROL] |= 
            				        STPWM_CAPTURE_ENAB;      /* set (if not on already) */
     }

#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    HWRead =
        stpwm_HWAccess[STPWM_CONTROL];
    stpwm_HWAccess[STPWM_CONTROL] =            /* update PWM bits only, */
                            HWRead | HWUpdt;   /* preserving any Counter setup */   
                           
#elif !( defined (ST_5100)|| defined (ST_7710) || defined(ST_5525))

    HWRead =
        stpwm_HWAccess[STPWM_CONTROL] &        /* read current H/W value */
            STPWM_COUNT_RETAIN;                /* strip all but COUNT bits */

    stpwm_HWAccess[STPWM_CONTROL] =            /* update PWM bits only, */
                            HWRead | HWUpdt;   /* preserving any Counter setup */
#else                                
    stpwm_HWAccess[STPWM_CONTROL] =  HWUpdt;   /* update PWM bits only, */                  
    
#endif    
    STOS_InterruptUnlock();     /* end of atomic region */         

    *Handle = stpwm_PWMEnc[ChanToOpen];     /* return Handle for PWM Channel */
    PWMHandle = (STPWM_Handle_t *)Handle;
    stpwm_ChansOpen |= *Handle;             /* add new channel to those open */
   stpwm_PWMModeUsed[ChanToOpen] = PWMUse;

    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STPWM_SetRatio()

Description  : Applies the specified Mark/Ratio to the
               PWM channel associated with the given Handle.

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 MarkValue for programming the hardware.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
               STPWM_GetRatio()
 ****************************************************************************/

ST_ErrorCode_t STPWM_SetRatio( STPWM_Handle_t Handle,
                               U32            MarkValue )
{
    U32  ChanToAccess;

    /* convert handle to channel no.
       and check for input legality */
    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
            ChanToAccess = STPWM_CHANNEL_0;
            break;

        case STPWM_HANDLE_NO_1:
            ChanToAccess = STPWM_CHANNEL_1;
            break;

        case STPWM_HANDLE_NO_2:
            ChanToAccess = STPWM_CHANNEL_2;
            break;

        default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( IsOutsideRange( MarkValue,
            STPWM_MARK_MIN,
            STPWM_MARK_MAX ) )              /* outside valid range? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    STOS_InterruptLock();
  
    stpwm_HWAccess[STPWM_VAL_0 +
        ChanToAccess] = MarkValue - 1;      /* MarkSpace set to user value */

    STOS_InterruptUnlock();                     /* end of atomic region */


    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STPWM_GetRatio()

Description  : Returns the mark/space ratio associated
               with the supplied handle.

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 *MarkValue for return of the result.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
               STPWM_SetRatio()
 ****************************************************************************/

ST_ErrorCode_t STPWM_GetRatio( STPWM_Handle_t Handle,
                               U32            *MarkValue )
{
    U32  ChanToAccess;

    /* convert handle to channel no.
       and check for input legality */
    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
            ChanToAccess = STPWM_CHANNEL_0;
            break;

        case STPWM_HANDLE_NO_1:
            ChanToAccess = STPWM_CHANNEL_1;
            break;

        case STPWM_HANDLE_NO_2:
            ChanToAccess = STPWM_CHANNEL_2;
            break;

        default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( MarkValue == NULL )                 /* MarkValue pointer omitted? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

#if defined (ST_7710)
    /* sti7710 has HW problem */
    *MarkValue = 0;
    return( ST_NO_ERROR );
#endif

    *MarkValue = stpwm_HWAccess[
        STPWM_VAL_0 + ChanToAccess] + 1;    /* MarkSpace returned to caller */

    return( ST_NO_ERROR );
}
/****************************************************************************
Name         : STPWM_SetCompareValue()

Description  : Set the compare value in CMP_VAL register to the
               PWM channel associated with the given Handle.

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 CompareValue for programming the hardware.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
               STPWM_GetCompareValue()
 ****************************************************************************/

ST_ErrorCode_t STPWM_SetCompareValue(STPWM_Handle_t Handle,
                                        U32         CompareValue)
{
	U32  ChanToAccess;
	STPWM_Mode_t PWMUse = 0xFF;
	U32 Offset;

   /* convert handle to channel no.
       and check for input legality */
    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
            ChanToAccess = STPWM_CHANNEL_0;
            break;

        case STPWM_HANDLE_NO_1:
            ChanToAccess = STPWM_CHANNEL_1;
            break;

	case STPWM_HANDLE_NO_2:
            ChanToAccess = STPWM_CHANNEL_2;
            break;

	default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( IsOutsideRange( CompareValue,
            STPWM_COMPARE_VAL_MIN,
            STPWM_COMPARE_VAL_MAX ) )              /* outside valid range? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

     PWMUse= stpwm_PWMModeUsed[ChanToAccess];

     switch( PWMUse)
    {
        case STPWM_Compare0:
            Offset= STPWM_OFFSET_0;
            break;

        case STPWM_Compare1:
            Offset= STPWM_OFFSET_1;
            break;
        default:
            return( ST_ERROR_BAD_PARAMETER );
    }

    STOS_InterruptLock();
 
    stpwm_HWAccess[STPWM_COMPARE0_VAL + Offset] = CompareValue;  
    /*Compare value set to user value */
      
    STOS_InterruptUnlock();                     /* end of atomic region */
    return( ST_NO_ERROR );
}
/****************************************************************************
Name         : STPWM_GetCompareValue()

Description  : Returns the compare value associated
               with the supplied handle.

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 *CompareValue for return of the result.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
               STPWM_SetCompareValue()
 ****************************************************************************/
ST_ErrorCode_t STPWM_GetCompareValue(STPWM_Handle_t Handle,
                                        U32         *CompareValue)
{                                        
    U32  ChanToAccess;
    STPWM_Mode_t PWMUse = 0xFF;
    U32 Offset;

    /* convert handle to channel no.
       and check for input legality */
    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
            ChanToAccess = STPWM_CHANNEL_0;
            break;

        case STPWM_HANDLE_NO_1:
            ChanToAccess = STPWM_CHANNEL_1;
            break;

	case STPWM_HANDLE_NO_2:
            ChanToAccess = STPWM_CHANNEL_2;
            break;
        default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( CompareValue == NULL )                 /* MarkValue pointer omitted? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

     PWMUse= stpwm_PWMModeUsed[ChanToAccess];

     switch( PWMUse)
    {
        case STPWM_Compare0:
            Offset = STPWM_OFFSET_0;
            break;

        case STPWM_Compare1:
            Offset= STPWM_OFFSET_1;
            break;
        default:
            return( ST_ERROR_BAD_PARAMETER );
    }
	 
    *CompareValue = stpwm_HWAccess[
        STPWM_COMPARE0_VAL + Offset];    /* Compare value returned to caller */

    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STPWM_GetCaptureValue()

Description  : Returns the Capture value from CPT_VAL in case any capture 
			   event occurs			        

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 *CaptureValue for return of the result.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
****************************************************************************/
ST_ErrorCode_t STPWM_GetCaptureValue(STPWM_Handle_t Handle,
                                     U32            *CaptureValue)
{                                        
    U32  ChanToAccess;
    STPWM_Mode_t PWMUse = 0xFF;
    U32 Offset;

    /* convert handle to channel no.
       and check for input legality */
    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
            ChanToAccess = STPWM_CHANNEL_0;
            break;

        case STPWM_HANDLE_NO_1:
            ChanToAccess = STPWM_CHANNEL_1;
            break;

	case STPWM_HANDLE_NO_2:
            ChanToAccess = STPWM_CHANNEL_2;
            break;
        default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( CaptureValue == NULL )                 /* MarkValue pointer omitted? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    PWMUse= stpwm_PWMModeUsed[ChanToAccess];

     switch( PWMUse)
    {
        case STPWM_Capture0:
            Offset = STPWM_OFFSET_0;
            break;

        case STPWM_Capture1:
            Offset= STPWM_OFFSET_1;
            break;
        default:
            return( ST_ERROR_BAD_PARAMETER );
    }

    *CaptureValue = stpwm_HWAccess[
        STPWM_CAPTURE0_VAL + Offset];    /* Capture value returned to caller */

    return( ST_NO_ERROR );
}
/****************************************************************************
Name         : STPWM_SetCaptureEdge()

Description  : Controls the edge used for the capture of the timer in register 
			   PWM0_CPT_VAL associated with the Handle

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 EdgeControl for programming the hardware.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
			   STPWM_GetCaptureEdge
 ****************************************************************************/
ST_ErrorCode_t STPWM_SetCaptureEdge(STPWM_Handle_t          Handle,
                                    STPWM_EdgeControl_t     EdgeControl)
{
	U32  ChanToAccess;
	STPWM_Mode_t PWMUse = 0xFF;
       U32 Offset;

    /* convert handle to channel no.
       and check for input legality */
    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
            ChanToAccess = STPWM_CHANNEL_0;
            break;

        case STPWM_HANDLE_NO_1:
            ChanToAccess = STPWM_CHANNEL_1;
            break;

	case STPWM_HANDLE_NO_2:
            ChanToAccess = STPWM_CHANNEL_2;
            break;
        default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( IsOutsideRange( EdgeControl,
                  STPWM_Disabled,
                  STPWM_RisingOrFallingEdge ) )              /* outside valid range? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    PWMUse= stpwm_PWMModeUsed[ChanToAccess];

     switch( PWMUse)
    {
        case STPWM_Capture0:
            Offset = STPWM_OFFSET_0;
            break;

        case STPWM_Capture1:
            Offset= STPWM_OFFSET_1;
            break;
        default:
            return( ST_ERROR_BAD_PARAMETER );
    }

    STOS_InterruptLock();
	     
    stpwm_HWAccess[STPWM_CAPTURE0_EDGE + Offset] = EdgeControl;  
    /*Compare value set to user value */
      
    STOS_InterruptUnlock();                     /* end of atomic region */
    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STPWM_GetCaptureEdge()

Description  : Retrieves the edge used for the capture of the timer from
			   the PWM0_CPT_VAL register associated with the Handle

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 *EdgeControl for return of the result.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
 ****************************************************************************/
ST_ErrorCode_t STPWM_GetCaptureEdge(STPWM_Handle_t          Handle,
                                    STPWM_EdgeControl_t     *EdgeControl) 
{                                       
    U32  ChanToAccess;
    STPWM_Mode_t PWMUse = 0xFF;
    U32 Offset;

    /* convert handle to channel no.
       and check for input legality */
    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
            ChanToAccess = STPWM_CHANNEL_0;
            break;

        case STPWM_HANDLE_NO_1:
            ChanToAccess = STPWM_CHANNEL_1;
            break;

	case STPWM_HANDLE_NO_2:
            ChanToAccess = STPWM_CHANNEL_2;
            break;
        default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( EdgeControl == NULL )                 /* Compare value pointer omitted? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    PWMUse= stpwm_PWMModeUsed[ChanToAccess];

     switch( PWMUse)
    {
        case STPWM_Capture0:
            Offset = STPWM_OFFSET_0;
            break;

        case STPWM_Capture1:
            Offset= STPWM_OFFSET_1;
            break;
        default:
            return( ST_ERROR_BAD_PARAMETER );
    }

    *EdgeControl = stpwm_HWAccess[
              STPWM_CAPTURE0_EDGE + Offset];    
    /* programmed edge value returned to caller */

    return( ST_NO_ERROR );
}   

/****************************************************************************
Name         : STPWM_SetCMPCPTCounterValue()

Description  : Sets the value of Counter used in capture and compare mode.

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 Value for programming the hardware.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
			   STPWM_GetCMPCPTCounterValue
	   
 ****************************************************************************/
ST_ErrorCode_t STPWM_SetCMPCPTCounterValue(STPWM_Handle_t  Handle,
                                           U32             Value)
{
	if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }
    
    if( IsOutsideRange( Value,
                  STPWM_CMPCPTCOUNTER_VAL_MIN,
                  STPWM_CMPCPTCOUNTER_VAL_MAX ))              /* outside valid range? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    
    
    STOS_InterruptLock();
    
    stpwm_HWAccess[STPWM_CMP_CPT_CNT] =  Value; 
    
    STOS_InterruptUnlock();    
    
    /*Capture counter  value set in the register */
    return( ST_NO_ERROR );
} 
/****************************************************************************
Name         : STPWM_GetCMPCPTCounterValue()

Description  : Retrieves the value of Counter used in capture and compare mode.

Parameters   : STPWM_Handle_t Handle is passed, plus an
               U32 *Value for return of the result

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
			   STPWM_SetCMPCPTCounterValue
 ****************************************************************************/
ST_ErrorCode_t STPWM_GetCMPCPTCounterValue(STPWM_Handle_t  Handle,
                                           U32             *Value)
{
	if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }
	if( Value == NULL )                 /* MarkValue pointer omitted? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    *Value = stpwm_HWAccess[STPWM_CMP_CPT_CNT];     
    /* Capture counter present value returned to caller */
    return( ST_NO_ERROR );
}     

/****************************************************************************
Name         : STPWM_CounterControl()

Description  : This APIs gives control to the user for enabling/disabling 
			   the counter at any time respective to the mode

Parameters   : STPWM_Handle_t Handle is passed, plus an
               BOOL CounterEnable for either enabling or disabling the counter
               plus the PWM mode.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
 ****************************************************************************/
ST_ErrorCode_t STPWM_CounterControl(STPWM_Handle_t  Handle,
                                    STPWM_Mode_t    PWMUsedFor,
                                    BOOL            CounterEnable)
{
    /* convert handle to channel no.
       and check for input legality */
    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( IsOutsideRange( PWMUsedFor,
                    STPWM_Timer,
                    STPWM_Compare1))              /* outside valid range? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    STOS_InterruptLock();
    
    if(PWMUsedFor == STPWM_Timer)
    {
    	if(CounterEnable == TRUE)
               stpwm_HWAccess[STPWM_CONTROL] |= 
            				       STPWM_PWM_ENAB ;       /* set the PWM enable bit */
        else
               stpwm_HWAccess[STPWM_CONTROL] &= 
             				       ~STPWM_PWM_ENAB ;      /* clear the PWM Enable bit */
    }
    else
    {
        if(CounterEnable == TRUE)
               stpwm_HWAccess[STPWM_CONTROL] |= 
             				       STPWM_CAPTURE_ENAB ;  /* set the Capture enable bit */
        else
               stpwm_HWAccess[STPWM_CONTROL] &= 
             				       ~STPWM_CAPTURE_ENAB ;  /* clear the Capture Enable bit */
    }
     
    STOS_InterruptUnlock();                     /* end of atomic region */
    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STPWM_InterruptControl()

Description  : This APIs gives control to the user for enabling/disabling 
			   the interrupts at any time respective with the mode

Parameters   : STPWM_Handle_t Handle is passed, plus an
               BOOL InterruptEnable for either enabling or disabling the interrupts
               plus the PWM mode.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
 ****************************************************************************/
ST_ErrorCode_t STPWM_InterruptControl(STPWM_Handle_t  Handle,
                                      STPWM_Mode_t    PWMUsedFor,
                                      BOOL            InterruptEnable)  
{                                         
    U32  ChanToAccess;
   
    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }
    
    /* convert handle to channel no.
       and check for input legality */
    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
            ChanToAccess = STPWM_CHANNEL_0;
            break;

        case STPWM_HANDLE_NO_1:
            ChanToAccess = STPWM_CHANNEL_1;
            break;

        case STPWM_HANDLE_NO_2:
            ChanToAccess = STPWM_CHANNEL_2;
            break;

        default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( IsOutsideRange( PWMUsedFor,
                    STPWM_Timer,
                    STPWM_Compare1))              /* outside valid range? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    STOS_InterruptLock();
    
    if(PWMUsedFor == STPWM_Timer)
    {
    	if(InterruptEnable == TRUE)
    	{
	    	 stpwm_HWAccess[STPWM_INT_ENAB] |=
         				            STPWM_PWM_INT;         /* Enable PWM interrupt */  
               stpwm_HWAccess[STPWM_INT_ACKN] |=
         				            STPWM_PWM_INT;         /* Ackn PWM interrupt to clear */           				            
         				            
        }              				            
        else
        {
               stpwm_HWAccess[STPWM_INT_ENAB] &= 
             				       ~STPWM_PWM_INT ;        /* Disable PWM interrupt  */
        }             				       
    }
    else
    {
	    if (PWMUsedFor == STPWM_Capture0 )
	    {					  
		       if(InterruptEnable == TRUE)
		       {
		               stpwm_HWAccess[STPWM_INT_ENAB] |=
		          				            STPWM_CAPTURE0_INT;         /* Enable Capture0 interrupt */
	                       stpwm_HWAccess[STPWM_INT_ACKN] |=
         			          	            STPWM_CAPTURE0_INT;          /* Ackn the Capture0 interrupts */     		         				                       
       	        }
		       else
		               stpwm_HWAccess[STPWM_INT_ENAB] &= 
		             				        ~STPWM_CAPTURE0_INT ;        /* Disable Capture0 interrupt  */
	      }
	      
	if (PWMUsedFor == STPWM_Capture1 )
	   {
	           if(InterruptEnable == TRUE)
	           {
	                   stpwm_HWAccess[STPWM_INT_ENAB] |=
	         				                STPWM_CAPTURE1_INT;          /* Enable Capture1 interrupt */  
                       stpwm_HWAccess[STPWM_INT_ACKN] |=
         			          	            STPWM_CAPTURE1_INT;          /* Ack the Capture1 to clear*/     		         				                
               }         			          	            
	          else
	                   stpwm_HWAccess[STPWM_INT_ENAB] &= 
	            				           ~STPWM_CAPTURE1_INT ;         /* Disable Capture1 interrupt  */
	   }
	if (PWMUsedFor == STPWM_Compare0)
	  {
       	   if(InterruptEnable == TRUE)
       	   {
                       stpwm_HWAccess[STPWM_INT_ENAB] |=
       			                    STPWM_COMPARE0_INT;          /* Enable Compare0 interrupt */ 
                       stpwm_HWAccess[STPWM_INT_ACKN] |=
         			          	            STPWM_COMPARE0_INT;          /* Ack the Compare0 to clear*/             				                    
                 } 
                 else
                       stpwm_HWAccess[STPWM_INT_ENAB] &= 
                       					    ~STPWM_COMPARE0_INT;         /* Disable Compare0 interrupt */
          }
	   
	 if (PWMUsedFor == STPWM_Compare1)
	  {
	           if(InterruptEnable == TRUE)
	           {
	                   stpwm_HWAccess[STPWM_INT_ENAB] |=
	         				                STPWM_COMPARE1_INT;          /* Enable Compare1 interrupt */  
	                   stpwm_HWAccess[STPWM_INT_ACKN] |=
         			          	            STPWM_COMPARE1_INT;          /* Ack the Compare1 to clear*/     
	           }
	           else
	                   stpwm_HWAccess[STPWM_INT_ENAB] &= 
	        				                ~STPWM_COMPARE1_INT ;         /* Disable Compare1 interrupt  */
        }
       
    }
    STOS_InterruptUnlock();                     /* end of atomic region */
    return( ST_NO_ERROR );
}
    
                    
/****************************************************************************
Name         : STPWM_SetCallback()

Description  : This API sets/unsets the callback rountine for notifying the
			   user in case of any interrupt.
			   The Callbackparams will contain the information about the
			   associated callback and callback data pointer

Parameters   : STPWM_Handle_t Handle is passed, plus an
               *CallbackParams for defining a callback plus the PWM mode

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      MarkValue out of valid range
               ST_ERROR_INVALID_HANDLE     Handle number mismatch

See Also     : STPWM_Handle_t
 ****************************************************************************/
ST_ErrorCode_t STPWM_SetCallback(STPWM_Handle_t  		 Handle,
                                 STPWM_CallbackParam_t   *CallbackParams,
                                 STPWM_Mode_t            PWMUsedFor)
{
   if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle not open? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }
    if(IsOutsideRange(PWMUsedFor,
    				  STPWM_Timer,
    				  STPWM_Compare1 ))
    {
    	 return( ST_ERROR_BAD_PARAMETER );
    }  

    CallbackInfo[PWMUsedFor].Handle = Handle;
    CallbackInfo[PWMUsedFor].PWMMode = PWMUsedFor;
    CallbackInfo[PWMUsedFor].CallbackParam = *CallbackParams;
    
    return( ST_NO_ERROR );
}
                                                                        
/****************************************************************************
Name         : STPWM_Close()

Description  : Closes a channel from operation by calling
               STPIO_Close and resetting the internal note
               of channels open.

Parameters   : STPWM_Handle_t matching that supplied with Open call.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Different from Open
               STPWM_ERROR_PIO             Error occured in subcomponent STPIO

See Also     : STPWM_Open()
 ****************************************************************************/

ST_ErrorCode_t STPWM_Close( STPWM_Handle_t Handle )
{
    int Index;                              /* PioHandle subscript */
    ST_ErrorCode_t stpio_Return;            /* STPIO_Close return code */

    switch( Handle )
    {
        case STPWM_HANDLE_NO_0:
        	 
             Index = STPWM_CHANNEL_0;
             break;

        case STPWM_HANDLE_NO_1:
        	 
             Index = STPWM_CHANNEL_1;
             break;

        case STPWM_HANDLE_NO_2:
             Index = STPWM_CHANNEL_2;
            break;
        default:
            return( ST_ERROR_INVALID_HANDLE );
    }

    if( ( Handle & stpwm_ChansOpen ) == 0 ) /* handle already closed? */
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( PioChannelInUse[Index] == TRUE )
    {
        /* ask PIO to close the PWM channel */
        stpio_Return = STPIO_Close( PioHandle[Index] );

        if( stpio_Return != ST_NO_ERROR )
        {
            return( STPWM_ERROR_PIO );      /* return generic PIO error code */
        }
    }
    stpwm_ChansOpen &= ~Handle;             /* flag PWM channel closed */
    stpwm_PWMModeUsed[Index] = 0xFF;

    return( ST_NO_ERROR );
}

/****************************************************************************
Name         : STPWM_Term()

Description  : Terminates the Pulse Width Modulator Driver, effectively
               reversing the Init call.

Parameters   : ST_DeviceName_t *DeviceName must match that supplied to Init,
               STPWM_TermParams_t *Params just contains a BOOL to optionally
               force termination with open Handles iff TRUE.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      One or both parameters naff
               ST_ERROR_UNKNOWN_DEVICE     Mismatched Device Name
               ST_ERROR_OPEN_HANDLE        Open Handle(s) with force FALSE
               STPWM_ERROR_PIO             Error occurred in subcomponent PIO

See Also     : STPWM_InitParams_t
               STPWM_Close()
               STPWM_Init()
               STPWM_Open()
 ****************************************************************************/

ST_ErrorCode_t STPWM_Term( const ST_DeviceName_t Name,
                           const STPWM_TermParams_t *TermParams )
{
    int i;
    int ChansOpen;                          /* local copy used if forced */
    ST_ErrorCode_t stpio_Return;            /* STPIO_Close return code */

    if( ( Name       == NULL )   ||         /* NULL Name ptr? */
        ( TermParams == NULL ) )            /* NULL structure ptr? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    if( ( stpwm_DeviceName[0] == '\0' ) ||  /* not Initialized? */
        ( strcmp( Name,
            stpwm_DeviceName ) != 0 ) )     /* different Name from Init? */
    {
        return( ST_ERROR_UNKNOWN_DEVICE );
    }

    if( stpwm_ChansOpen !=
                STPWM_HANDLE_NONE )         /* any channels open? */
    {
        if( !TermParams->ForceTerminate )   /* no force applied? */
        {
            return( ST_ERROR_OPEN_HANDLE ); /* user needs to call Close */
        }

        /* if we reach this point, we have
           one or more handles to force
           closed before effecting Term    */

        ChansOpen = stpwm_ChansOpen;             /* initialize local copy */

        for( i = STPWM_CHANNEL_0;
                i < STPWM_CHANNEL_CNT;
                i++, ChansOpen >>= 1 )           /* loop on all channels */
        {
            if( ( ChansOpen &
                STPWM_HANDLE_NO_0 ) != 0 )       /* open channel? */
            {
                if( PioChannelInUse[i] == TRUE )
                {
                    stpio_Return =               /* translate to PIO Handle */
                      STPIO_Close( PioHandle[i] );

                    if( stpio_Return !=
                        ST_NO_ERROR )            /* STPIO_Close failure? */
                    {
                      return( STPWM_ERROR_PIO ); /* return generic PIO error */
                    }
                }
                stpwm_ChansOpen &=
                  ~stpwm_PWMEnc[i];       /* close individually in case fail */
		  stpwm_PWMModeUsed[i] = 0xFF;
         
            }
        }
    }

    /* Ensure atomic operation between reading HW register
       and writing back as this registers is shared
       with STCOUNT */
    STOS_InterruptLock();
    stpwm_HWAccess[STPWM_CONTROL] &=
                        ~STPWM_PWM_ENAB | ~STPWM_CAPTURE_ENAB;    /* reset PWM and capture enable bit */
    STOS_InterruptUnlock();
    
    STOS_InterruptLock();
    if (STOS_InterruptDisable(stpwm_C4InPars_p->InterruptNumber,
    						  stpwm_C4InPars_p->InterruptLevel) == 0/*STOS_SUCCESS*/)
    {
       if ( STOS_InterruptUninstall(stpwm_C4InPars_p->InterruptNumber, 
       		stpwm_C4InPars_p->InterruptLevel,
       		(void*)0)== 0 /*STOS_SUCCESS*/)
       {
            STOS_InterruptUnlock();
       }
       else
       {
            STOS_InterruptUnlock();
            return (ST_ERROR_INTERRUPT_UNINSTALL);
       }
    }
    else
    {
         STOS_InterruptUnlock();
         return (ST_ERROR_BAD_PARAMETER);
    }
    
    stpwm_DeviceName[0] = '\0';             /* flag device closed */

    return( ST_NO_ERROR );
}


/****************************************************************************
Name         : MaskToIndex

Description  : Converts valid mask values into
               STPIO_BitConfig_t BitConfig array subscript.

Parameters   : STPIO_BitMask_t in range PIO_BIT_0 to
               PIO_BIT_7.

Return Value : 0 to 7 Index value if valid input
               -1 if invalid input
 ****************************************************************************/
static int MaskToIndex( const STPIO_BitMask_t BitMask )
{
    int i;

    STPIO_BitMask_t CurMask = BitMask;

    if( BitMask == 0 )
    {
        return( STPWM_UNUSED_MASK );
    }

    for( i = 0; i < STPWM_PIO_BIT_CNT; i++, CurMask >>= 1 )
    {
        if( ( CurMask & PIO_BIT_0 ) != 0 )
        {
            if( CurMask == PIO_BIT_0 )
            {
                return( i );
            }
            else
            {
                return( STPWM_BAD_BIT_MSK );
            }
        }
    }

    return( STPWM_BAD_BIT_MSK );
}


/****************************************************************************
Name         : IsOutsideRange()

Description  : Checks if U32 formal parameter 1 is outside the range of
               bounds (U32 formal parameters 2 and 3).

Parameters   : Value for checking, followed by the two bounds, preferably
               the lower bound followed by the upper bound.

Return Value : TRUE  if parameter is outside given range
               FALSE if parameter is within  given range
 ****************************************************************************/

static BOOL IsOutsideRange( U32 ValuePassed,
                            U32 LowerBound,
                            U32 UpperBound )
{
    U32 Temp;

    /* ensure bounds are the correct way around */
    if( UpperBound < LowerBound ) /* bounds require swapping */
    {
        Temp       = LowerBound;
        LowerBound = UpperBound;
        UpperBound = Temp;
    }

    if( ( ValuePassed < LowerBound ) ||
        ( ValuePassed > UpperBound ) )
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
}

/******************************************************************************
* Function Name:  PWMInterruptHandler
* Returns:        Void
* Parameters:     STPIO_Init_t *PIO
* Use:            The interrupt handler for the PWM driver.
******************************************************************************/

STOS_INTERRUPT_DECLARE( PWMInterruptHandler , Param ) 
{
	U32  StatusVal;
	STPWM_Mode_t Index = 0;

        UNUSED_PARAMETER(Param);

	StatusVal = stpwm_HWAccess[STPWM_INT_STAT];

   if(((stpwm_HWAccess[STPWM_INT_STAT]) & STPWM_PWM_INT)==	STPWM_PWM_INT)
    {
             Index =  STPWM_Timer;

       if(((stpwm_HWAccess[STPWM_INT_ENAB]) & STPWM_PWM_INT)== STPWM_PWM_INT)
        {
             stpwm_HWAccess[STPWM_INT_ACKN] = STPWM_PWM_INT;
             /* ackn. PWM timer interrupt to clr */
             
	        if (CallbackInfo[Index].CallbackParam.CallBack!= 0)
	        {
#if defined (WA_GNBvd54182) || defined (WA_GNBvd56512)
/* HDCP WA and ACR packet generation WA*/
/* This code is for skiping interrupts 3 times*/
	            if(IntFirstTime)
	            {
	                InterTime= *((volatile int*) 0xb8010050);
	                IntFirstTime=0;
	            }
	            if(Skip%(Nb_TIMES)==2)
	            {
	                *((volatile int*) 0xb8010050)=InterTime+1;
	            }
	            if(Skip%Nb_TIMES==0)
	            {
	                *((volatile int*) 0xb8010050)=InterTime;
#if defined(WA_GNBvd54182)
	                CallbackInfo[Index].CallbackParam.CallBack(CallbackInfo[Index].CallbackParam.CallbackData_p);
#endif/*WA_GNBvd54182*/
	                Skip=0;
	            }
#endif/*(WA_GNBvd54182) || defined (WA_GNBvd56512)*/
#if !defined(WA_GNBvd54182)
	            CallbackInfo[Index].CallbackParam.CallBack(CallbackInfo[Index].CallbackParam.CallbackData_p);
#endif
	            Skip++;
	       }
	    }
    }
    if(((stpwm_HWAccess[STPWM_INT_STAT]) & STPWM_CAPTURE0_INT)== STPWM_CAPTURE0_INT)
    {
        if(((stpwm_HWAccess[STPWM_INT_ENAB]) & STPWM_CAPTURE0_INT)== STPWM_CAPTURE0_INT)
        {
	        stpwm_HWAccess[STPWM_INT_ACKN] = STPWM_CAPTURE0_INT;
	        /* ackn. PWM Capture channel 0 interrupt to clr */
	        Index =  STPWM_Capture0;
	
	        if(CallbackInfo[Index].CallbackParam.CallBack!= 0)
	        {
	           CallbackInfo[Index].CallbackParam.CallBack(CallbackInfo[Index].CallbackParam.CallbackData_p);
	        }
        }
    }
    if(((stpwm_HWAccess[STPWM_INT_STAT]) & STPWM_CAPTURE1_INT)== STPWM_CAPTURE1_INT)
    {
    	if(((stpwm_HWAccess[STPWM_INT_ENAB]) & STPWM_CAPTURE1_INT)== STPWM_CAPTURE1_INT)
        {
       	    stpwm_HWAccess[STPWM_INT_ACKN] = STPWM_CAPTURE1_INT;
	        Index =  STPWM_Capture1;
	        /* ackn. PWM Capture channel 1 interrupt to clr */
	        if(CallbackInfo[Index].CallbackParam.CallBack!= 0)
	        {
	           CallbackInfo[Index].CallbackParam.CallBack(CallbackInfo[Index].CallbackParam.CallbackData_p);
	        }
	    }
    }
    if(((stpwm_HWAccess[STPWM_INT_STAT]) & STPWM_COMPARE0_INT)== STPWM_COMPARE0_INT)
    { 

   	if(((stpwm_HWAccess[STPWM_INT_ENAB]) & STPWM_COMPARE0_INT)== STPWM_COMPARE0_INT)
    	{
	        stpwm_HWAccess[STPWM_INT_ACKN] = STPWM_COMPARE0_INT;
	        Index =  STPWM_Compare0;
	        /* ackn. PWM Compare channel 1 interrupt to clr */
	        if(CallbackInfo[Index].CallbackParam.CallBack!= 0)
	        {
	           CallbackInfo[Index].CallbackParam.CallBack(CallbackInfo[Index].CallbackParam.CallbackData_p);
	        }
        }
    }
    if(((stpwm_HWAccess[STPWM_INT_STAT]) & STPWM_COMPARE1_INT)== STPWM_COMPARE1_INT)
    {
    	if(((stpwm_HWAccess[STPWM_INT_ENAB]) & STPWM_COMPARE1_INT)== STPWM_COMPARE1_INT)
    	{
       	     stpwm_HWAccess[STPWM_INT_ACKN] = STPWM_COMPARE1_INT;
	        Index =  STPWM_Compare1;
	        /* ackn. PWM Compare channel 1 interrupt to clr */
	        if(CallbackInfo[Index].CallbackParam.CallBack!= 0)
	        {
	           CallbackInfo[Index].CallbackParam.CallBack(CallbackInfo[Index].CallbackParam.CallbackData_p);
	        }
	    }
    }

#if defined(ST_OSLINUX)
    return IRQ_HANDLED;
#elif defined (ST_OS21)
    return(OS21_SUCCESS);
#else
    return;
#endif
}
