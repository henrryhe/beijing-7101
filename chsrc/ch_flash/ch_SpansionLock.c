#ifndef CH_LDR_PROTECT
#include "stddefs.h"
#include "lld_wsxxx_asp.h"/**/
#include "lld.h"
#include "CH_FlashMid.h"


#define FLASH_BASE_ADDRESS      0xA0000000
#define FLASH_BLOCK_COUNT       256
#define FLASH_BLOCK_SIZE            0x20000

/////////////////////////////spansion  numonyx/////////////////////////////////////

//加上offset块的DYB保护，写完offset块调用
void lld_DybProgramOp
(
FLASHDATA *   base_addr,	/* device base address in system */
ADDRESS   offset			/* sector offset address */
)
{
	lld_DybEntryCmd(base_addr);
	lld_DybSetCmd(base_addr, offset);
	lld_DybExitCmd(base_addr);
}

//清除offset块的DYB保护，写offset块之前调用
void lld_DybClrOp
(
FLASHDATA *   base_addr,	/* device base address in system */
ADDRESS   offset			/* sector offset address */
)
{
	lld_DybEntryCmd(base_addr);
	lld_DybClrCmd(base_addr, offset);
	lld_DybExitCmd(base_addr);
}

//读取DYB状态
FLASHDATA CH_LDR_ReadDybStatus
(
FLASHDATA *   base_addr,	/* device base address in system */
ADDRESS   offset			/* sector offset address */
)
{
	FLASHDATA status;
	lld_DybEntryCmd(base_addr);
	status = lld_DybReadCmd(base_addr, offset);
	lld_DybExitCmd(base_addr);
	return status;
}

//DYB保护Flash所有块,Flash初始化时调用
void CH_LDR_DybPAllSectors()
{
    int i;
    
	for(i = 0; i < FLASH_BLOCK_COUNT; i++)
	{
		 lld_DybProgramOp((FLASHDATA*)FLASH_BASE_ADDRESS,  (ADDRESS)(i*FLASH_BLOCK_SIZE)/2);
		 //DBG_DRV_Printf("flash sector >>%d<< is DYB Protected now!\n", i);
	}

}
void CH_LDR_DybUnPAllSectors()
{
    int i;
    
	for(i = 0; i < FLASH_BLOCK_COUNT; i++)
	{
		 lld_DybClrOp((FLASHDATA*)FLASH_BASE_ADDRESS,  (ADDRESS)(i*FLASH_BLOCK_SIZE)/2);
		 //DBG_DRV_Printf("flash sector >>%d<< is DYB Protected now!\n", i);
	}

}


void CH_LDR_SetFirst2BlockPPBBits( void )
{

        lld_PpbProgramOp((FLASHDATA*)FLASH_BASE_ADDRESS,0);
        lld_PpbProgramOp((FLASHDATA*)FLASH_BASE_ADDRESS,(ADDRESS)(FLASH_BLOCK_SIZE/2));   

}


//32MFlash，激活第0,1块OTP
void CH_LDR_EnableFirst2BlockOTP( void )
{
        FLASHDATA  PPBStatus1 = 1,PPBStatus2 = 1;
        int res = 0;

#if 0 	/*<!-- ZQ 2009/6/24 13:16:06 --!>*/
        lld_PpbProgramOp((FLASHDATA*)FLASH_BASE_ADDRESS,0);
        lld_PpbProgramOp((FLASHDATA*)FLASH_BASE_ADDRESS,(ADDRESS)(FLASH_BLOCK_SIZE/2));       
        lld_PpbAllEraseOp((FLASHDATA*)FLASH_BASE_ADDRESS);       
#endif 	/*<!-- ZQ 2009/6/24 13:16:06 --!>*/

        //第0块PPB状态，0为激活，1为未激活
        PPBStatus1 = lld_PpbStatusReadOp((FLASHDATA*)FLASH_BASE_ADDRESS,0);
        //第1块PPB状态
        PPBStatus2 = lld_PpbStatusReadOp((FLASHDATA*)FLASH_BASE_ADDRESS,(ADDRESS)(FLASH_BLOCK_SIZE/2));

        //如果两块的PPB状态均激活,则set volatile PPB Lock Bit. 
        if((PPBStatus1 == 0) && (PPBStatus2 == 0))
        {
                res = lld_PpbLockBitSetOp ((FLASHDATA*)FLASH_BASE_ADDRESS);
              
	          res = lld_PpbLockBitSetOp ((FLASHDATA*)(FLASH_BASE_ADDRESS+FLASH_BLOCK_SIZE/2));
                printf("lld_PpbLockBitSet OK!\n");
        }
        
#if 0 	/*<!-- ZQ 2009/6/24 13:44:19 --!>*/
         lld_PpbAllEraseOp((FLASHDATA*)FLASH_BASE_ADDRESS);      
#endif 	/*<!-- ZQ 2009/6/24 13:44:19 --!>*/

}

boolean CH_LDR_GetFirst2BlockOTPStatus( void )
{
        FLASHDATA  LockStatus1 = 0,LockStatus2 = 0;
        int res = 0;

#if 0 	/*<!-- ZQ 2009/6/24 13:16:06 --!>*/
        lld_PpbProgramOp((FLASHDATA*)FLASH_BASE_ADDRESS,0);
        lld_PpbProgramOp((FLASHDATA*)FLASH_BASE_ADDRESS,(ADDRESS)(FLASH_BLOCK_SIZE/2));       
        lld_PpbAllEraseOp((FLASHDATA*)FLASH_BASE_ADDRESS);       
#endif 	/*<!-- ZQ 2009/6/24 13:16:06 --!>*/

        //第0块LOCK状态，1为激活，0为未激活
        LockStatus1 = lld_PpbLockBitReadOp ((FLASHDATA*)FLASH_BASE_ADDRESS);
        //第1块LOCK状态
        LockStatus2 = lld_PpbLockBitReadOp((FLASHDATA*)(FLASH_BASE_ADDRESS+FLASH_BLOCK_SIZE/2));

        if((LockStatus1 == 0) && (LockStatus2 == 0))
        {
                //OTP打开
                return 1;
                
        }
        else
        {
                //OTP未打开
                return 0;
        }

}

/////////////////////////////spansion  numonyx/////////////////////////////////////

void  CH_LDR_BlockUnlock(U32 Blockid)
{
     lld_DybClrOp((FLASHDATA*)FLASH_BASE_ADDRESS,(ADDRESS)(Blockid * FLASH_BLOCK_SIZE)/2);
}
    

void  CH_LDR_BlockLock(U32 Blockid)
{
      lld_DybProgramOp((FLASHDATA*)FLASH_BASE_ADDRESS,(ADDRESS)(Blockid * FLASH_BLOCK_SIZE)/2);
}


#endif
