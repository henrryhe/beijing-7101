#include "appltype.h"
#include "CHDRV_NET.h"
#include "CHDRV_NET_MAC110.h"
#include "initterm.h"
#include "stpio.h"

#define SMI_DEALY           1
#define SMI_CLK             (1 << 4)
#define SMI_DATA            (1 << 3)
#define SMI_DAT_BIT         32

extern STPIO_Handle_t g_PHYPIOhandle;
extern ST_DeviceName_t PIO_DeviceName[] ;
void smi_write_1bit (U8 w_data);
void dm8606_int(void)
{
#if 0
    STPIO_OpenParams_t ST_OpenParams;
	int i_error;
    U8 i;
    ST_OpenParams.ReservedBits    = PIO_BIT_3 | PIO_BIT_4|PIO_BIT_6;
    ST_OpenParams.BitConfigure[3] = STPIO_BIT_BIDIRECTIONAL ;/*DATA*/
    ST_OpenParams.BitConfigure[4] = STPIO_BIT_OUTPUT;        /*CLK*/
    ST_OpenParams.BitConfigure[6] = STPIO_BIT_OUTPUT;        /*RESET*/
    ST_OpenParams.IntHandler = NULL;
    i_error = STPIO_Open(PIO_DeviceName[2], &ST_OpenParams, &g_PHYPIOhandle );
	if(i_error)
		do_report(0,"dm8606_int error\n");

    STPIO_Set(g_PHYPIOhandle,1 << 6);
    MILLI_DELAY(1000);

#endif	
    U8 i;
    for(i = 0 ; i < 100; i++)
    {
        smi_write_1bit(1);
    }
    
}

void smi_write_1bit (U8 w_data)
{
    if(w_data & 0x01)
        STPIO_Set(g_PHYPIOhandle, SMI_DATA);
    else
        STPIO_Clear(g_PHYPIOhandle, SMI_DATA);
    STPIO_Clear(g_PHYPIOhandle, SMI_CLK);
 //  MILLI_DELAY(SMI_DEALY);



    STPIO_Set(g_PHYPIOhandle, SMI_CLK);
  // MILLI_DELAY(SMI_DEALY);
}


U8 smi_read_1bit (void)
{
    U8 bit_data;

    STPIO_Set(g_PHYPIOhandle, SMI_CLK);
  //MILLI_DELAY(SMI_DEALY);



    STPIO_Clear(g_PHYPIOhandle, SMI_CLK);
       STPIO_Read(g_PHYPIOhandle, &bit_data);
   //MILLI_DELAY(SMI_DEALY);

    if(bit_data & SMI_DATA)
        return 1;
    else
        return 0;
}


void dm8606_write_reg(U16 reg_addr, U32 reg_data)
{
    U8 i;
    U32 data_mask;
    
    //Preamble 32 1 
    for(i = 0 ; i < 35; i++)
    {
        smi_write_1bit(1);
    }
    
    //star
    smi_write_1bit(0);
    smi_write_1bit(1);
    
    //Opcode
    smi_write_1bit(0);
    smi_write_1bit(1);
    
    //register addr
    data_mask = (1 << 9);
    for(i = 0 ; i < 10; i++)
    {
        if(reg_addr & (data_mask))
            smi_write_1bit(1);
        else
            smi_write_1bit(0);
        data_mask >>= 1;
    }

    //TA
    smi_write_1bit(1);
    smi_write_1bit(0);

    //register data
    data_mask = (1 << (SMI_DAT_BIT - 1));
    for(i = 0; i < SMI_DAT_BIT; i++)
    {
        if(reg_data & (data_mask))
            smi_write_1bit(1);
        else
            smi_write_1bit(0);
        data_mask >>= 1;
    } 

    //TA
    smi_read_1bit();
    smi_read_1bit();
    smi_read_1bit();
    smi_read_1bit();


    return;
}

U32 dm8606_read_reg(U16 reg_addr)
{
    U8 i;
    U32 reg_data = 0;
    U32 data_mask;

    //Preamble 32 1 
    for(i = 0 ; i < 35; i++)
    {
        smi_write_1bit(1);
    }
    
    //star
    smi_write_1bit(0);
    smi_write_1bit(1);
    
    //Opcode
    smi_write_1bit(1);
    smi_write_1bit(0);

    //register addr
    data_mask = (1 << 9);
    for(i = 0 ; i < 10; i++)
    {
        if(reg_addr & (data_mask))
            smi_write_1bit(1);
        else
            smi_write_1bit(0);
        //data_mask >>= 1 ;
        reg_addr <<= 1; 
    }

      //TA
//  smi_read_1bit();
 // MILLI_DELAY(SMI_DEALY);
  //  MILLI_DELAY(SMI_DEALY);
    smi_read_1bit();
    smi_read_1bit();
// smi_write_1bit(0);
     
    //register data
    for(i = 0; i < SMI_DAT_BIT; i++)
    {
        reg_data <<= 1;
        if(smi_read_1bit())
            reg_data |= 0x01;
    }

    //TA
    smi_read_1bit();
    smi_read_1bit();
    smi_read_1bit();
    smi_read_1bit();
 
    return reg_data;
}


void CH_GetPortStatus(void)
{
	U32 ui_status;

  //   dm8606_write_reg(0x01,0x01);
// dm8606_write_reg(0x03,0x03);
#if 1
 

	ui_status = dm8606_read_reg(0x01);
    do_report(0,"[0x1] 1  = %x\n",ui_status);
	ui_status = dm8606_read_reg(0x01 | 0x01 << 7);
    do_report(0,"[0x1] 2 = %x\n",ui_status);
	ui_status = dm8606_read_reg(0x01 | 0x02 << 7);
    do_report(0,"[0x1] 3 = %x\n",ui_status);
	ui_status = dm8606_read_reg(0x01 | 0x03 << 7);
    do_report(0,"[0x1] 4 = %x\n",ui_status);

/*  
  ui_status = dm8606_read_reg(0x03);
    do_report(0,"[0x3] = %x\n",ui_status);
	ui_status = dm8606_read_reg(0x05);
    do_report(0,"[0x5] = %x\n",ui_status);

		ui_status = dm8606_read_reg(0x07);
    do_report(0,"[0x7] = %x\n",ui_status);
		ui_status = dm8606_read_reg(0x08);
    do_report(0,"[0x8] = %x\n",ui_status);


	ui_status = dm8606_read_reg(0x201);
    do_report(0,"[0x201] = %x\n",ui_status);
    ui_status = dm8606_read_reg(0x221);
    do_report(0,"[0x221] = %x\n",ui_status);
	ui_status = dm8606_read_reg(0x241);
    do_report(0,"[0x241] = %x\n",ui_status);

		ui_status = dm8606_read_reg(0x261);
    do_report(0,"[0x261] = %x\n",ui_status);

		ui_status = dm8606_read_reg(0x281);
    do_report(0,"[0x281] = %x\n",ui_status);
*/
#else
 		ui_status = dm8606_read_reg(0xa0);
    do_report(0,"[0xa0] = %x\n",ui_status);

		ui_status = dm8606_read_reg(0xa1);
    do_report(0,"[0xa1] = %x\n",ui_status);
#endif
}


void CH_Test1(void)
{
    U32 ui_status;
	int i;
for ( i = 0x00 ; i < 0x3f; i++)
{
    if((i % 8) == 0x00) do_report(0,"\n  reg %02x : " , i);
    ui_status = dm8606_read_reg(i);
    do_report(0," %08lx",ui_status);
}

}
void CH_Test2(void)
{
    U32 ui_status;
	int i;
for ( i = 0x0 ; i < 0x3; i++)
{
    if((i % 8) == 0x00) do_report(0,"\n  reg %02x : " , i);
    ui_status = dm8606_read_reg(i | 1 << 9 );
    do_report(0," %08lx",ui_status);
}

}
u8_t CH_DMPORT0_LINKUP(void)
{
U32 port_stauts;
port_stauts = dm8606_read_reg(0x1 | 1 << 9); 

if(port_stauts & (1 << 0))
{
    do_report(0,"port 0 link is ok ==> %sMbps %s mode \n", 
       port_stauts & (1 << 1) ? "100" : "10",
       port_stauts & (1 << 2) ? "FULL" : "HALF");
       return 1;
}
else
{
   // do_report(0,"\n port 0 link is fail");
    return  0;
}

}
u8_t CH_DMPORT1_LINKUP(void)
{
 U32 port_stauts;
port_stauts = dm8606_read_reg(0x1 | 1 << 9); 



if(port_stauts & (1 << 8))
{
    do_report(0,"port 1 link is ok ==> %sMbps %s mode \n", 
       port_stauts & (1 << 9) ? "100" : "10",
       port_stauts & (1 << 10) ? "FULL" : "HALF");
	return 1;
       
}
else
{
   // do_report(0,"\n port 1 link is fail");
    return 0;
}


}
u8_t CH_DMPORT2_LINKUP(void)
{
 U32 port_stauts;
port_stauts = dm8606_read_reg(0x1 | 1 << 9); 



if(port_stauts & (1 << 16))
{
    do_report(0,"port 2 link is ok ==> %sMbps %s mode \n", 
       port_stauts & (1 << 17) ? "100" : "10",
       port_stauts & (1 << 18) ? "FULL" : "HALF");
	return 1;
}
else
{
  //  do_report(0,"\n port 2 link is fail");
	return 0;
}
}
u8_t CH_DMPORT3_LINKUP(void)
{
   U32 port_stauts;
port_stauts = dm8606_read_reg(0x1 | 1 << 9); 


if(port_stauts & (1 << 24))
{
    do_report(0,"port 3 link is ok ==> %sMbps %s mode \n", 
       port_stauts & (1 << 17) ? "100" : "10",
       port_stauts & (1 << 18) ? "FULL" : "HALF");
	return 1;
}
else
{
  //  do_report(0,"\n port 3 link is fail");
	return 0;
}
}
u8_t CH_DMPORT4_LINKUP(void)
{
  
}


