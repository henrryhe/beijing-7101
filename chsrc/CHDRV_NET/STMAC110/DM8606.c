#include "appltype.h"
#include "CHDRV_NET.h"
#include "CHDRV_NET_MAC110.h"
#include "initterm.h"
#include "stpio.h"

static U8 dm8606_phy_addr[5] = {0};
#define STMAC110_BASE_ADDRESS 0xb8110000

#define MAC_BASE_ADDR (void *)SYS_MapRegisters(STMAC110_BASE_ADDRESS,0x2000,"STxMAC")//0x18110000
#define PHY_READ_REG32(R) OSPLUS_READ32(R)//(*((volatile unsigned int   *) (MAC_BASE_ADDR+(R))))
#define PHY_WRITE_REG32(R,D) OSPLUS_WRITE32(R,D)//((*((volatile unsigned int   *) (MAC_BASE_ADDR+(R))))=(D))
#define MII_ADDR_OFFSET	MAC_BASE_ADDR+0x14
#define MII_DATA_OFFSET	MAC_BASE_ADDR+0x18

#define ETH_MII_ADDR_WRITE(d) PHY_WRITE_REG32(MII_ADDR_OFFSET,d)
#define ETH_MII_ADDR_READ PHY_READ_REG32(MII_ADDR_OFFSET)

#define ETH_MII_DATA_WRITE(d) PHY_WRITE_REG32(MII_DATA_OFFSET,d)
#define ETH_MII_DATA_READ PHY_READ_REG32(MII_DATA_OFFSET)

#define PRINT_LINK_STATUS

void MII_write_reg(U16 device_addr, U16 reg_addr, U32 reg_data)
{
    U8 wait_check = 100;
    U32 mii_cmd = 0;
    
    for(wait_check = 100; wait_check && (ETH_MII_ADDR_READ & 0x01); wait_check--)
    	MILLI_DELAY(1);
    
    if(0x00 == wait_check)
    {
#ifdef PRINT_LINK_STATUS
		printf(" in MII_write_reg time out err\n");
#endif
		return;
	}
	
	ETH_MII_DATA_WRITE(reg_data);
	mii_cmd = (device_addr & 0x1f) << 11;
	mii_cmd |= (reg_addr & 0x1f) << 6;
	ETH_MII_ADDR_WRITE(mii_cmd | 0x02);
	
#ifdef PRINT_LINK_STATUS
		//printf(" MII_write_reg cmd is %08x\n", (mii_cmd | 0x02));
#endif	
	
    for(wait_check = 100; wait_check && (ETH_MII_ADDR_READ & 0x01); wait_check--)
    	MILLI_DELAY(1);
	
    if(0x00 == wait_check)
    {
#ifdef PRINT_LINK_STATUS
		printf(" out MII_write_reg time out err\n");
#endif
		return;
	}
}


U32 MII_read_reg(U16 device_addr, U16 reg_addr)
{
    U8 wait_check = 100;
    U32 mii_cmd = 0;
    
    for(wait_check = 100; wait_check && (ETH_MII_ADDR_READ& 0x01); wait_check--)
    	MILLI_DELAY(1);
    
    if(0x00 == wait_check)
    {
#ifdef PRINT_LINK_STATUS
		printf(" in MII_read_reg time out err\n");
#endif
		return;
	}
	
	mii_cmd = (device_addr & 0x1f) << 11;
	mii_cmd |= (reg_addr & 0x1f) << 6;
	ETH_MII_ADDR_WRITE(mii_cmd);
	
#ifdef PRINT_LINK_STATUS
		//printf(" MII_read_reg cmd is %08x\n", mii_cmd);
#endif		
	
    for(wait_check = 100; wait_check && (ETH_MII_ADDR_READ & 0x01); wait_check--)
    	MILLI_DELAY(1);
	
    if(0x00 == wait_check)
    {
#ifdef PRINT_LINK_STATUS
		printf(" out MII_read_reg time out err\n");
#endif
		return 0;
	}
	
#ifdef PRINT_LINK_STATUS
		//printf(" MII_read_reg return data is %08x\n", ETH_MII_DATA_READ & 0xffff);
#endif			
	
	return (ETH_MII_DATA_READ & 0xffff);
}


void dm8606_int(void)
{
	U32 phy_id_check_c = 0; 
	U32 phy_id_check_a = 0; 
    U8 i;
    
    for(i = 0; i < 32; i++)
    {
    	phy_id_check_c = MII_read_reg(i, 0x02);
    	//printf(" dm8606_int test addr %0x val is %08x\n", i, phy_id_check_c);
    }
    
    
    phy_id_check_c = MII_read_reg(0x02, 0x02);
    if(0x0181 == phy_id_check_c)
    {
#ifdef PRINT_LINK_STATUS
    	printf(" dm8606_int found dm8606c \n");
#endif
    	dm8606_phy_addr[0] = 0x02;
    	dm8606_phy_addr[1] = 0x03;
    	dm8606_phy_addr[2] = 0x04;
    	dm8606_phy_addr[3] = 0x05;
    	dm8606_phy_addr[4] = 0x06;
    	return;
    }
    
    phy_id_check_a = MII_read_reg(0x08, 0x02);
    if(0x0302 == phy_id_check_a)
    {
#ifdef PRINT_LINK_STATUS
    	printf(" dm8606_int found dm8606a \n");
#endif
    	dm8606_phy_addr[0] = 0x08;
    	dm8606_phy_addr[1] = 0x09;
    	dm8606_phy_addr[2] = 0x0a;
    	dm8606_phy_addr[3] = 0x0b;
    	dm8606_phy_addr[4] = 0x0c;
    	return;
    }
    
}


u8_t CH_DMPORT0_LINKUP(void)
{
	U32 phy_reg_1;
	U32 phy_reg_4;
	U32 phy_reg_5;
	U32 phy_link_check;
	
	MII_read_reg(dm8606_phy_addr[0], 0x01);
	phy_reg_1 = MII_read_reg(dm8606_phy_addr[0], 0x01);  // 2 time
	
	if(phy_reg_1 & 0x04) 
	{
#ifdef PRINT_LINK_STATUS
		phy_reg_4 = MII_read_reg(dm8606_phy_addr[0], 0x04);
		phy_reg_5 = MII_read_reg(dm8606_phy_addr[0], 0x05);
		phy_link_check = phy_reg_4 & phy_reg_5;
			
	    printf("port 0 link is ok ==> %sMbps %s mode \n", 
	       phy_link_check & ((1 << 8) | (1 << 7)) ? "100" : "10",
	       phy_link_check & ((1 << 8) | (1 << 6)) ? "FULL" : "HALF");
#endif
		return 1;
	}
	else
	{
#ifdef PRINT_LINK_STATUS
	   	printf("\n port 0 link is fail\n");
#endif
    	return  0;
    }
}


u8_t CH_DMPORT1_LINKUP(void)
{
	U32 phy_reg_1;
	U32 phy_reg_4;
	U32 phy_reg_5;
	U32 phy_link_check;
	
	MII_read_reg(dm8606_phy_addr[1], 0x01);
	phy_reg_1 = MII_read_reg(dm8606_phy_addr[1], 0x01);  // 2 time
	
	if(phy_reg_1 & 0x04) 
	{
#ifdef PRINT_LINK_STATUS
		phy_reg_4 = MII_read_reg(dm8606_phy_addr[1], 0x04);
		phy_reg_5 = MII_read_reg(dm8606_phy_addr[1], 0x05);
		phy_link_check = phy_reg_4 & phy_reg_5;
			
	    printf("port 1 link is ok ==> %sMbps %s mode \n", 
	       phy_link_check & ((1 << 8) | (1 << 7)) ? "100" : "10",
	       phy_link_check & ((1 << 8) | (1 << 6)) ? "FULL" : "HALF");
#endif
		return 1;
	}
	else
	{
#ifdef PRINT_LINK_STATUS
	   	printf("\n port 1 link is fail\n");
#endif
    	return  0;
    }
}

u8_t CH_DMPORT2_LINKUP(void)
{
	U32 phy_reg_1;
	U32 phy_reg_4;
	U32 phy_reg_5;
	U32 phy_link_check;
	
	MII_read_reg(dm8606_phy_addr[2], 0x01);
	phy_reg_1 = MII_read_reg(dm8606_phy_addr[2], 0x01);  // 2 time
	
	if(phy_reg_1 & 0x04) 
	{
#ifdef PRINT_LINK_STATUS
		phy_reg_4 = MII_read_reg(dm8606_phy_addr[2], 0x04);
		phy_reg_5 = MII_read_reg(dm8606_phy_addr[2], 0x05);
		phy_link_check = phy_reg_4 & phy_reg_5;
			
	    printf("port 2 link is ok ==> %sMbps %s mode \n", 
	       phy_link_check & ((1 << 8) | (1 << 7)) ? "100" : "10",
	       phy_link_check & ((1 << 8) | (1 << 6)) ? "FULL" : "HALF");
#endif
		return 1;
	}
	else
	{
#ifdef PRINT_LINK_STATUS
	   	printf("\n port 2 link is fail\n");
#endif
    	return  0;
    }
}

u8_t CH_DMPORT3_LINKUP(void)
{
	U32 phy_reg_1;
	U32 phy_reg_4;
	U32 phy_reg_5;
	U32 phy_link_check;
	
	MII_read_reg(dm8606_phy_addr[3], 0x01);
	phy_reg_1 = MII_read_reg(dm8606_phy_addr[3], 0x01);  // 2 time
	
	if(phy_reg_1 & 0x04) 
	{
#ifdef PRINT_LINK_STATUS
		phy_reg_4 = MII_read_reg(dm8606_phy_addr[3], 0x04);
		phy_reg_5 = MII_read_reg(dm8606_phy_addr[3], 0x05);
		phy_link_check = phy_reg_4 & phy_reg_5;
			
	    printf("port 3 link is ok ==> %sMbps %s mode \n", 
	       phy_link_check & ((1 << 8) | (1 << 7)) ? "100" : "10",
	       phy_link_check & ((1 << 8) | (1 << 6)) ? "FULL" : "HALF");
#endif
		return 1;
	}
	else
	{
#ifdef PRINT_LINK_STATUS
	   	printf("\n port 3 link is fail\n");
#endif
    	return  0;
    }
}


u8_t CH_DMPORT4_LINKUP(void)
{
	
}
