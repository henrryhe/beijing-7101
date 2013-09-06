/*******************文件说明注释************************************/
/*公司版权说明：版权（2007）归长虹网络科技有限公司所有。           */
/*文件名：ch_tkdma.c                                               */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2007-07-19                                             */
/*模块功能:完成控制字加解密控制                                    */
/*主要函数及功能:                                                  */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*  

                                                                   */
/*******************************************************************/

/**************** #include 部分*************************************/
#include "stddefs.h"            
#include "stlite.h"              
#include "ch_tkdma.h"

boolean CH_tkd_init(void);
void clear_word_dma_example_code(void);

/*******************************************************************/
/**************** #define 部分**************************************/
#define ASSERT(X) if (!(X)) { printf("Assert at %d\n", __LINE__); return; }
/*******************************************************************/
/***************  全局变量定义部分 *********************************/

STTKDMA_Key_t StandardKey1 =
{{0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf}};
STTKDMA_Key_t StandardKey2 =
{{0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11}};


U8* SourceBuffer = (U8*) 0xa4500000;
U8* DestBuffer =   (U8*) 0xa4520000;
U8* EncodedBuffer =(U8*) 0xa4600000;

/*
* Print memory contents in an easy to read format.
*/
void MemoryPrint(U8* Buffer, int Bytes)
{
	int Index;
	for (Index = 0; Index < Bytes; Index++)
	{
		if ((Index % 16) == 0) printf("0x%08x:", (int) Buffer+Index);
		do_report(0," %02x", *(Buffer+Index));
		if ((Index % 16) == 15) printf("\n");
	}
	if ((Index % 16) == 0)
		do_report(0,"\n");
}

void CH_TDK_TEST(void)
{
	ST_ErrorCode_t ErrorCode;
	// Initialise the dirver
	ErrorCode = CH_tkd_init();
	ASSERT(ErrorCode == ST_NO_ERROR);
	// Execute the examples
	// (This function will change depending on the example)
	//example_func();
	clear_word_dma_example_code();
	// Close down the driver
	
}
/*******************************************************************/
/************************函数说明***********************************/
/*函数名:boolean CH_tkd_init(void)                                 */
/*开发人和开发时间:zengxianggen 2007-07-08                         */
/*函数功能描述:完成STTKDMA模块的初始化                             */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */

boolean CH_tkd_init(void)
{
	ST_ErrorCode_t st_ErrorCode;
	st_ErrorCode = STTKDMA_Init("TKDma", 12);
#if 0
	/* set input keys to work same way pre version 1.0.? */
	st_ErrorCode = STTKDMA_ConfigureTK(STTKDMA_TKCFG_LEGACY_KEYS, TRUE);
	/* set TKD keys lader to work same way pre version 1.0.? */
	st_ErrorCode = STTKDMA_ConfigureTK(STTKDMA_TKCFG_ALT_FORMAT, FALSE);
	/* set TKDma keys lader to work same way pre version 1.0.? */
	st_ErrorCode = STTKDMA_ConfigureTK(STTKDMA_TKCFG_DMA_ALT_FORMAT, FALSE);
#endif
	
}

void clear_word_dma_example_code(void)
{
/*
* This function demonstrates DMA encryption/decryption with TKD disabled parts. The encryption key
* is supplied by software. It encrypts a source buffer to an encoded buffer and then decrypts the
* encoded buffer to a destination buffer. The source and destination buffer should be identical.
*
* The output produced by this function should be
* 0xa4500000: 00 01 02 03 04 05 06 07 11 11 11 11 11 11 11 11
* 0xa4500010: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4500020: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4500030: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4500040: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4500050: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4500060: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4500070: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4500080: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4500090: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
*
* 0xa4600000: 71 b1 00 d3 50 ca e0 f5 c3 69 40 d0 9d 1a d4 75
* 0xa4600010: c3 69 40 d0 9d 1a d4 75 c3 69 40 d0 9d 1a d4 75
* 0xa4600020: c3 69 40 d0 9d 1a d4 75 c3 69 40 d0 9d 1a d4 75
* 0xa4600030: c3 69 40 d0 9d 1a d4 75 c3 69 40 d0 9d 1a d4 75
* 0xa4600040: c3 69 40 d0 9d 1a d4 75 c3 69 40 d0 9d 1a d4 75
* 0xa4600050: c3 69 40 d0 9d 1a d4 75 c3 69 40 d0 9d 1a d4 75
* 0xa4600060: c3 69 40 d0 9d 1a d4 75 c3 69 40 d0 9d 1a d4 75
* 0xa4600070: c3 69 40 d0 9d 1a d4 75 c3 69 40 d0 9d 1a d4 75
* 0xa4600080: 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22
* 0xa4600090: 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22
*
* 0xa4520000: 00 01 02 03 04 05 06 07 11 11 11 11 11 11 11 11
* 0xa4520010: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4520020: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4520030: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4520040: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4520050: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4520060: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4520070: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
* 0xa4520080: 33 33 33 33 33 33 33 33 33 33 33 33 33 33 33 33
* 0xa4520090: 33 33 33 33 33 33 33 33 33 33 33 33 33 33 33 33
*
* The encrypt operation is identical to
* tdes -e afaeadacabaaa9a8 a7a6a5a4a3a2a1a0 0302010007060504 (at 0xa4600000)
* Result = 0xd300b171.f5e0ca50
*
* Note the repeating pattern in the encoded buffer. This can be
* eliminated by using TKDMA_ALGORITHM_MODE_CBC instead
* of TKDMA_ALGORITHM_MODE_ECB.
*
* Note2: To use AES rather than TDES, replace TKDMA_ALGORITHM_TDES
* with TKDMA_ALGORITHM_AES.
	*/
	ST_ErrorCode_t ErrorCode;
	U32 DmaSize = 128; /* Size of buffer, must be multiple of 32, minimum 128 */
	U8 CWIndex = 0; /* This encoding will use key index 0 */
	STTKDMA_DMAConfig_t DMAConfig; /* Config parameters for the DMA */
	STTKDMA_Key_t EncryptKey; /* Key to use for encryption/decryption */
	STTKDMA_Key_t OffsetKey; /* Not used */
	printf("clear_word_dma_example_code() ---------------------------------------\n");
	/* Get a copy of the standard encryption key */
	memcpy(&EncryptKey, &StandardKey1, sizeof(STTKDMA_Key_t));
	ErrorCode = STTKDMA_DecryptKey(STTKDMA_COMMAND_DIRECT_CPCW, &EncryptKey, 0, &OffsetKey);
	ASSERT(ErrorCode == ST_NO_ERROR);
	/* Configure the DMA for TDES encryption. */
	DMAConfig.algorithm = STTKDMA_ALGORITHM_TDES;
	DMAConfig.algorithm_mode = STTKDMA_ALGORITHM_MODE_ECB;
	DMAConfig.refresh = TRUE;
	DMAConfig.decrypt_not_encrypt = FALSE;
	DMAConfig.sck_overrride = FALSE;
	/* As an example, fill the buffer with a test pattern. */
	memset(SourceBuffer, 0x11, 0x1000);
	memcpy(SourceBuffer, "\x00\x01\x02\x03\x04\x05\x06\x07", 8);
	memset(EncodedBuffer, 0x22, 0x1000);
	memset(DestBuffer, 0x33, 0x1000);
	MemoryPrint(SourceBuffer, DmaSize+32);
	/* Encrypt the source buffer to the encoded buffer. */
	ErrorCode = STTKDMA_StartDMA(&DMAConfig, SourceBuffer, EncodedBuffer, DmaSize, CWIndex, TRUE);
	ASSERT(ErrorCode == ST_NO_ERROR);
	MemoryPrint(EncodedBuffer, DmaSize+32);
	/* Decrypt the encoded buffer to the destination buffer. */
	DMAConfig.decrypt_not_encrypt = TRUE;
	ErrorCode = STTKDMA_StartDMA(&DMAConfig, EncodedBuffer, DestBuffer, DmaSize, CWIndex, TRUE);
	ASSERT(ErrorCode == ST_NO_ERROR);
	MemoryPrint(DestBuffer, DmaSize+32);
}

/* load clear key in to pti key slot 0 */
void simple_tkd_clear_key_code()
{
	ST_ErrorCode_t ErrorCode;
	U8 cw_index;
	STTKDMA_Key_t pti_offset;
	cw_index = 0;
	ErrorCode = STTKDMA_DecryptKey(STTKDMA_COMMAND_DIRECT_CW,
		&StandardKey1, cw_index, &pti_offset);
}

/*函数名:void CH_CA_LoadCW( U8* u_key,int i_cwIndex )                         */
/*开发人和开发时间:zengxianggen 2007-07-19                                    */
/*函数功能描述:20070718 增加得到加密芯片解密后的KEY                           */
/*函数原理说明:                                                               */
/*输入参数：U8* u_key,输入KEY                                                 */
/*输出参数:                                                                   */
/*使用的全局变量、表和结构：                                                  */
/*调用的主要函数列表：                                                        */
/*返回值说明：无                                                              */   
/*调用注意事项:                                                               */
/*其他说明:                                                                   */
void CH_CA_LoadCW( U8* u_Inkey,int i_cwIndex,U8* u_Outkey )
{
	ST_ErrorCode_t st_ErrorCode;
	U8 cw_index;
	int i;
	STTKDMA_Key_t st_ptioffset;
    STTKDMA_Key_t st_key;
#if 0
    do_report(0,"u_key=");
	for(i=0;i<8;i++)
	{
		do_report(0,"%x,",u_Inkey[i]);
	}
    do_report(0,"\n");
#endif
	/*memcpy(StandardKey1.key_data,u_key,8);*/
	for(i=0;i<8;i++)
    {
	 st_key.key_data[i] = u_Inkey[7-i];
	}
	cw_index = i_cwIndex;
	st_ErrorCode = STTKDMA_DecryptKey(STTKDMA_COMMAND_DIRECT_CW,
		&st_key, cw_index, &st_ptioffset);
#if 0	
	do_report(0,"pti_offset.key_data=");
	for(i=0;i<8;i++)
	{
		do_report(0,"%x,",st_ptioffset.key_data[i]);
	}
#endif
	memcpy(u_Outkey,&st_ptioffset.key_data[0],8);
}
/*****************************************************************************/








