/*{{{ COMMENT Standard Header*/
/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 1995

Source file name : st20nvm.h
Author :           S.R.Christian


Date        Modification                                    Initials
----        ------------                                    --------
15-Aug-95   Created                                         SRC
25-MAY-98   GPV, KS
            CV       Adapted for Diversity board
************************************************************************/
/*}}}*/

#ifndef  __NVM_H__
#define  __NVM_H__


#include "appltype.h"
/* external routine definitions from associated module */

#define  CH_NVM_NAME_LENGTH         12
typedef   struct tagNvmBlockHeader
{
	BYTE   aucBlockName [ CH_NVM_NAME_LENGTH ]; /* 12 bytes long block name ends with '\0' */
	USHORT usMagicValue;    /* 16 bit magic value - 0x7234 */
	USHORT usRecordLength;  /* length of each record in 4 bytes boundary */
	USHORT usNoOfRecords;   /* total no of records in this block */
	USHORT usBlockLength;   /* length of this block excluding header, this is equivalent to ( usRecordLength * usNoOfRecords ) */
} CH_NVM_BLOCK_HEADER;

typedef struct tagNvmRootBlockData
{
	INT iNoOfBlocksAvailable : 4; /* total no of blocks in the whole NVM */
	INT biFlashDataValidity : 1;  /* FALSE = Flash Data is invalid */
	INT abiUnused : 27;           /* unused 27 bits */
} CH_NVM_ROOT_BLOCK_DATA;

/* length of header length */
#define CH_NVM_HEADER_LENGTH         ( sizeof ( CH_NVM_BLOCK_HEADER ) )
/* length of 'root' block data */
#define CH_NVM_ROOT_BLOCK_LENGTH     ( sizeof ( CH_NVM_ROOT_BLOCK_DATA ) )






enum Update_Type_t
{
	xpdr_type=0, 
     bjdbase_type,
	service_type	
};

typedef  int (*PTR_2_HOOK_FUNCTION) ( S32 iParam1, S32 iParam2 );


extern BOOLEAN  CH_NVMUpdate ( opaque_t nvmid );
extern int InitialisePWMInfo ( INT iParam1, INT iParam2 );
extern int InitialiseNvmBoxInfo ( INT iParam1, INT iParam2 );
extern BOOLEAN CH_NVMDRVInit (void);
extern BOOL CH_DBASEMGRINIT(void);

extern S8 Reset(void);

#endif   /* __NVM_H__ */

