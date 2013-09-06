/******************************************************************************* */
/*                                                                               */
/*                                                                               */
/* TITLE:       bootrom.s                                                        */
/*                                                                               */
/* PURPOSE:     First code run upon reset:                                       */
/*              - initialize CPU (clocks, memory, etc.)                          */
/*              - copy user program in RAM                                       */
/*              - branch to user start address.                                  */
/*                                                                               */
/*              Clock and memory configuration is performed according to a       */
/*              table (__memory_setup_table, at the end of this file), which is  */
/*              interpreted in real-time by a configuration loop, and has to be  */
/*              updated for each hardware platform.                              */
/*                                                                               */
/*              This table consists in a suite of 'instructions', each of which  */
/*              being made of an opcode and a variable number of arguments. The  */
/*              end of the table is signalled using a special opcode (0). Macros */
/*              are defined for each instruction to help filling the table;      */
/*              currently, the following instructions are recognized:            */
/*                                                                               */
/*   Setup table macro                        Operation                          */
/*   --------------------------------------------------------------------------- */
/*   POKE_UPDATE_LONG(A1, A2, AND, SHIFT, OR) *A1 = ((*A2 & AND) << SHIFT) | OR; */
/*   WHILE_NE(A, AND, VAL)                    while ((*A & AND) != VAL) ;        */
/*   UPDATE_LONG(A, AND, OR)                  *A = (*A & AND) | OR;              */
/*   OR_LONG(A, V)                            *A = *A | V;                       */
/*   POKE_LONG(A, V)                          *(long*)A = (long)V;               */
/*   POKE_SHORT(A, V)                         *(short*)A = (short)V;             */
/*   POKE_CHAR(A, V)                          *(char*)A = (char)V;               */
/*   END_MARKER                               <<< End of table >>>               */
/*                                                                               */
/*                                                                               */
/*********************************************************************************/

#define _SH4REG_ASM_
#include "cocoref_v2_7109_registers.h"

/* Macro definitions */
/* ----------------- */
#define POKE_UPDATE_LONG(A1, A2, AND, SHIFT, OR)    .long 8, A1, A2, AND, SHIFT, OR
#define WHILE_NE(A, AND, VAL)                       .long 7, A, AND, VAL
#define UPDATE_LONG(A, AND, OR)                     .long 6, A, AND, OR
#define OR_LONG(A, V)                               .long 5, A, V
#define POKE_LONG(A, V)                             .long 4, A, V
#define POKE_SHORT(A, V)                            .long 2, A, V
#define POKE_CHAR(A, V)                             .long 1, A, V
#define END_MARKER                                  .long 0, 0, 0

###########################################################################
############################ START OF BOOSTRAP ############################
###########################################################################

/* ========================================================================
   Name:        main
   Description: main is MAIN. Entry point of the application.
   ======================================================================== */

    .section    .text                     ! Section name
    .org        0x0                       ! Entry point

    /* Stop the watchdog timer */
    /* ----------------------- */
stop_watchdog:
    mov.l   wtcsr_address, r0
    mov.w   wtcsr_off, r1
    mov.w   r1, @r0
    
    /* I-cache & D-cache on; D-cache in write through */
    /* ---------------------------------------------- */
enable_cache:
    mov.l   ccr_address, r0
    mov.l   ccr_mode, r1
    mov.l   r1, @r0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    /* Now we can continue executing the program in cache... */
    /* ----------------------------------------------------- */
jump_in_cache:
    mova    1f, r0                      ! R6 = address in P2 space (0xA...)
    mov.l   p2_to_p1_mask, r3           ! R3 = P2 -> P1 mapping mask
    and     r3, r0                      ! R6 = cacheable address (0x8...)
    jmp     @r0                         ! Go!
    .balign 4 
1:  nop
    
    /* Insert the loop table in the cache */
    /* ---------------------------------- */
    /*    
     * Setting up the memory interfaces may cause the EMI (where this FLASH
     * image resides) to briefly hang, so it is necessary to execute the
     * configuration loop in program cache, and to have the whole configuration
     * table into data cache before starting the loop.
     *
     * Since the configuration loop is compact enough and written is such a way
     * that it is completely pulled into instruction cache on its first iteration
     * (however, to be completely safe, first instruction in configuration table
     * should not cause an EMI hang).
     *
     * To get the configuration table into data cache, we just read it all once.
     */

pull_table_in_cache:
    mov.l   p2_to_p1_mask, r3               ! R3 = P2 -> P1 mapping mask

    /* Check chip version */
    /* ------------------ */                

    mov.l   chip_metalid2,r0                ! R0 = Chip id register
    mov.l   @r0,r0                          ! R0 = Chip id value
    and     #0xF,r0                         ! R0 = R0&0xF = Chip cut value
0:  cmp/eq  #4,r0                           ! If (R0==0)
    bf      1f
    mov.l   data_start_address_cut_4_x, r1  ! R1 = table start address for cut 2
    mov.l   data_end_address_cut_4_x, r2    ! R2 = table end address for cut 2
    bra	    2f
	nop
1:  
    mov.l   data_start_address_cut_3_x, r1  ! R1 = table start address for cut 3
    mov.l   data_end_address_cut_3_x, r2    ! R2 = table end address for cut 3
    
2:  and     r3, r1                          ! R1 = cached table start address
    and     r3, r2                          ! R2 = cached table end address
    mov     r1, r5                          ! R5 = scratch data pointer (we have to
3:  mov.l   @r5+, r4                        ! keep r1 unchanged for config_loop)
    cmp/eq  r5, r2
    bf      3b

    /* Perform the poke loop table */
    /* --------------------------- */
    /*
     * It is now safe to call the configuration loop with real data, since it
     * and its associated data table are in onchip caches. Setting
     * up the memory interfaces may cause the EMI (where this FLASH
     * image resides) to briefly hang - but the CPU will be safely
     * executing from cache should this happen.
     */
call_config:
    mov     #0, r9                      ! <<<<< Debug >>>>>
    bsr     config_loop                 ! Execute configuration loop
    nop
    
/* Henry code start*/

    
/* Loading STA12 CFG15 addr*/
    
    mov.l   sys_sta12_addr, r3
    mov.l   sys_sta12_mask, r5  
    mov.l   sys_cfg15_addr, r2

    mov.l   dlldefaultdata, r0
    mov.l   r0, @r2

 
/* r0 = opcode */
/* r1 = poke table */
/* r2 = addr1 (alias SYS_CFG15) */
/* r3 = addr2 (alias SYS_STA12) */  

/* begin DLL2 adjust loop ----------------------------- */

dll_reg_n_mask_ready:
mov #32, r4 /* r4 = loop count */
mov #0, r6 /* r6 = accumulator */
/* From this point: */
/* r0, r7 are used as scratch regs */
2:
/*test code start*/
/*mov.l r3, @r1*/
/*test code end*/
mov #3,r0 /* DLL2 Freeze mask into r4 (0x000C0000) */
shll16 r0
shll2 r0
mov.l @r2, r7 /* SYS_CFG15 --> r7 */
or r0, r7
mov.l r7, @r2 /* Freeze DLL2 by writing SYS_CFG15 */
/* small sleep - count up to 64K */
mov #1,r0
swap.w r0, r0
3: add #-1,r0
cmp/eq #0, r0
bf 3b
/* Jump to 1: when loading in I-cache */
/* skip the Jump when executing ... */
bra skip_load1
1:
nop
bf 1f /* False condition still true so touch next cache line */
skip_load1:
/* get the STA12 register and accumulate */
mov.l @r3, r0
and r5, r0
shlr8 r0 /* shift right by 10 */
shlr2 r0
/*test code start*/
/*mov.l r0, @r1+*/
/*test code end*/
add r0, r6 /* accumulate into r6 */
/* UnFreeze DLL2 by wrinting SYS_CFG15 */
mov #3, r7 /* Build DLL2 Unfreeze mask into r7 (0x00080000) */
shll16 r7
shll2 r7
not r7, r0 /* r0 = 0xfff3ffff */
mov.l @r2, r7 /* SYS_CFG15 --> r7 */
and r0, r7
mov.l r7, @r2 /* Un Freeze DLL2 by writing SYS_CFG15 */
/* Jump to 1: when loading in I-cache */
/* skip the Jump when executing ... */
bra skip_load2
1:
nop
bf 1f /* False condition still true so touch next cache line */
skip_load2:
dt r4 /* perform this loop 32 times */
bf 2b
mov r6, r0 /* divide by 32 + b[19]:1 + b[18]:0 */
shlr2 r0
shlr2 r0
or #1, r0 /* set bit 19 = 1 (Force user cmd) */
shll r0 /* set bit 18 = 0 */
mov r0, r6
shll16 r6
shll2 r6
mov.l @r2, r7 /* SYS_CFG15 --> r7 */
/* May be not necessary, but safer... */
/* The following block of 7 instructions is */
/* used to reset bit 18->28 of r7 (SYS_CFG15) */
/* before to set them to the right value (r6) */
mov #7, r0
shll8 r0
or #0xff, r0
shll16 r0
shll2 r0 /* r0 = 0x1ffc0000 */
not r0, r0 /* r0 = 0xe003ffff */
and r0, r7
or r6, r7
/*test code start*/
/*mov #4, r0
add r0, r1
mov.l r7, @r1*/
/*test code end*/
mov.l r7, @r2
mov #0, r0 /* just to continue safely.... */
/* end of DLL2 adjust loop ----------------------------------- */
1:  
bra after_all_patch

##############################################
#################Henry Data #################
##############################################

    .balign 4    

clk_pll1_addr:		.long 0xB9213024
clk_pll1_cut3:		.long 0x0001ED10
clk_pll1_cut4:		.long 0x00012C03
clk_pll1_mask:		.long 0xfff80000
dlldefaultdata:		.long 0x04d80000
sys_sta12_addr:		.long 0xB9001038  
sys_sta12_mask:		.long 0x0007FC00
sys_cfg15_addr:		.long 0xB900113c
chip_metalid2:		.long 0xB900102C 
/*dll_reg_setup_end_addr:		.long dll_reg_setup_end
dll_reg_n_mask_ready_addr:	.long dll_reg_n_mask_ready  */

after_all_patch:
 
    /* Copy the code in DDR */
    /* -------------------- */
call_copy:
    /*bsr     copy_code  20070618 add for LOADER compress boot*/                 ! Copy user program in RAM
    nop

    /* Jump to user start address */
    /* -------------------------- */    
jump_to_user:
    mov.l   dbg_connected_address, r0   ! Zero out dbg connected flag
    mov.l   @r0,r0
    mov     #0,r1                       ! To prevent comms with MicroConnect
    mov.l   r1,@r0
    mov.l   start_address, r0           ! Jump to user start address in RAM
    mov.l   @r0,r0
    jmp     @r0
    nop

###########################################################################
############################# END OF BOOSTRAP #############################
###########################################################################

/* ========================================================================
   Name:        config_loop
   Description: Parse the poke loop table and execute it 
   ======================================================================== */

    .balign 4    
config_loop:                ! do {
    add     #1, r9          ! <<<<< Debug >>>>>
    mov.l   @r1+, r0        ! opcode  = *table++; /* R0 = opcode */
    mov.l   @r1+, r2        ! address = *table++; /* R2 = address */
    mov.l   @r1+, r3        ! operand = *table++; /* R3 = operand */
    cmp/eq  #0, r0          ! if (opcode == END_MARKER)
    bf      1f              ! {
    rts                     !   return;
    nop                     ! }
1:  cmp/eq  #4, r0          ! if (opcode ==  POKE_LONG)
    bf      1f              ! {
    mov.l   r3, @r2         !   *(long*)address = (long)operand;
1:  cmp/eq  #2, r0          ! if (opcode == POKE_SHORT)
    bf      1f              ! {
    mov.w   r3, @r2         !   *(short*)address = (short)operand;
                            ! }
1:  cmp/eq  #1, r0          ! if (opcode == POKE_CHAR)
    bf      1f              ! {
    mov.b   r3, @r2         !   *(char*)address = (char)operand;
                            ! }
1:  cmp/eq  #5, r0          ! if (opcode == OR_LONG)
    bf      1f              ! {
    mov.l   @r2,r4          !   temp = *address;
    or      r3,r4           !   temp = temp | operand;
    mov.l   r4,@r2          !   *address = temp;
                            ! }
1:  cmp/eq  #6, r0          ! if (opcode == UPDATE_LONG)
    bf      1f              ! {
    mov.l   @r2,r4          !   temp = *address;   
    and     r3,r4           !   temp = temp & operand;
    mov.l   @r1+,r3         !   operand = *table++; /* Get next operand */
    or      r3,r4           !   temp = temp | operand;
    mov.l   r4,@r2          !   *address = temp;
                            ! }
1:  cmp/eq  #8, r0          ! if (opcode == POKE_UPDATE_LONG)
    bf      1f              ! {
    mov.l   @r3,r4          !   temp = *operand; /* First operand is an address */   
    mov.l   @r1+,r3         !   operand = *table++;
    and     r3,r4           !   temp = temp & operand;
    mov.l   @r1+,r3         !   operand = *table++;
    shld    r3,r4           !   temp = temp << operand;
    mov.l   @r1+,r3         !   operand = *table++;
    or      r3,r4           !   temp = temp | operand;
    mov.l   r4,@r2          !   *address = temp;
                            ! }
1:  cmp/eq  #7, r0          ! if (opcode == WHILE_NE)
    bf      1f              ! {
    mov.l   @r1+,r5         !   temp2 = *table++;
2:  mov.l   @r2,r4          !   do {    temp  = *address;
    and     r3,r4           !           temp  = temp & operand;
    cmp/eq  r4,r5           !   } while ( temp != temp2 );
    bf      2b              ! }
1:  mov     r0, r7          ! <<<<< Debug >>>>>
    mov     #1,r0           ! r0 = 1;
    swap.w  r0, r0          ! r0 = 65536;
2:  add     #-1,r0          ! do {  r0 = r0 - 1;
    cmp/eq  #0, r0          ! } while (r0 != 0);
    bf      2b              !
    bt      config_loop     ! } while (1); /* infinite loop */
    nop

/* ========================================================================
   Name:        copy_code
   Description: Copy the code in DDR 
   ======================================================================== */

    .balign 4
copy_code:
    mov.l   load_address , r0           ! Get load address in RAM
    mov.l   @r0, r1                     ! r1 = load_addr;
  /*20070502 del mova    ram_to_flash_mask , r0      ! Source address in FLASH 
    mov.l   @r0, r2                     !
    and     r1, r2                      ! r2 = load_addr & 0xf000ffff;
	*/

/*20070502 add*/
	mova   ram_to_flash_mask, r0
    mov.l   @r0, r2
/********/
    mov.l   image_size_address , r0     ! Image size
    mov.l   @r0, r3                     ! r3 = image_size;
    add     #3, r3                      ! Round to higher multiple of 4
    shlr2   r3                          ! Divide by 4 to get size in long words
1:  mov.l   @r2+, r4                    ! r4 = *src_addr++;
    mov.l   r4, @r1                     ! *load_addr = r4;
    dt      r3                          ! image_size--;
    bf/s    1b                          ! Continue loop if image_size != 0
    add     #4, r1                      ! load_addr++; /* long ptr incr = +4 */
    rts                                 ! return;
    nop

###########################################################################
################################### DATAS #################################
###########################################################################

/* Chip version */
/* ------------ */
                        .balign 4
chip_deviceid:          .long 0xB9001000
chip_status12:          .long 0xB9001010
chip_status13:          .long 0xB9001014

/* Watchdog register address */
/* ------------------------- */
wtcsr_address:          .long ST40_CPG_WTCSR
wtcsr_off:              .word 0xa500

/* Cache configuration registers */
/* ----------------------------- */
                        .balign 4   
ccr_address:            .long SH4_CCN_CCR
ccr_mode:               .long 0x8000090b 
                                         
/* Table addresses */
/* --------------- */
p2_to_p1_mask:              .long ~0x20000000
data_start_address_cut_2_x: .long __memory_setup_table_cut_2_x
data_end_address_cut_2_x:   .long __memory_setup_table_cut_2_x_end
data_start_address_cut_3_x: .long __memory_setup_table_cut_3_x
data_end_address_cut_3_x:   .long __memory_setup_table_cut_3_x_end
data_start_address_cut_4_x: .long __memory_setup_table_cut_4_x
data_end_address_cut_4_x:   .long __memory_setup_table_cut_4_x_end



###########################################################################

/* User program */
/* ------------ */
load_address:           .long 0xA0120FF4
image_size_address:     .long 0xA0120FF8
start_address:          .long 0xA0120FFC
dbg_connected_address:  .long 0xA0121000
/*ram_to_flash_mask:    .long 0xF000FFFF */
ram_to_flash_mask:      .long 0x80001000 /*zxg 20070502 add */
###########################################################################
######################### POKE TABLE FOR CUT>=2.0 #########################
###########################################################################

    .balign 4
__memory_setup_table_cut_2_x:

    /* CLOCKGENA Setup PLL0 - 531Mhz */
    /* ----------------------------- */
    POKE_LONG(STB7109_CLOCKGENA_LOCK       , 0xC0DE)
    OR_LONG(STB7109_CLOCKGENA_PLL0_CFG     , 0x00100000)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG , 0xFFF7FFFF,0)
	UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG, 0xfff80000, 0x14 | (0xb1 << 8) | (0x0 << 16))
    OR_LONG(STB7109_CLOCKGENA_PLL0_CFG     , 0x00080000)
    WHILE_NE(STB7109_CLOCKGENA_PLL0_STATUS , 0x00000001,0x00000001)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG , 0xFFEFFFFF,0)
    POKE_LONG(STB7109_CLOCKGENA_LOCK       , 0x0)

    /* CLOCKGENA Setup PLL1 - 400Mhz */
    /* ----------------------------- */
    POKE_LONG(STB7109_CLOCKGENA_LOCK        , 0xC0DE)
    POKE_LONG(STB7109_CLOCKGENA_PLL1_BYPASS , 0x00000002)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL1_CFG  , 0xFFF7FFFF,0)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL1_CFG  , 0xFFF80000,0x1E|(0xC8<<8)|(0x00<<16))
    OR_LONG(STB7109_CLOCKGENA_PLL1_CFG      , 0x00080000)
    WHILE_NE(STB7109_CLOCKGENA_PLL1_STATUS  , 0x00000001,0x00000001)
    POKE_LONG(STB7109_CLOCKGENA_PLL1_BYPASS , 0x00000000)
    POKE_LONG(STB7109_CLOCKGENA_LOCK        , 0x0)

    /* SYSCONF Setup */
    /* ------------- */
    POKE_LONG(STB7109_SYSCONF_SYS_CFG11 , 0x080780C0)

    WHILE_NE(STB7109_SYSCONF_SYS_STA12  , (1<<9)|(1<<19),(1<<9)|(1<<19))
    WHILE_NE(STB7109_SYSCONF_SYS_STA13  , (1<<9)|(1<<19),(1<<9)|(1<<19))
    POKE_LONG(STB7109_SYSCONF_SYS_CFG12 , 0x4000000F|(0xF<<12)|(0xF<<23))
    POKE_LONG(STB7109_SYSCONF_SYS_CFG13 , 0x4000000F|(0xF<<12)|(0xF<<23))

    /* INTERCONNECT Setup */
    /* ------------------ */
    POKE_LONG(0xB9216200 , 0x1)
    POKE_LONG(0xB9216204 , 0x5)
    POKE_LONG(0xB9216208 , 0x3)
    POKE_LONG(0xB921620C , 0x2)
    POKE_LONG(0xB9216210 , 0x4)
    POKE_LONG(0xB9243000 , 0x4)
    POKE_LONG(0xB9243004 , 0x3)
    POKE_LONG(0xB9243008 , 0x2)
    POKE_LONG(0xB924300C , 0x1)
    POKE_LONG(0xB9243010 , 0x5)
    
    /* LMI SYS Setup (200Mhz) */
    /* ---------------------- */
    /*POKE_LONG(ST40_LMI_MIM_0(SYS)   , 0x061A0247)*/
    /*POKE_LONG(ST40_LMI_MIM_1(SYS)   , 0x01010000)*/
    POKE_LONG(ST40_LMI_MIM_0(SYS)   , 0x861A0247)
    POKE_LONG(ST40_LMI_MIM_1(SYS)   , 0x01010022)
    POKE_LONG(ST40_LMI_STR_0(SYS)   , 0x35B06455)
    /*FOR LMI 64 M BYTE*/
    /*POKE_LONG(ST40_LMI_SDRA0_0(SYS) , 0x08001900)*/
    /*POKE_LONG(ST40_LMI_SDRA1_0(SYS) , 0x08001900)*/
    /*FOR LMI 128 M BYTE*/
	POKE_LONG(ST40_LMI_SDRA0_0(SYS) , 0x010001a00)
    POKE_LONG(ST40_LMI_SDRA1_0(SYS) , 0x010001a00)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000003)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000002)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000402)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000133)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000002)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000033)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000000)

    /* LMI VID Setup (200Mhz) */
    /* ---------------------- */
    /*POKE_LONG(ST40_LMI_MIM_0(VID)   , 0x061A0247)*/
    /*POKE_LONG(ST40_LMI_MIM_1(VID)   , 0x01010000)*/
    POKE_LONG(ST40_LMI_MIM_0(VID)   , 0x861A0247)
    POKE_LONG(ST40_LMI_MIM_1(VID)   , 0x01010022)
    POKE_LONG(ST40_LMI_STR_0(VID)   , 0x35B06455)
    /* 修改支持128 MBYTE POKE_LONG(ST40_LMI_SDRA0_0(VID) , 0x14001900)*/
    /* 修改支持128 MBYTE POKE_LONG(ST40_LMI_SDRA1_0(VID) , 0x14001900) */
    POKE_LONG(ST40_LMI_SDRA0_0(VID) ,  0x1c001a00)
    POKE_LONG(ST40_LMI_SDRA1_0(VID) ,  0x1c001a00)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000003)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000002)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000402)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000133)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000002)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000033)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000000)

    /* EMI Setup */
    /* --------- */
    POKE_LONG(ST40_EMI_BANK_ENABLE          , 0x00000005)
    POKE_LONG(ST40_EMI_BANK0_BASEADDRESS    , 0x00000000)
    POKE_LONG(ST40_EMI_BANK1_BASEADDRESS    , 0x00000008)
    POKE_LONG(ST40_EMI_BANK2_BASEADDRESS    , 0x00000009)
    POKE_LONG(ST40_EMI_BANK3_BASEADDRESS    , 0x0000000A)
    POKE_LONG(ST40_EMI_BANK4_BASEADDRESS    , 0x0000000C)
    POKE_LONG(ST40_EMI_GENCFG               , 0x00000010)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA0 , 0x001016D1)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA1 , 0x9D200000)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA2 , 0x9D220000)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA0 , 0x00021791)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA1 , 0x10004141)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA2 , 0x10004141)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA0 , 0x042086F1)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA1 , 0x88112111)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA2 , 0x88112211)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA0 , 0x00021791)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA1 , 0x08004141)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA2 , 0x08004141)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA0 , 0x041086F1)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA1 , 0x93001111)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA2 , 0x91001111)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA3 , 0x00000000)
    END_MARKER
__memory_setup_table_cut_2_x_end:

###########################################################################
######################### POKE TABLE FOR CUT>=3.0 #########################
###########################################################################

    .balign 4
__memory_setup_table_cut_3_x:

    /* CLOCKGENA Setup PLL0 - 531Mhz */
    /* ----------------------------- */
    POKE_LONG(STB7109_CLOCKGENA_LOCK       , 0xC0DE)
    OR_LONG(STB7109_CLOCKGENA_PLL0_CFG     , 0x00100000)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG , 0xFFF7FFFF,0)
	UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG, 0xfff80000, 0x14 | (0xb1 << 8) | (0x0 << 16))
    OR_LONG(STB7109_CLOCKGENA_PLL0_CFG     , 0x00080000)
    WHILE_NE(STB7109_CLOCKGENA_PLL0_STATUS , 0x00000001,0x00000001)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG , 0xFFEFFFFF,0)
    POKE_LONG(STB7109_CLOCKGENA_LOCK       , 0x0)

    /* CLOCKGENA Setup PLL1 - 400Mhz */
    /* ----------------------------- */
    POKE_LONG(STB7109_CLOCKGENA_LOCK        , 0xC0DE)
    POKE_LONG(STB7109_CLOCKGENA_PLL1_BYPASS , 0x00000002)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL1_CFG  , 0xFFF7FFFF,0)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL1_CFG  , 0xFFF80000,0x1E|(0xC8<<8)|(0x00<<16))
    OR_LONG(STB7109_CLOCKGENA_PLL1_CFG      , 0x00080000)
    WHILE_NE(STB7109_CLOCKGENA_PLL1_STATUS  , 0x00000001,0x00000001)
    POKE_LONG(STB7109_CLOCKGENA_PLL1_BYPASS , 0x00000000)
    POKE_LONG(STB7109_CLOCKGENA_LOCK        , 0x0)

    /* SYSCONF Setup */
    /* ------------- */
    /*POKE_LONG(STB7109_SYSCONF_SYS_CFG11 , 0x0C2FFEFF)*/
    POKE_LONG(STB7109_SYSCONF_SYS_CFG11 , 0x080780C0)
    WHILE_NE(STB7109_SYSCONF_SYS_STA12  , (1<<9)|(1<<19),(1<<9)|(1<<19))
    WHILE_NE(STB7109_SYSCONF_SYS_STA13  , (1<<9)|(1<<19),(1<<9)|(1<<19))
    POKE_LONG(STB7109_SYSCONF_SYS_CFG12 , 0x4000000F|(0xF<<12)|(0xF<<23))
    POKE_LONG(STB7109_SYSCONF_SYS_CFG13 , 0x4000000F|(0xF<<12)|(0xF<<23))

/* INTERCONNECT Setup */
    /* ------------------ */
   POKE_LONG(0xB9216200 , 0x1)
    POKE_LONG(0xB9216204 , 0x5)
    POKE_LONG(0xB9216208 , 0x3)
    POKE_LONG(0xB921620C , 0x2)
    POKE_LONG(0xB9216210 , 0x4)
    POKE_LONG(0xB9243000 , 0x4)
    POKE_LONG(0xB9243004 , 0x3)
    POKE_LONG(0xB9243008 , 0x2)
    POKE_LONG(0xB924300C , 0x1)
    POKE_LONG(0xB9243010 , 0x5)

    /* LMI SYS Setup (200Mhz) */
    /* ---------------------- */
    /*POKE_LONG(ST40_LMI_MIM_0(SYS)   , 0x061A0247)*/
    /*POKE_LONG(ST40_LMI_MIM_1(SYS)   , 0x01010000)*/
    POKE_LONG(ST40_LMI_MIM_0(SYS)   , 0x861A0247)
    POKE_LONG(ST40_LMI_MIM_1(SYS)   , 0x01010022)
    POKE_LONG(ST40_LMI_STR_0(SYS)   , 0x35B06455)
    /*FOR LMI 64 M BYTE*/
    /*POKE_LONG(ST40_LMI_SDRA0_0(SYS) , 0x08001900)*/
    /*POKE_LONG(ST40_LMI_SDRA1_0(SYS) , 0x08001900)*/
    /*FOR LMI 128 M BYTE*/
	POKE_LONG(ST40_LMI_SDRA0_0(SYS) , 0x010001a00)
    POKE_LONG(ST40_LMI_SDRA1_0(SYS) , 0x010001a00)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000003)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000002)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000402)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000133)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000002)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000033)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000000)

  /* LMI VID Setup (200Mhz) */
    /* ---------------------- */
    /*POKE_LONG(ST40_LMI_MIM_0(VID)   , 0x061A025F)*/
    /*POKE_LONG(ST40_LMI_MIM_1(VID)   , 0x00000005)*/
    POKE_LONG(ST40_LMI_MIM_0(VID)   , 0x861A0247)
    POKE_LONG(ST40_LMI_MIM_1(VID)   , 0x01010022)
    POKE_LONG(ST40_LMI_STR_0(VID)   , 0x35B06455)
    /* 修改支持128 MBYTE POKE_LONG(ST40_LMI_SDRA0_0(VID) , 0x14001900)*/
    /* 修改支持128 MBYTE POKE_LONG(ST40_LMI_SDRA1_0(VID) , 0x14001900) */
    POKE_LONG(ST40_LMI_SDRA0_0(VID) ,  0x1c001a00)
    POKE_LONG(ST40_LMI_SDRA1_0(VID) ,  0x1c001a00)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000003)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000002)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000402)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000133)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000002)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000033)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000000)

    /* EMI Setup */
    /* --------- */
    POKE_LONG(ST40_EMI_BANK_ENABLE          , 0x00000005)
    POKE_LONG(ST40_EMI_BANK0_BASEADDRESS    , 0x00000000)
    POKE_LONG(ST40_EMI_BANK1_BASEADDRESS    , 0x00000008)
    POKE_LONG(ST40_EMI_BANK2_BASEADDRESS    , 0x00000009)
    POKE_LONG(ST40_EMI_BANK3_BASEADDRESS    , 0x0000000A)
    POKE_LONG(ST40_EMI_BANK4_BASEADDRESS    , 0x0000000C)
    POKE_LONG(ST40_EMI_GENCFG               , 0x00000010)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA0 , 0x001016D1)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA1 , 0x9D200000)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA2 , 0x9D220000)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA0 , 0x00021791)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA1 , 0x10004141)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA2 , 0x10004141)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA0 , 0x042086F1)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA1 , 0x88112111)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA2 , 0x88112211)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA0 , 0x00021791)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA1 , 0x08004141)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA2 , 0x08004141)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA0 , 0x041086F1)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA1 , 0x93001111)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA2 , 0x91001111)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA3 , 0x00000000)
    
    END_MARKER
__memory_setup_table_cut_3_x_end:


###########################################################################
######################### POKE TABLE FOR CUT>=4.0 #########################
###########################################################################

    .balign 4
__memory_setup_table_cut_4_x:

    /* CLOCKGENA Setup PLL0 - 531Mhz */
    /* ----------------------------- */
    POKE_LONG(STB7109_CLOCKGENA_LOCK       , 0xC0DE)
    OR_LONG(STB7109_CLOCKGENA_PLL0_CFG     , 0x00100000)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG , 0xFFF7FFFF,0)
   	UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG, 0xfff80000, 0x14 | (0xb1 << 8) | (0x0 << 16)) 
    OR_LONG(STB7109_CLOCKGENA_PLL0_CFG     , 0x00080000)
    WHILE_NE(STB7109_CLOCKGENA_PLL0_STATUS , 0x00000001,0x00000001)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL0_CFG , 0xFFEFFFFF,0)
    POKE_LONG(STB7109_CLOCKGENA_LOCK       , 0x0)

    /* CLOCKGENA Setup PLL1 - 400Mhz */
    /* ----------------------------- */
    POKE_LONG(STB7109_CLOCKGENA_LOCK        , 0xC0DE)
    POKE_LONG(STB7109_CLOCKGENA_PLL1_BYPASS , 0x00000002)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL1_CFG  , 0xFFF7FFFF,0)
    UPDATE_LONG(STB7109_CLOCKGENA_PLL1_CFG  , 0xFFF80000,0x1E|(0xC8<<8)|(0x00<<16))

    OR_LONG(STB7109_CLOCKGENA_PLL1_CFG      , 0x00080000)
    WHILE_NE(STB7109_CLOCKGENA_PLL1_STATUS  , 0x00000001,0x00000001)
    POKE_LONG(STB7109_CLOCKGENA_PLL1_BYPASS , 0x00000000)
    POKE_LONG(STB7109_CLOCKGENA_LOCK        , 0x0)

    /* SYSCONF Setup */
    /* ------------- */
    /*POKE_LONG(STB7109_SYSCONF_SYS_CFG11 , 0x0C2FFEFF)*/
    POKE_LONG(STB7109_SYSCONF_SYS_CFG11 , 0x080780C0)
    WHILE_NE(STB7109_SYSCONF_SYS_STA12  , (1<<9)|(1<<19),(1<<9)|(1<<19))
    WHILE_NE(STB7109_SYSCONF_SYS_STA13  , (1<<9)|(1<<19),(1<<9)|(1<<19))
    POKE_LONG(STB7109_SYSCONF_SYS_CFG12 , 0x4000000F|(0xF<<12)|(0xF<<23))
    POKE_LONG(STB7109_SYSCONF_SYS_CFG13 , 0x4000000F|(0xF<<12)|(0xF<<23))
    POKE_LONG(STB7109_SYSCONF_SYS_CFG15 , 0x14d80000)

/* INTERCONNECT Setup */
    /* ------------------ */
      POKE_LONG(0xB9216200 , 0x1)
    POKE_LONG(0xB9216204 , 0x5)
    POKE_LONG(0xB9216208 , 0x3)
    POKE_LONG(0xB921620C , 0x2)
    POKE_LONG(0xB9216210 , 0x4)
    POKE_LONG(0xB9243000 , 0x4)
    POKE_LONG(0xB9243004 , 0x3)
    POKE_LONG(0xB9243008 , 0x2)
    POKE_LONG(0xB924300C , 0x1)
    POKE_LONG(0xB9243010 , 0x5)

    /* LMI SYS Setup (200Mhz) */
    /* ---------------------- */
    /*POKE_LONG(ST40_LMI_MIM_0(SYS)   , 0x061A0247)*/
    /*POKE_LONG(ST40_LMI_MIM_1(SYS)   , 0x01010000)*/
    POKE_LONG(ST40_LMI_MIM_0(SYS)   , 0x861A0247)
    POKE_LONG(ST40_LMI_MIM_1(SYS)   , 0x01010022)
    POKE_LONG(ST40_LMI_STR_0(SYS)   , 0x35B06455)
    /*FOR LMI 64 M BYTE*/
    /*POKE_LONG(ST40_LMI_SDRA0_0(SYS) , 0x08001900)*/
    /*POKE_LONG(ST40_LMI_SDRA1_0(SYS) , 0x08001900)*/
    /*FOR LMI 128 M BYTE*/
	POKE_LONG(ST40_LMI_SDRA0_0(SYS) , 0x010001a00)
    POKE_LONG(ST40_LMI_SDRA1_0(SYS) , 0x010001a00)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000003)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000002)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000402)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000133)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000002)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000004)
    POKE_LONG(ST40_LMI_SDMR0(SYS)   , 0x00000033)
    POKE_LONG(ST40_LMI_SCR_0(SYS)   , 0x00000000)

    /* LMI VID Setup (200Mhz) */
    /* ---------------------- */
    /*POKE_LONG(ST40_LMI_MIM_0(VID)   , 0x061A0247)*/
    /*POKE_LONG(ST40_LMI_MIM_1(VID)   , 0x01010000)*/
    POKE_LONG(ST40_LMI_MIM_0(VID)   , 0x861A0247)
    POKE_LONG(ST40_LMI_MIM_1(VID)   , 0x01010022)
    POKE_LONG(ST40_LMI_STR_0(VID)   , 0x35B06455)
    /* 修改支持128 MBYTE POKE_LONG(ST40_LMI_SDRA0_0(VID) , 0x14001900)*/
    /* 修改支持128 MBYTE POKE_LONG(ST40_LMI_SDRA1_0(VID) , 0x14001900) */
    POKE_LONG(ST40_LMI_SDRA0_0(VID) ,  0x1c001a00)
    POKE_LONG(ST40_LMI_SDRA1_0(VID) ,  0x1c001a00)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000003)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000001)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000002)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000402)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000133)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000002)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000004)
    POKE_LONG(ST40_LMI_SDMR0(VID)   , 0x00000033)
    POKE_LONG(ST40_LMI_SCR_0(VID)   , 0x00000000)

    /* EMI Setup */
    /* --------- */
   POKE_LONG(ST40_EMI_BANK_ENABLE          , 0x00000005)
    POKE_LONG(ST40_EMI_BANK0_BASEADDRESS    , 0x00000000)
    POKE_LONG(ST40_EMI_BANK1_BASEADDRESS    , 0x00000008)
    POKE_LONG(ST40_EMI_BANK2_BASEADDRESS    , 0x00000009)
    POKE_LONG(ST40_EMI_BANK3_BASEADDRESS    , 0x0000000A)
    POKE_LONG(ST40_EMI_BANK4_BASEADDRESS    , 0x0000000C)
    POKE_LONG(ST40_EMI_GENCFG               , 0x00000010)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA0 , 0x001016D1)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA1 , 0x9D200000)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA2 , 0x9D220000)
    POKE_LONG(ST40_EMI_BANK0_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA0 , 0x00021791)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA1 , 0x10004141)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA2 , 0x10004141)
    POKE_LONG(ST40_EMI_BANK1_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA0 , 0x042086F1)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA1 , 0x88112111)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA2 , 0x88112211)
    POKE_LONG(ST40_EMI_BANK2_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA0 , 0x00021791)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA1 , 0x08004141)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA2 , 0x08004141)
    POKE_LONG(ST40_EMI_BANK3_EMICONFIGDATA3 , 0x00000000)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA0 , 0x041086F1)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA1 , 0x93001111)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA2 , 0x91001111)
    POKE_LONG(ST40_EMI_BANK4_EMICONFIGDATA3 , 0x00000000)
    END_MARKER
__memory_setup_table_cut_4_x_end:
    .end

