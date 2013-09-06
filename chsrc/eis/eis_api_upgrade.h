
typedef enum
{
  UPDATE_REPLACE_LOWER=0,  /*替换比新软件版本更低的软件(相同的manufacturer_id和Hardware_Id)*/
  UPDATE_REPLACE_ALL,      /*替换所有版本软件(相同的manufacturer_id和Hardware_Id)*/
  UPDATE_REPLACE_TARGET    /*替换目标版本软件(相同的manufacturer_id和Hardware_Id)*/

}UpdateType;

typedef struct tagDCT
{
    int transport_stream_id;
    int version_number;
    unsigned char global_configuration_version;
    unsigned char service_name_version;
    unsigned char manufacturer_id;
	
    int     usMagicNo ;            /* 16bit magic_id 0xA55A */
    unsigned char     hardware_id  ;
    unsigned char     abiSoftVersionMajor ;   /* 8bit softversion Major */
    unsigned char    abiSoftVersionMinor ;   /* 8bit softversion Minor */
    int     abiDateStamp;            /* 32bit date code in DD/MM/YYYY format */
    int     abiTimeStamp;            /* 32bit time code in xxHHMMSS format */
    int     iImageCRC;              /* 32bit CRC             */
    int     iImageSize;             /* total size of the image */
	
    int download_frequency;
    short download_symbol;
    short download_modulation;
    short download_pid;
	
    unsigned char Header_table_id;
    unsigned char mage_table_id;
	
    unsigned char Target_main_software;
    unsigned char Target_minor_software;
    /*Polarity_No	8
    LnbPower	8
    PolControl_No	8
	*/
	/*0	替换比新软件版本更低的软件(相同的manufacturer_id和Hardware_Id)
	1	替换所有版本软件(相同的manufacturer_id和Hardware_Id)
	2	替换目标版本软件(相同的manufacturer_id和Hardware_Id)
	*/
    UpdateType Download_type;
	
} CDTTable  ;
 typedef struct
 {
  int Freq;
  int Sym_Rate;
  int Pol;
  int pid;
  int tableid;
  int flash_corrupted;
 }UpdatePara;
typedef   struct  tagHL_INFORStruct
{
	U32     hardware_id;  
	U32     loader_ver_id;
	U32     uiImageStartAddress;    /*
									* image start address for FLASH it is
									* the ORed value of receive address and
									* 0x7FF00000
	*/
	U32      uiImageSize;            /* size of the downlload image */
	U32      ucSoftwareVerMajor;     /* software version major (8)*/
	U32      ucSoftwareVerMinor;     /* software version minor (4)*/
	U32      iDateCode;              /* datecode in DD/MM/YYYY format */
	U32      iTimeCode;
	U32      HL_CHECKSUM;
	U32      HL_INFOR_CHECKSUM;
	
}HL_INFOR ;

 

