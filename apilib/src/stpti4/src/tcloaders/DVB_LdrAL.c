/************************************************************************
COPYRIGHT (C) ST Microelectronics (R&D) 2008
All rights reserved.

Author :           Automatically generated file

Contains the transport controller code packed as a word array

************************************************************************/

#define TC_ID 18
#define TC_DVB 1
#define TC_PTI4_LITE 1
#define TC_CAROUSEL_SUPPORT 1
#define TC_WATCH_AND_RECORD_SUPPORT 1
#define TC_AES_DESCRAMBLE_SUPPORT 1
#define TC_AES_7100_ECB_WORKAROUND_GNBvd52114 1
#define TC_MULTI2_DESCRAMBLE_SUPPORT 1
#define TC_TSDEBUG_SUPPORT 1
#define TC_PROFILING 1

#define TRANSPORT_CONTROLLER_CODE_SIZE      2163

static char const VERSION[] = "\nTC Version STPTI_DVBTCLoaderAL \n\tRel: 427216. File: DVBTC4 Version: 0\n \n\tRel: 427216. File: TCINIT Version: 0\n \n\tRel: 427216. File: TS_HEADER Version: 0\n \n\tRel: 427216. File: SETUPDMA Version: 0\n \n\tRel: 427216. File: AF_ALTOUT Version: 0\n \n\tRel: 427216. File: AF_PROC Version: 0\n \n\tRel: 427216. File: CC_PROC Version: 0\n \n\tRel: 427216. File: SLT_SWIT Version: 0\n \n\tRel: 427216. File: ECMEMM Version: 0\n \n\tRel: 427216. File: RAW_PROC Version: 0\n \n\tRel: 427216. File: PES_PROC Version: 0\n \n\tRel: 427216. File: CAR_PROC Version: 0\n \n\tRel: 427216. File: RAMCAM_PROC Version: 0\n \n\tRel: 427216. File: DMA_UNLD Version: 0\n \n\tRel: 427216. File: SIG_DEC Version: 0\n \n\tRel: 427216. File: STAT_BLK Version: 0\n \n\tRel: 427216. File: STREAMID Version: 0\n \n\tRel: 427216. File: ERR_PROC Version: 0\n \n\tRel: 427216. File: COMMON Version: 0\n \n\tRel: 427216. File: TCDMA Version: 0\n \n\tRel: 427216. File: SET_SCRAM Version: 0\n";


#define TRANSPORT_CONTROLLER_VERSION    2


static const unsigned int transport_controller_code[] = {
	0x2c000040, 0x40003f80, 0x40009f00, 0x2fa67b04, 0x2c8001c0, 0x4020a888, 
	0x2c800000, 0x40002108, 0x40003888, 0x4020ab88, 0x2d200000, 0x40002010, 
	0x2d001800, 0x40002090, 0x2c000000, 0x4000cb00, 0x4000c100, 0x50003880, 
	0x50802100, 0x5100c100, 0x20400040, 0x20400080, 0x88001100, 0x2c000000, 
	0x4000cb00, 0x2c000040, 0x4000c100, 0x40008600, 0x40008680, 0x2c000000, 
	0x4020aa00, 0x4020ab00, 0x2e20b500, 0xe0000100, 0x44400500, 0x120001a0, 
	0xe0000240, 0x44400500, 0x4020a500, 0x4020a580, 0x4000c900, 0x40206f00, 
	0x2ca06a00, 0xe0000280, 0x44400440, 0x2c800000, 0x10800048, 0x08800008, 
	0x5000c100, 0x24400000, 0x88002e00, 0x5020ab80, 0x00800048, 0x88003a00, 
	0x10000040, 0x00800400, 0x98004100, 0x2c0003c0, 0x30000040, 0x10267000, 
	0x50c00000, 0x10800048, 0x08800008, 0x40400008, 0x2c000000, 0x4020ab80, 
	0x50a0a700, 0x5020a780, 0x10800048, 0x18000000, 0x4020a708, 0x4020a780, 
	0x51a0b104, 0x25c000db, 0x80006200, 0x2f004d04, 0xc007c000, 0x51a0b180, 
	0x2f005004, 0xc0077200, 0xe04000c0, 0x2fc00180, 0x2f005404, 0xc0080200, 
	0x25800098, 0x80005b00, 0x2f005804, 0xc007de00, 0x2d800000, 0x4020b118, 
	0xc0001700, 0x51808200, 0x52808280, 0x484010da, 0x484014ea, 0x484020da, 
	0x484024ea, 0xc0001700, 0x2c203800, 0x40206980, 0x2ca63804, 0x50a63b80, 
	0x2483ffc8, 0x00002008, 0x80007700, 0x51a0b000, 0x24004018, 0x80064e00, 
	0x25003fd8, 0x2d8000c0, 0x2ca63804, 0x58c01c42, 0x2403ffc8, 0x28400080, 
	0x80007700, 0x1080100d, 0x01800058, 0x88006f00, 0xc0064e00, 0x2d800040, 
	0x2f007a04, 0xc0077200, 0x2d400180, 0x4020a990, 0x2e002e80, 0x5ac01842, 
	0x26800428, 0x88008100, 0x2e002040, 0x58405c42, 0x24400000, 0x80008e00, 
	0x50a0b000, 0x25804008, 0x80008a00, 0x24803fc8, 0x01c00050, 0x80008e00, 
	0x26c00168, 0x80008e00, 0x298011d0, 0x88001700, 0x10c00100, 0x40001008, 
	0x11800040, 0x2f009304, 0xc0077200, 0xe0400000, 0x2d400180, 0x31000210, 
	0x21400190, 0x4020a310, 0x4020aa90, 0x58400042, 0x10000040, 0x48400042, 
	0x26800428, 0x88009f00, 0xc0001700, 0x2c380000, 0x2487ffd0, 0x2f00a304, 
	0xc006ff00, 0x5120a300, 0x24200010, 0x88066f00, 0x51a0a600, 0x258003d8, 
	0x01800058, 0x8800b000, 0x2d801100, 0x40003798, 0x2d800040, 0x40003898, 
	0x2d800000, 0x40003898, 0x2d800040, 0x2f00b304, 0xc0077200, 0x24100010, 
	0x8000b800, 0x5020b580, 0x20008000, 0x4020b580, 0x5220a580, 0x24000220, 
	0x8800c200, 0x5020a600, 0x24004000, 0x8000c700, 0x22000220, 0x4020a5a0, 
	0x26000060, 0x8800c700, 0x2d400180, 0x5120a680, 0x293c0010, 0x8003e300, 
	0xc000c900, 0x2c400180, 0x4020a380, 0x2d800000, 0x4020ab18, 0x5020a600, 
	0x258003c0, 0x25000c00, 0x8800d100, 0x25c000d8, 0x80010800, 0x25801000, 
	0x8800fa00, 0x5020a380, 0x25002000, 0x8000fa00, 0x24afffc8, 0x51a0b680, 
	0x21800118, 0x24001000, 0x8800e000, 0x25000048, 0x8000e700, 0x25bfff98, 
	0x24bfff88, 0xc000e400, 0x25000048, 0x8800e700, 0x21800058, 0x20800048, 
	0x5120b580, 0x21010010, 0x4020b590, 0x4020b698, 0x25000108, 0x8800ef00, 
	0x20800108, 0x51a0b580, 0x21810018, 0x4020b598, 0x20800088, 0x59400c02, 
	0x29bc0010, 0x88082900, 0x51a0a600, 0x258003d8, 0x018000d8, 0x80010800, 
	0x51a0a380, 0x25800418, 0x80010800, 0xc006a000, 0x25900008, 0x88010000, 
	0x25800108, 0x80010800, 0x25800088, 0x80010800, 0x24afffc8, 0x24bffe08, 
	0x51a0b680, 0x25bffed8, 0x4020b698, 0x51a0b580, 0x21810018, 0x4020b598, 
	0x5120a580, 0x26800210, 0x88010e00, 0x5120a600, 0x268003d0, 0x80011a00, 
	0x51a0a680, 0x4020b898, 0x293c0018, 0x80011a00, 0x594010c0, 0x4020bc10, 
	0x594014c0, 0x4020bc90, 0x2f011804, 0xc007c000, 0x2f011a04, 0xc007cf00, 
	0x5120a580, 0x25800210, 0x80011f00, 0x25800050, 0x8003e300, 0x51a0a600, 
	0x25800c18, 0x88012500, 0x2d000000, 0x4000c910, 0xc0013200, 0x2d0000c0, 
	0x4000c910, 0x2d000400, 0x4000c810, 0x2d0011c0, 0x4000ca10, 0x2d000000, 
	0x4000c810, 0x5120a300, 0x31000610, 0x4000ca10, 0x5120a300, 0x4000ca10, 
	0x5220a380, 0x25002020, 0x80016000, 0x59400c02, 0x2abc0010, 0x80014f00, 
	0x51400080, 0x26801020, 0x88013e00, 0x26800050, 0x80014f00, 0xc0014000, 
	0x25000090, 0x80014f00, 0x5120a600, 0x26800810, 0x80014800, 0x253fcfe0, 
	0x4000ca10, 0x2c000040, 0x4000c900, 0xc0016700, 0x258003d0, 0x018000d8, 
	0x88016000, 0x25001010, 0x88016000, 0x253fcfe0, 0xc0016700, 0x25c000d8, 
	0x80015a00, 0x5120a380, 0x4000ca10, 0x2d000040, 0x4000c910, 0x5120a600, 
	0x258003d0, 0x018000d8, 0x80016000, 0xc006a600, 0x5120a600, 0x258003d0, 
	0x80016000, 0x018000d8, 0x80016000, 0xc0016000, 0x5120a380, 0x51a0a600, 
	0x25800c18, 0x80016700, 0x4000ca10, 0x2c000040, 0x4000c900, 0x24000c10, 
	0x8006bc00, 0x5020a580, 0x24000040, 0x80017c00, 0x5220b000, 0x25804020, 
	0x80017100, 0x25803fe0, 0xc0017200, 0x2d8011c0, 0x2fc000c0, 0x5a401802, 
	0x260000a0, 0x80017800, 0x2f017804, 0xc0079600, 0x5220a300, 0x33800620, 
	0x2fc00100, 0x2fc00080, 0x25000810, 0x88018100, 0x24400000, 0x8801e600, 
	0xc001bf00, 0x2d800040, 0x2f018404, 0xc0077200, 0x2dc00180, 0x24000040, 
	0x80018800, 0x2fc000c0, 0x52a0a380, 0x26800428, 0x88018d00, 0x2d802dc0, 
	0xc0019400, 0x02802dd8, 0x9006bc00, 0x26c000d8, 0x88019400, 0x24000040, 
	0x8001bf00, 0xc001e600, 0x2f019604, 0xc0077200, 0x2d400180, 0x24000040, 
	0x80019a00, 0x2fc00080, 0x01800058, 0x52a0b580, 0x26bfc028, 0x22c000a8, 
	0x4020b5a8, 0x26800410, 0x8001b500, 0x02800198, 0x9801b500, 0x32800230, 
	0x22c001a8, 0x4020b928, 0x31000230, 0x21400190, 0x4020b990, 0x32000230, 
	0x224001a0, 0x4020ba20, 0x24000040, 0x8001b400, 0x33800628, 0x2fc00140, 
	0x33800610, 0x2fc00080, 0x33800620, 0x2fc00100, 0x01800198, 0x24000040, 
	0x8001ba00, 0xe04000c0, 0x2fc00180, 0xc001e600, 0xe04000c0, 0x2c400180, 
	0x5220a380, 0x26000420, 0x8001e600, 0x51a0a600, 0x25820018, 0x8801e600, 
	0x51a0a380, 0x25800418, 0x8001e600, 0x5120b580, 0x26820008, 0x8801cb00, 
	0x25008010, 0x8801e000, 0xc006f000, 0x24002010, 0x8001cf00, 0x25000410, 
	0x8001e000, 0x5020a380, 0x260003c0, 0x26803c08, 0x32800728, 0x28400160, 
	0x8801db00, 0x25000208, 0x8801d900, 0x20800208, 0xc0056900, 0x24bffdc8, 
	0xc0069d00, 0x24bffdc8, 0x12800068, 0x268003e8, 0x28400160, 0x88069d00, 
	0x5020a380, 0x260003c0, 0x32000120, 0x24bfc3c8, 0x20c00108, 0x20820008, 
	0x51a0a680, 0x29bc0018, 0x8801ed00, 0x51a0ac00, 0x29bc0018, 0x8006a000, 
	0xc0056900, 0x5020a600, 0x258003c0, 0x25000c00, 0x8801f300, 0x25c000d8, 
	0x80056900, 0x5120a600, 0x25000c10, 0x8001f800, 0x2e000040, 0x4000c920, 
	0x50001000, 0x51a0a600, 0x258003d8, 0x88020200, 0x21880000, 0x2f01ff04, 
	0xc0077200, 0xe0400000, 0x2dc00180, 0xc0056900, 0x010000d8, 0x80026500, 
	0x24400000, 0x80051e00, 0x01000098, 0x80027100, 0x01000058, 0x80043800, 
	0xc0056900, 0x29002e00, 0x8806bc00, 0x25810008, 0x80056900, 0x5a401806, 
	0x2b3c0025, 0x80056900, 0x5020b580, 0x24008000, 0x88021800, 0x25c00127, 
	0x88024d00, 0xc0056900, 0x25c00127, 0x80021f00, 0x51a0a680, 0x50000400, 
	0x40000200, 0x50000480, 0x40000280, 0x2d900100, 0x2f022204, 0xc0077200, 
	0x2c400180, 0x2c400180, 0x31800234, 0x21c0019d, 0x2583ffd9, 0x80056900, 
	0x2b3c0025, 0x80056900, 0x5a401c06, 0x26400021, 0x5ac00c06, 0x2a400129, 
	0x26003fe0, 0x88056900, 0x2e8011c0, 0x26000040, 0x88023600, 0x25800088, 
	0x80023900, 0xc0023f00, 0x25800048, 0x80023c00, 0xc0023f00, 0x20800088, 
	0x24bfff88, 0xc0023e00, 0x20800048, 0x24bfff48, 0x2e801d00, 0x2d900040, 
	0x2f024204, 0xc0077200, 0x32000230, 0x20400100, 0x2a0011e8, 0x80056900, 
	0xe0000300, 0x2f800000, 0x2fc00000, 0x33800619, 0x2fc000c2, 0x33800600, 
	0x0180005d, 0x2583ffdd, 0x51801000, 0x21900018, 0x2f025204, 0xc0077200, 
	0x01900018, 0x2d4000c2, 0x004000da, 0x90025800, 0x2d4000c0, 0x2c000000, 
	0xe0400080, 0x2fc00180, 0xe0400000, 0x2f400184, 0x01c000dd, 0x90025f00, 
	0x2d800004, 0x4840181b, 0x21c000df, 0x88051e00, 0x2f800000, 0xc0026b00, 
	0xc0056900, 0x51a0ab00, 0x21c000c0, 0x2f026904, 0xc0077200, 0xe0400000, 
	0x2fc00180, 0x2d800040, 0x4020a518, 0x51a0a580, 0x25800218, 0x8803e300, 
	0xc0051e00, 0x51008204, 0x250000d5, 0x268000c0, 0x114000ae, 0x250000d5, 
	0x52a0b580, 0x24008028, 0x80029e00, 0x25018008, 0x01818010, 0x80028500, 
	0x01810010, 0x9802a900, 0x51a0b500, 0x21802018, 0x4020b518, 0x24be7fc8, 
	0x2f028404, 0xc0081900, 0xc002a900, 0x51a0a800, 0x52a0a680, 0x25000418, 
	0x88029300, 0x59c02140, 0x59402540, 0x2fc000c0, 0x33800618, 0x2fc00080, 
	0x33800610, 0x2f029104, 0xc0080200, 0x25800098, 0x8806b400, 0x2d800040, 
	0x4020a518, 0x51008200, 0x50008280, 0x4020a410, 0x4020a480, 0x48402150, 
	0x48402540, 0x40008210, 0x40008280, 0xc002a900, 0x24018008, 0x80056900, 
	0x2e000000, 0x2f02a304, 0xc003d800, 0x2e800000, 0x01018000, 0x80036b00, 
	0x01010000, 0x80036b00, 0xc0033c00, 0x52801000, 0x01000268, 0x9002b000, 
	0x2dc00140, 0x2f02af04, 0xc0077200, 0xc006ab00, 0x2e001000, 0x2f02b304, 
	0xc003d800, 0x258001c8, 0x25800098, 0x8802b900, 0x2e800240, 0x2c400140, 
	0xc002c600, 0x5020aa00, 0x24400000, 0x8002c400, 0x2c000000, 0x4020aa00, 
	0x2c400140, 0x26bffc00, 0x8802c200, 0xc002c600, 0x240003c0, 0x4020aa00, 
	0x5020ab00, 0x20400140, 0x2dc00000, 0x2f02c904, 0xc0077200, 0x028000a8, 
	0x30000230, 0x20400180, 0x8806ab00, 0x02800068, 0x2c400180, 0x00000040, 
	0x8806ab00, 0x2f800000, 0x2f800000, 0x2f800040, 0x02800068, 0x2c400180, 
	0x48402002, 0x2fc00000, 0x59c01802, 0x25004018, 0x8002de00, 0x25803fd8, 
	0x29c00018, 0x8806ab00, 0x028000a8, 0x31000230, 0x21400190, 0x48401c12, 
	0x25400090, 0x8002e800, 0x9802e800, 0x51801000, 0x01c000d0, 0x9806ab00, 
	0x33800610, 0x2fc00080, 0x29802f00, 0x8002fa00, 0x29802fc0, 0x8002fa00, 
	0x29803c00, 0x8002fa00, 0x29803c40, 0x8002fa00, 0x29803fc0, 0x8002fa00, 
	0x29803c80, 0x8002fa00, 0x29803e00, 0x8002fa00, 0x29802f80, 0x88030600, 
	0xe0400140, 0x2fc00180, 0x2e800000, 0x253e7fc8, 0x20818010, 0x59c01c02, 
	0x01800298, 0x48401c1a, 0x50001000, 0x24400000, 0x8003b600, 0xc0038c00, 
	0x2c400180, 0x02800068, 0x25800800, 0x80032600, 0x51a0a600, 0x25801018, 
	0x88032600, 0x25800108, 0x88031700, 0x20800108, 0x24bfff48, 0x51a0b580, 
	0x21810018, 0x4020b598, 0x51a0b680, 0x21800118, 0x4020b698, 0x5a400c02, 
	0x29bc0020, 0x8006a600, 0x25800400, 0x88032100, 0x52400100, 0x26004020, 
	0x8006a600, 0x24bfff88, 0xc0032e00, 0x52400100, 0x26008020, 0x8006a600, 
	0x20800048, 0xc0032e00, 0x25800108, 0x80032e00, 0x25800088, 0x88032e00, 
	0x24bffe08, 0x51a0b580, 0x21810018, 0x4020b598, 0x2fc00000, 0x028000a8, 
	0x2fc00180, 0x2c400180, 0x2fc00000, 0x48402002, 0x24400000, 0x80035e00, 
	0x59401c02, 0x25400090, 0x80033b00, 0x01400010, 0x48401c12, 0x20808008, 
	0x58402002, 0x51001000, 0x25400090, 0x8003c400, 0x01c00010, 0x90034300, 
	0x2c400080, 0x26000088, 0x80034d00, 0x26c00168, 0x88035300, 0x2ec00080, 
	0x2dc00080, 0x21880018, 0x2f034c04, 0xc0077200, 0xc0035300, 0x26c00168, 
	0x8806ab00, 0x2ec00000, 0x2dc00000, 0x2f035304, 0xc0077200, 0x59402002, 
	0x01400010, 0x48402012, 0xe0400000, 0x2fc00180, 0x02c00028, 0x25400090, 
	0x80035e00, 0x26c00168, 0x8806ab00, 0xc003c400, 0x58401c02, 0x24400000, 
	0x80036600, 0x000000c0, 0x48401c02, 0x253e7fc8, 0x20810010, 0xc0036800, 
	0x253e7fc8, 0x20818010, 0x51001000, 0x25400090, 0x8003c400, 0x240001c8, 
	0x25000100, 0x80038800, 0x25000080, 0x88038800, 0x59c00c02, 0x293c0018, 
	0x8006a000, 0x25000040, 0x80037a00, 0x514000c0, 0x25008010, 0x8006a600, 
	0x12000318, 0xc0037e00, 0x514000c0, 0x25008010, 0x8006a600, 0x12000118, 
	0x55400500, 0x54400500, 0x4000e410, 0x4000e480, 0x55400500, 0x54400500, 
	0x4000e510, 0x4000e580, 0x2c080000, 0x4020ab00, 0x50001000, 0x59c01c02, 
	0x00400018, 0x48401c02, 0x26c00168, 0x88039300, 0x51a0ab00, 0x52801000, 
	0x21c00158, 0x2f039304, 0xc0077200, 0x25400097, 0x88039a00, 0xe0400140, 
	0x2fc00180, 0x2e800000, 0xc003b600, 0x2d400144, 0x01c000aa, 0x98039900, 
	0xe04000c0, 0x2fc00180, 0x2f03a004, 0xc0080200, 0x25800098, 0x8806b400, 
	0x52808200, 0x51808280, 0x40008228, 0x40008298, 0x298000d1, 0x8803b300, 
	0x01000095, 0x2fc00180, 0x2fc00180, 0x2f03ad04, 0xc0080200, 0x25800098, 
	0x8806b400, 0x52808200, 0x51808280, 0x40008228, 0x40008298, 0x2e800000, 
	0xe0400082, 0x2fc00180, 0x5020aa00, 0x24400000, 0x8003be00, 0x2dc00000, 
	0x2f03bc04, 0xc0077200, 0xe0400000, 0x2fc00180, 0x24018008, 0x00018000, 
	0x8003c400, 0x58401c02, 0x24400000, 0x8003c800, 0x5020a800, 0x24000100, 
	0x8803c900, 0xc0051e00, 0x24be7fc8, 0x51a0a500, 0x11800058, 0x4020a518, 
	0x51a0a800, 0x25000418, 0x8803d600, 0x51a0a680, 0x594020c0, 0x59c024c0, 
	0x2fc00080, 0x33800610, 0x2fc000c0, 0x33800618, 0x2c000040, 0xc0051f00, 
	0x5120a800, 0x25000110, 0x8003e200, 0x52a0b580, 0x26802028, 0x22c00128, 
	0x26003c08, 0x32000720, 0x23c00160, 0x53801000, 0xc0400182, 0x50207200, 
	0x25000400, 0x8803e400, 0x2d000040, 0x40207390, 0x25000040, 0x8803ee00, 
	0x5120a580, 0x25000050, 0x88051e00, 0xc0043500, 0x25000080, 0x80040000, 
	0x2ea07280, 0x55c00540, 0x51000a00, 0x300007d8, 0x251fffd0, 0x02400010, 
	0x98043100, 0x2a400010, 0x88040000, 0x54400540, 0x51000a80, 0x300007c0, 
	0x318003d8, 0x204000c0, 0x01400010, 0x98043100, 0x52a07200, 0x30000628, 
	0x2d802f00, 0x01c00018, 0x5220a680, 0x2a3c0020, 0x80041000, 0x2e3fffc0, 
	0x2ea07400, 0x12c00028, 0x54400540, 0x2fc00000, 0x33800600, 0x01800098, 
	0x88040a00, 0xc0042300, 0x2e000000, 0x2ea07400, 0x2d0000c0, 0x4000c910, 
	0x2d000400, 0x4000c810, 0x54400540, 0x4000ca00, 0x2d000000, 0x4000c810, 
	0x30000600, 0x4000ca00, 0x01800098, 0x54400540, 0x4000ca00, 0x30000600, 
	0x4000ca00, 0x01800098, 0x88041d00, 0x50207200, 0x24003c00, 0x40207200, 
	0x2c000000, 0x40207380, 0x5020b500, 0x20010000, 0x4020b500, 0x263fffe0, 
	0x80056900, 0x51a0a500, 0x11800058, 0x4020a518, 0xc0051e00, 0x5120a580, 
	0x25000050, 0x88051e00, 0xc0043500, 0x2d000000, 0x40207390, 0xc0001700, 
	0x59401802, 0x29bc0010, 0x8006a000, 0x51a0ab00, 0x21c000c0, 0x2f043f04, 
	0xc0077200, 0x59c01842, 0x25bc0018, 0x30000618, 0x200f0000, 0x40003800, 
	0x58400c42, 0x2a140018, 0x88044900, 0x20010000, 0xc0044d00, 0x2a1c0018, 
	0x88044c00, 0x10000200, 0x2000c000, 0x40004100, 0x58400842, 0x40004180, 
	0x2c000040, 0x40004000, 0x40004080, 0x52a0a684, 0x58402142, 0x5a402542, 
	0x4020a400, 0x4020a4a0, 0x40008f00, 0x40008fa0, 0x2c000000, 0x40003c80, 
	0x40003d80, 0x40004600, 0x40004880, 0x5ac01842, 0x26bc0028, 0x59c04042, 
	0x58400080, 0x244000c0, 0x40003c00, 0x40003d00, 0x59c04442, 0x58400480, 
	0x244000c0, 0x40003c80, 0x40003d80, 0x59c04842, 0x58400880, 0x25c000c0, 
	0x28100028, 0x88047600, 0x40003d18, 0x59c04c42, 0x58400c80, 0x244000c0, 
	0x40003d80, 0xc0047800, 0x40004618, 0x40004898, 0x24008008, 0x8004a100, 
	0x5020b580, 0x24008000, 0x80048d00, 0x2c400180, 0x59c02480, 0x018000d8, 
	0x90048400, 0x11c000c0, 0x98048700, 0xc0048d00, 0x59c03080, 0x2a4000c0, 
	0x80048d00, 0x24bf7fc8, 0x51a0a400, 0x5220a480, 0x40008218, 0x400082a0, 
	0xc004a500, 0x12800210, 0x2d800140, 0x2e003000, 0x54400540, 0x44400500, 
	0x01800058, 0x88049000, 0x2d8000c0, 0x2e003480, 0x54400540, 0x44400500, 
	0x01800058, 0x88049600, 0x2d800080, 0x2e003700, 0x54400540, 0x44400500, 
	0x01800058, 0x88049c00, 0xc004b400, 0x5020b580, 0x24008000, 0x8006bc00, 
	0x2c400180, 0x51801000, 0x01c00018, 0x9806bc00, 0xe0400000, 0x2ec00180, 
	0x25c000d8, 0x80056900, 0x2c000000, 0x40003880, 0x40003700, 0x40003480, 
	0x40003780, 0x2c3fffc0, 0x40003500, 0x40003580, 0x2f04b604, 0xc007a400, 
	0x2c000040, 0x40003880, 0x2f04ba04, 0xc007ab00, 0x24200020, 0x88056900, 
	0x24000060, 0x8004ea00, 0x240000a0, 0x8004ea00, 0x58401842, 0x24000080, 
	0x8804d200, 0x2d8001c0, 0x40008618, 0x40008698, 0x2c3fffc0, 0x00000040, 
	0x8004cd00, 0x51808600, 0x2a800058, 0x26bffee8, 0x8004c700, 0x50008200, 
	0x51808280, 0x40008f00, 0x40008f98, 0xc004f200, 0x5020b500, 0x20000080, 
	0x4020b500, 0x5020a880, 0x243ffec0, 0x4020a880, 0x5020ad00, 0x24000080, 
	0x8804d800, 0x2c000040, 0x40008600, 0x40008680, 0x50008600, 0x29800040, 
	0x8004de00, 0x5120a880, 0x21000110, 0x4020a890, 0x24000080, 0x8806b400, 
	0x50008f00, 0x51808f80, 0x40008200, 0x40008298, 0x50008f00, 0x51a0a400, 
	0x284000c0, 0x8804f200, 0x50008f80, 0x51a0a480, 0x284000c0, 0x8004f900, 
	0x50008f00, 0x51808f80, 0x4020a400, 0x4020a498, 0x5020a500, 0x10000040, 
	0x4020a500, 0x24000220, 0x80050100, 0x50001000, 0xe0400000, 0x2d400180, 
	0x2c000040, 0x24bf7fc8, 0xc0051f00, 0x24000120, 0x8004ac00, 0x51003480, 
	0x010000d0, 0x98050900, 0x50003700, 0x24400000, 0x8004fe00, 0x20808008, 
	0x5ac01802, 0x12800228, 0x2e003000, 0x2c000140, 0x55400500, 0x44400550, 
	0x00000040, 0x88050e00, 0x2e003480, 0x2c0000c0, 0x55400500, 0x44400550, 
	0x00000040, 0x88051400, 0x2e003700, 0x2c000080, 0x55400500, 0x44400550, 
	0x00000040, 0x88051a00, 0x2c000000, 0x2f052104, 0xc0080200, 0x52a0a680, 
	0x5120a600, 0x25018010, 0x80052900, 0x52008304, 0x52808384, 0x48401961, 
	0x48401d69, 0x25800098, 0x8806b400, 0x51808204, 0x52008280, 0x48401159, 
	0x48401560, 0x24400000, 0x80053300, 0x4020a419, 0x4020a4a0, 0x5020a600, 
	0x24018000, 0x80053e00, 0x2e008200, 0x30000700, 0x12c00020, 0x54400500, 
	0x55400500, 0x243ffc00, 0x44400540, 0x44400550, 0x52a0a500, 0x51a0a680, 
	0x5a4028c0, 0x59402cc0, 0x10400160, 0x19000010, 0x4020bb00, 0x4020bb90, 
	0x26c00168, 0x80056900, 0x484028c0, 0x48402cd0, 0x5220a600, 0x260003e0, 
	0x29000060, 0x80055200, 0x290000a0, 0x80055200, 0x2e808200, 0xc0055300, 
	0x2ea0a400, 0x56400540, 0x55400540, 0x484020e0, 0x484024d0, 0x5a4034c0, 
	0x02c00100, 0x98056900, 0x52a0a600, 0x268003e8, 0x2a8000e8, 0x88056100, 
	0x12000120, 0x02c00100, 0x90056900, 0x584030c0, 0x26800040, 0x88056900, 
	0x26800200, 0x88056900, 0x5020b500, 0x20001000, 0x4020b500, 0x51801000, 
	0x51207100, 0x01c00098, 0xe04000c0, 0x2c400180, 0x51801000, 0x2f057104, 
	0xc0077200, 0xe04000c0, 0x2c400180, 0x2f057504, 0xc0080200, 0x25800098, 
	0x8806ba00, 0x51a0ac00, 0x293c0018, 0x8005e100, 0x40206f80, 0x58402402, 
	0x24004000, 0x8005e100, 0x5020a580, 0x25000100, 0x8805e100, 0x2e4000c4, 
	0x2f058404, 0xc007c000, 0x59401102, 0x4020bc10, 0x59401502, 0x4020bc90, 
	0x2d800040, 0x4000cb18, 0x2da03800, 0x4000cc18, 0x2d206a00, 0x50400080, 
	0x2d803180, 0x4000c018, 0x40001018, 0x2e3fffc0, 0x51a0a600, 0x258003d8, 
	0x2a800198, 0x80059900, 0x52a0b500, 0x26880028, 0x80059e00, 0x5ac02002, 
	0x26c00168, 0x80059e00, 0x3180062c, 0x26003fe8, 0x2dc00180, 0x018011d8, 
	0x8805a200, 0x2dc00180, 0x2f8011c0, 0x5c400480, 0x2403ffc0, 0x25800040, 
	0x11c000c0, 0x2e800000, 0x02400020, 0x8c05ae00, 0x12400020, 0x02c00100, 
	0x2c400100, 0x2e000000, 0xe0400000, 0x2fc00180, 0xe0400140, 0x2f400184, 
	0xe0400140, 0x2fc000c2, 0x25800040, 0x8005b700, 0x2dc00180, 0x5c400480, 
	0x2403ffc0, 0x8805a500, 0x51801000, 0xe04000c0, 0x2c400180, 0x2f05bf04, 
	0xc0080200, 0x25800098, 0x8805d900, 0x50008200, 0x51008280, 0x48401102, 
	0x48401512, 0x48402102, 0x48402512, 0x58402902, 0x59402d02, 0x10000040, 
	0x19000010, 0x48402902, 0x48402d12, 0x4020bd00, 0x4020bd90, 0x59403502, 
	0x00400080, 0x9805e100, 0x58403102, 0x24000040, 0x8805e100, 0x5020b580, 
	0x20100000, 0x4020b580, 0xc005e000, 0x58402102, 0x59402502, 0x48401102, 
	0x48401512, 0x51a0b500, 0x21a20018, 0x4020b518, 0x50206f80, 0x5120b500, 
	0x59c03842, 0x21880018, 0x25c00098, 0x4020b518, 0x25336fd0, 0x293fffd0, 
	0x59c03842, 0x25c00098, 0x4840385a, 0x5120b580, 0x59c01402, 0x21900018, 
	0x26c000d0, 0x5a403c42, 0x258000a0, 0x8805f500, 0x52a0b580, 0x26900028, 
	0xc0060900, 0x51a0a600, 0x258003d8, 0x298000d8, 0x88060600, 0x51a0a580, 
	0x25800118, 0x88060900, 0x25800060, 0x80060900, 0x22820028, 0x5120b680, 
	0x25000110, 0x80060300, 0x22810028, 0x263fffa0, 0x48403c62, 0xc0060900, 
	0x51a0ac00, 0x293c0018, 0x8805f900, 0x59405042, 0x4020b710, 0x59c05442, 
	0x4020b798, 0x5a405842, 0x4020b820, 0x5120b500, 0x4020b5a8, 0x22c000a8, 
	0x80064600, 0x2da63400, 0x2f061604, 0xc007c000, 0x52a0a580, 0x26800128, 
	0x80061c00, 0x52a0b580, 0x26a3bfe8, 0x4020b5a8, 0x2ea0b500, 0x2d800900, 
	0x56400540, 0x27803fe0, 0x33800620, 0x01800098, 0x88061e00, 0x26080010, 
	0x80063300, 0x51a0a984, 0x24000040, 0x01c0001d, 0x2d800040, 0x4000cb18, 
	0x4000cc19, 0x2d801e00, 0x11c00018, 0x4000c018, 0x40001018, 0xe0400000, 
	0x2f400184, 0xe0001e00, 0x2fc00180, 0x2f063504, 0xc0080200, 0x25800098, 
	0x80063c00, 0x2f063904, 0xc007de00, 0x2d800080, 0x40005018, 0xc0064c00, 
	0x51808200, 0x51008280, 0x40263618, 0x40263690, 0x51a63600, 0x51263700, 
	0x29c00098, 0x80064600, 0x2d800040, 0x40005018, 0x51a0a580, 0x25800218, 
	0x88001700, 0x51a0a600, 0x25804018, 0x88001700, 0x4840000a, 0xc0001700, 
	0x2d800040, 0x2f065104, 0xc0077200, 0x2d400180, 0x2c000000, 0x4000cb00, 
	0x2c000180, 0x4000c000, 0x2c400180, 0x31800780, 0x258000c4, 0x3180021d, 
	0x22c0019d, 0x32000234, 0x224001a5, 0x31800234, 0x21c0019d, 0x2e0000c0, 
	0x2ca63804, 0x58c01c42, 0x2403ffc8, 0x284000c0, 0x80066900, 0x1080100d, 
	0x02000060, 0x88066100, 0xc0001700, 0x24200008, 0x80066e00, 0x4840506b, 
	0x48405463, 0x4840585b, 0xc0007b00, 0x2c000100, 0x40005000, 0x58403042, 
	0x25800c00, 0x80001700, 0x2c000400, 0x4000c800, 0x2c0000c0, 0x4000c900, 
	0x2c0011c0, 0x4000ca00, 0x2c000000, 0x4000c800, 0x5020a300, 0x31000600, 
	0x4000ca10, 0x4000ca00, 0x2c000040, 0x4000c900, 0x2c002e40, 0x59401842, 
	0x25000410, 0x88068700, 0x2c002000, 0x4000c000, 0xe0400000, 0x2c400180, 
	0xc0001700, 0x51a0a600, 0x25802018, 0x80069d00, 0x51a0a680, 0x2a3c0018, 
	0x80069d00, 0x5ac030c0, 0x268003a8, 0x2a800028, 0x88069d00, 0x2f069704, 
	0xc007c000, 0x2f802d00, 0x2f800000, 0x2f800000, 0x2f800040, 0x2f802d00, 
	0xc006c000, 0x24bdffc8, 0x2e800400, 0xc006bd00, 0x2e900000, 0xc006bd00, 
	0x2e800080, 0xc006bd00, 0x2e800100, 0xc006bd00, 0x59400c02, 0x2abc0010, 
	0x8006a000, 0x2e800100, 0xc006bd00, 0x2e802000, 0xc006bd00, 0x2a000128, 
	0x8006bc00, 0x2a000328, 0x8006bc00, 0x2a000228, 0x8006bc00, 0xc0056900, 
	0x51a0b500, 0x21a00018, 0x4020b518, 0x51a0b580, 0x25bfbfd8, 0x4020b598, 
	0x2f06c004, 0xc007de00, 0x2e800200, 0x51a0b500, 0x21c00158, 0x4020b518, 
	0x2f06c204, 0xc0080200, 0x25800098, 0x8006d700, 0x51a0b500, 0x21a00018, 
	0x4020b518, 0x51a0a600, 0x25818018, 0x8006d500, 0x2e008200, 0x31800718, 
	0x12c000e0, 0x55c00540, 0x55400540, 0x5220a680, 0x12000220, 0x44400518, 
	0x44400510, 0x44400518, 0x44400510, 0x2f06d704, 0xc007de00, 0x5020a600, 
	0x240003c0, 0x01800040, 0x8006e400, 0x01800080, 0x8006ef00, 0x59c01802, 
	0x293c0018, 0x8006f000, 0x594034c0, 0x253fdfd0, 0x484034d0, 0xc006f000, 
	0x50008700, 0x2d800040, 0x40009718, 0x40009798, 0x51809700, 0x25800058, 
	0x8806e800, 0x40008700, 0x40008780, 0x24bf7fc8, 0xc006f000, 0x24be7fc8, 
	0x2f06f204, 0xc0081900, 0x5120b580, 0x25008010, 0x80056900, 0x5120b500, 
	0x29000410, 0x88056900, 0x51001000, 0x25400090, 0x80056900, 0x58401842, 
	0x24000400, 0x8801e000, 0xc0056900, 0x2e3c0000, 0x4020a6a0, 0x4020ac20, 
	0x51263b80, 0x2503ffd0, 0x29002010, 0x80070900, 0x59401c42, 0x25200010, 
	0x88071000, 0x52005200, 0x51005280, 0x52805300, 0x26a07fe8, 0x48405062, 
	0x48405452, 0x4840586a, 0x59402442, 0x25000090, 0x80071500, 0x2c3fffc0, 
	0x2cbfffc0, 0x59401042, 0x40002010, 0x59401442, 0x40002090, 0x40002188, 
	0x2d000040, 0x40002110, 0x51002100, 0x25400090, 0x88071c00, 0x51002200, 
	0x24a00010, 0x88075500, 0x58401046, 0x00200005, 0x300007c5, 0x10400085, 
	0x30800201, 0x4020b608, 0x30800101, 0x30000085, 0x10400045, 0x1020be05, 
	0x58c00402, 0x10800048, 0x4840040a, 0x5ac01002, 0x24bfc028, 0x2a3fc008, 
	0x80073a00, 0x32000688, 0x308006c8, 0x12400060, 0x30800048, 0x12400060, 
	0x12229e20, 0x4020ac20, 0x24803fe8, 0x2a003fc8, 0x80074400, 0x32000088, 
	0x308000c8, 0x12400060, 0x30800048, 0x12400060, 0x12229e20, 0x4020a6a0, 
	0x58c03100, 0x4020a808, 0x58c00802, 0x4020a608, 0x2e000000, 0x248003c8, 
	0x288000c8, 0x88075300, 0x5220a300, 0x5ac01c02, 0x26400020, 0x22400160, 
	0x4020a320, 0x2e000040, 0x4020a5a0, 0x58c00002, 0xc0400182, 0x2cbfffc0, 
	0x4020b608, 0x5120af00, 0x50a0af80, 0x2c3fffc0, 0x00afbbc8, 0x80072200, 
	0x58c03042, 0x24000c0c, 0x88067400, 0x2400400c, 0x80001700, 0x52a07200, 
	0x24800068, 0x80001700, 0x2e000200, 0x4020a5a0, 0x24800328, 0x30800788, 
	0x2ca63804, 0xe0400040, 0x1080100d, 0x58c03442, 0x4020a688, 0x58c03040, 
	0x4020a808, 0x2c800000, 0x4020a608, 0xc0400182, 0x40207008, 0x2483ffd8, 
	0x80078b00, 0x40206f98, 0x40207090, 0x2c800080, 0x4000cb08, 0x51206f00, 
	0x10800090, 0x40206f08, 0x10a06a10, 0x51206980, 0x4000cc10, 0x4000c018, 
	0x40400058, 0x2583ffd8, 0x50801000, 0x00c000c8, 0x40207108, 0x24800058, 
	0x11c00058, 0x114000d0, 0x40206990, 0x51207080, 0x51a06f80, 0x50a07000, 
	0xc0400182, 0x56c00504, 0x4440052b, 0x56c00504, 0x4440052b, 0x56c00504, 
	0x4440052b, 0x56c00504, 0x4440052b, 0xc0400182, 0x5a401c42, 0x2603ffe0, 
	0x320000a0, 0x59405046, 0x32800611, 0x23c00160, 0x2fc00082, 0x5a405442, 
	0x33800620, 0x2fc00100, 0x5a405842, 0x33800620, 0x2fc00100, 0xc0400182, 
	0x5220ad00, 0x26000060, 0x8807a400, 0x5220a880, 0x263fffa0, 0x4020a8a0, 
	0xc0400182, 0x5220b080, 0x52003884, 0x26000065, 0x8007bb00, 0x02000060, 
	0x8807ac00, 0x52004c04, 0x402d7fa1, 0x2e000004, 0x400038a1, 0x5220a904, 
	0x12000065, 0x0a000025, 0x4020a921, 0x52200000, 0xc007bc00, 0x52003780, 
	0x5220a884, 0x22000065, 0x4020a8a1, 0xc0400182, 0x2e008000, 0x2e800200, 
	0x554004c4, 0x44400511, 0x02800068, 0x8807c200, 0x2d800000, 0x52008000, 
	0x52808300, 0x2a400160, 0x8807cc00, 0x2d800040, 0x40008418, 0x40008498, 
	0xc0400182, 0x5020a600, 0x24018000, 0x8007dd00, 0x2e008300, 0x30000700, 
	0x12c00020, 0x55c00540, 0x54400540, 0x44400518, 0x44400500, 0x52a0a680, 
	0x12800328, 0x44400558, 0x44400540, 0xc0400182, 0x2d800380, 0x40008718, 
	0x40008798, 0x5120ae00, 0x51a0ae80, 0x40008210, 0x40008298, 0x40008098, 
	0x40008398, 0x2d800000, 0x40008118, 0x40008198, 0x51a0a880, 0x25bffed8, 
	0x4020a898, 0x51a0ad00, 0x25800098, 0x8807ed00, 0x2d8001c0, 0x40008618, 
	0x40008698, 0x2d8003c0, 0x40008718, 0x40008798, 0x51808600, 0x25000058, 
	0x8807f600, 0x5120a880, 0x21000110, 0x4020a890, 0x2d000100, 0x40008610, 
	0x40008690, 0x25800098, 0x8807de00, 0xc0400182, 0x51a0a880, 0x25bffed8, 
	0x4020a898, 0x51a0ad00, 0x25800098, 0x88080500, 0x2d8001c0, 0x40008618, 
	0x40008698, 0x2e3fffc0, 0x02000060, 0x8004cd00, 0x51808600, 0x29000058, 
	0x253ffed0, 0x80080c00, 0x5220a880, 0x22000120, 0x4020a8a0, 0x2e000100, 
	0x40008620, 0x400086a0, 0xc0400182, 0x51a0a800, 0x25800818, 0x80082800, 
	0x51a0a680, 0x293c0018, 0x80082800, 0x5120a600, 0x25018010, 0x88082800, 
	0x594020c0, 0x584024c0, 0x484010d0, 0x40008210, 0x484014c0, 0x40008280, 
	0xc0400182, 0x51a0a380, 0x25800c18, 0x29800818, 0x80087200, 0x51c00080, 
	0x25c000d8, 0x8006a400, 0x263c0018, 0x32000520, 0x25801c18, 0x28001818, 
	0x88083600, 0x2d800400, 0x318007d8, 0x21c000e0, 0x4000e018, 0x2c400100, 
	0x2a000120, 0x88084600, 0x2e203000, 0x2e00ec04, 0x2f084004, 0xc0078d00, 
	0x2f084204, 0xc0078d00, 0x2f084404, 0xc0078d00, 0x2f084604, 0xc0078d00, 
	0x26800048, 0x80084d00, 0x51c00080, 0x25800098, 0x8006a600, 0x12000310, 
	0xc0085100, 0x51c00080, 0x25800058, 0x8006a600, 0x12000110, 0x2a0000c4, 
	0x80085d00, 0x2a000144, 0x80085d00, 0x2e00e404, 0x2f085804, 0xc0078d00, 
	0x12000220, 0x2e00ea04, 0x2f085c04, 0xc0078d00, 0xc0087000, 0x2e00ee04, 
	0x2f086004, 0xc0078d00, 0x26c00168, 0x88086400, 0x12000220, 0xc0086500, 
	0x12000620, 0x2e00f004, 0x2f086804, 0xc0078d00, 0x2e00ec04, 0x2f086b04, 
	0xc0078d00, 0x2e00ea04, 0x2f086e04, 0xc0078d00, 0x2d800040, 0x4020aa18, 
	0x2c080000, 0x4020ab00, 0xc0010800 };

#ifdef ST_OSLINUX

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif
#include <linux/module.h>

#endif  /* ST_OSLINUX */

#include "stddefs.h"
#include "../stpti/pti4/tcdefs.h"
#include "../stpti/stptiint.h"

#if !defined( ST_OSLINUX )
#include "stsys.h"
#endif  /* ST_OSLINUX */


#if ((0X99EC-0X8000) > 0X1A00)
#error TCCODE data ram overflow
#endif

#if (TRANSPORT_CONTROLLER_CODE_SIZE > 0X0900)
#error TCCODE instruction ram overflow
#endif


ST_ErrorCode_t STPTI_DVBTCLoaderAL(STPTI_DevicePtr_t CodeStart, void * Params_p)
{
    U32 PTI4_Base = (U32)((U32)CodeStart - 0XC000); 

    STPTI_TCParameters_t * TC_Params_p = Params_p;

    /* Check that the user is using the correct loader for the correct architecture */
    #if defined( TC_PTI4_SECURELITE ) 
        #if !defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
            return( STPTI_ERROR_INVALID_LOADER_OPTIONS );
        #endif

    #elif defined( TC_PTI4_LITE ) 
        #if !defined( STPTI_ARCHITECTURE_PTI4_LITE )
            return( STPTI_ERROR_INVALID_LOADER_OPTIONS );
        #endif

    #else  /* PTI4 */
        #if !defined( STPTI_ARCHITECTURE_PTI4 )
            return( STPTI_ERROR_INVALID_LOADER_OPTIONS );
        #endif
    #endif

    TC_Params_p->TC_CodeStart                  = (STPTI_DevicePtr_t)VERSION;
    TC_Params_p->TC_CodeStart                  = CodeStart;
    TC_Params_p->TC_CodeSize                   = TRANSPORT_CONTROLLER_CODE_SIZE * sizeof(U32);
    TC_Params_p->TC_DataStart                  = (U32*)(((U32)PTI4_Base) + 0X8000);

    TC_Params_p->TC_LookupTableStart           = (U32*)(((U32)PTI4_Base) + 0X8000);
    TC_Params_p->TC_SystemKeyStart             = (U32*)(((U32)PTI4_Base) + 0X80C0);
    TC_Params_p->TC_GlobalDataStart            = (U32*)(((U32)PTI4_Base) + 0X828C);
    TC_Params_p->TC_StatusBlockStart           = (U32*)(((U32)PTI4_Base) + 0X82D4);
    TC_Params_p->TC_MainInfoStart              = (U32*)(((U32)PTI4_Base) + 0X82F8);
    TC_Params_p->TC_DMAConfigStart             = (U32*)(((U32)PTI4_Base) + 0X8A78);
    TC_Params_p->TC_DescramblerKeysStart       = (U32*)(((U32)PTI4_Base) + 0X907C);
    TC_Params_p->TC_TransportFilterStart       = (U32*)(((U32)PTI4_Base) + 0X91D0);
    TC_Params_p->TC_SCDFilterTableStart        = (U32*)(((U32)PTI4_Base) + 0X91D0);
    TC_Params_p->TC_PESFilterStart             = (U32*)(((U32)PTI4_Base) + 0X91D0);
    TC_Params_p->TC_SubstituteDataStart        = (U32*)(((U32)PTI4_Base) + 0X81C8);
    TC_Params_p->TC_SFStatusStart              = (U32*)(((U32)PTI4_Base) + 0X91D0);
    TC_Params_p->TC_InterruptDMAConfigStart    = (U32*)(((U32)PTI4_Base) + 0X98D0);
    TC_Params_p->TC_SessionDataStart           = (U32*)(((U32)PTI4_Base) + 0X98E0);
    TC_Params_p->TC_EMMStart                   = (U32*)(((U32)PTI4_Base) + 0X99A0);
    TC_Params_p->TC_ECMStart                   = (U32*)(((U32)PTI4_Base) + 0X99A0);
    TC_Params_p->TC_VersionID                  = (U32*)(((U32)PTI4_Base) + 0X99E0);

    TC_Params_p->TC_NumberCarousels            = 0X0001;
    TC_Params_p->TC_NumberSystemKeys           = 0X0001;
    TC_Params_p->TC_NumberDMAs                 = 0X0037;
    TC_Params_p->TC_NumberDescramblerKeys      = 0X0005;
    TC_Params_p->TC_SizeOfDescramblerKeys      = 0X0044;
    TC_Params_p->TC_NumberPesFilters           = 0X0000;
    TC_Params_p->TC_NumberSectionFilters       = 0X0040;
    TC_Params_p->TC_NumberSlots                = 0X0060;
    TC_Params_p->TC_NumberOfSessions           = 0X0003;
    TC_Params_p->TC_NumberIndexs               = 0X0060;
    TC_Params_p->TC_NumberTransportFilters     = 0X0000;
    TC_Params_p->TC_NumberSCDFilters           = 0X0000;

    TC_Params_p->TC_SignalEveryTransportPacket = 0X0001;
    TC_Params_p->TC_NumberEMMFilters           = 0X0000;
    TC_Params_p->TC_NumberECMFilters           = 0X0060;
    TC_Params_p->TC_AutomaticSectionFiltering  = FALSE;


    {
        U32 i;
        STPTI_DevicePtr_t DataStart = (U32*)(((U32)CodeStart&0xffff0000) | 0X8000);

        for(i = 0; i < (0X1A00/4); ++i)
        {
            STSYS_WriteRegDev32LE( (U32)&DataStart[i], 0x00 );
        }

        for(i = 0; i < 0X0900; ++i)
        {
            STSYS_WriteRegDev32LE( (U32)&CodeStart[i], 0x00 );
        }

        for(i = 0; i < TRANSPORT_CONTROLLER_CODE_SIZE; ++i)
        {
            STSYS_WriteRegDev32LE( (U32)&CodeStart[i], transport_controller_code[i] );
        }
        
        STSYS_WriteRegDev32LE( (U32)&(TC_Params_p->TC_VersionID[0]), 0X5354 | (0X5054<<16) );
        STSYS_WriteRegDev32LE( (U32)&(TC_Params_p->TC_VersionID[1]), 0X4934 | (0X0012<<16) );
        STSYS_WriteRegDev32LE( (U32)&(TC_Params_p->TC_VersionID[2]), 0X84D0 | (0X0000<<16) );
    }

    return ST_NO_ERROR;

}
