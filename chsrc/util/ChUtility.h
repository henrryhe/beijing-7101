#ifndef __CHUTILITY_H__
#define __CHUTILITY_H__

/*****************************************************************************
  include
 *****************************************************************************/
typedef  struct  tDateInfo
{
   int   TYear;
   int   TMonth;
   int   TDay;
}tDateInfoType;


typedef struct
{
     SHORT                     OPINo;        /*OPI Number*/
     U8                           OPIName[16];   /*OPI name*/
     U8                           geography_zero;/*OPI地理区域*/
     tDateInfoType          Subdate;    /*定购期限*/
     U8                           CaOffer[8];    /*CA OFFER*/
}OPIInfoStr;


typedef   enum
{
      CHCA_UnknownStatus,
      CHCA_NoPair,
      CHCA_PairError,
      CHCA_PairOK
}CHCA_PairingStatus_t;



/*******************************************************************************
 *Function name: CHCA_GetStbID
 *           
 *
 *Description: the general interface for getting the stb id of the box
 *             
 *             
 *    
 *Prototype:
 *     CHCA_BOOL  CHCA_GetStbID(CHCA_UCHAR*   iStbID)
 *           
 *
 *
 *input:
 *      CHCA_UCHAR*   iStbID  : the pointer of the stb id data 
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *      true: interface execution error
 *      flse: interface execution ok
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
extern BOOL  CHCA_GetStbID(U8*   iStbID);



/*******************************************************************************
 *Function name: CHCA_GetOperatorInfo
 * 
 *
 *Description: get the operator information 
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  CHCA_GetOperatorInfo(OPIInfoStr*  OpiInfo,CHCA_UINT*  OpiNum)          
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *           false:  the function execution ok
 *           true:  the function execution error
 *
 *Comments:
 *    SHORT                     OPINo;                   ::   'OPI Number
 *    U8                           OPIName[16];         ::   OPI name
 *    U8                           geography_zero;     ::    OPI地理区域
 *    SHORT                     Subdate;                ::    定购期限
 *    U8                           CaOffer[8];             ::CA OFFER
 * 
 * 
 *******************************************************************************/
extern BOOL  CHCA_GetOperatorInfo(OPIInfoStr*  OpiInfo,U32*  OpiNum);


/*******************************************************************************
 *Function name: CHCA_AppEnterOperation
 * 
 *
 *Description: check the right of entering into the application
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  CHCA_AppEnterOperation(CHCA_INT  iProgramIndex)          
 *
 *input:
 *      CHCA_INT  iProgramIndex  : the index of the program 
 * 
 *
 *output:
 *
 *Return Value:
 *           false:  can not enter into the app
 *           true:  can enter into the app
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
extern U32  CHCA_AppEnterOperation(U32  iProgramIndex);


#if 0
/*******************************************************************************
 *Function name: CHCA_CheckOfferRight
 * 
 *
 *Description: check the rigth of the offer
 *                  
 *
 *Prototype:
 *     CHCA_BOOL CHCA_CheckOfferRight(Authorized_offers_des *Authorized_offers)
 *
 *input:
 *     Authorized_offers_des *Authorized_offers,CA OFFER信息
 *     CHCA_USHORT   iOpiNum  : the total number of the operator
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     typedef struct
 *     {
 *	      U8  Stream_to_cut;
 *	      U16 OPI ; 
 *	      unsigned char Offers_Map[8];
 *     }Authorized_offers_des;
 *
 * 
 * 
 *******************************************************************************/
extern BOOL CHCA_CheckOfferRight(Authorized_offers_des *Authorized_offers,U16   iOpiNum);


/*******************************************************************************
 *Function name: CHCA_CheckGeoGraphicalRight
 * 
 *
 *Description: check the rigth of the Geographical
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  CHCA_CheckGeoGraphicalRight(Conditionnal_actions_des  *conditional_action)
 *
 *input:
 *     Authorized_offers_des *Authorized_offers,CA OFFER信息
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *   typedef struct
 *   {  
 *	    U16              OPI;
 *	    U8                Cur_zone_number;
 *	    U8                Geographical_zone_numbe[MAX_GEOGRAPHICAL_ZONE_NUM];
 *	    Action_des     action[MAX_GEOGRAPHICAL_ZONE_NUM];
 *   }Conditionnal_actions_des;
 *
 * 
 * 
 *******************************************************************************/

/*******************************************************************************
 *Function name: CHCA_CheckCountryRight
 * 
 *
 *Description: check the rigth of the country
 *                  
 *
 *Prototype:
 *     CHCA_BOOL CHCA_CheckCountryRight(Country_availability_des  *Country_availability)
 *
 *input:
 *     Country_availability_des  *Country_availability
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 * typedef struct
 *{
 *  	 BOOL Country_availability_flag[MAX_LANGUAGE_ID];
 * }Country_availability_des;
 *
 * 
 * 
 *******************************************************************************/

/*******************************************************************************
 *Function name: CHCA_CheckCountryRight
 * 
 *
 *Description: check the rigth of the ParentalRight
 *                  
 *
 *Prototype:
 *     CHCA_BOOL CHCA_CheckCountryRight(Country_availability_des  *Country_availability)
 *
 *input:
 *     Country_availability_des  *Country_availability
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
typedef struct
{
   U8 Rating[MAX_LANGUAGE_ID]; 
                                                 *0x00	Undefined
							0x01 to 0x0F	minimum age = rating + 3 years
							0x10	(priv) Code CSA all
							0x11	(priv) Code CSA Accord parental souhaitable
							0x12	(priv) Code CSA -12 ans
							0x13	(priv) Code CSA -16 ans
							0x14	(priv) Code CSA -18 ans / pour adultes seulement
							0x15 to 0xFF	Defined by the broadcaster
 }Parental_rating_des; 
 * 
 * 
 *******************************************************************************/
extern BOOL CHCA_CheckParentalRight(Parental_rating_des  *Parental_rating);
#endif

#endif  /*__CHUTILITY_H__*/

