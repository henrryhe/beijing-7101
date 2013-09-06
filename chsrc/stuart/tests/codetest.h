/* codetest.h */

#define CODETEST_USE_BANK_1
#define CODETEST_5510

#ifdef CODETEST_USE_BANK_1

#ifdef CODETEST_5510

#define CTRL_PORT (volatile unsigned long *)0x50800000;   /* amc_ctrl_port_ */
#define DATA_PORT (volatile unsigned long *)0x50800004;   /* amc_data_port_ */
#define EMI_REG0_VALUE_32 0x449 /* 32 bit EMI */
#define EMI_REG0_VALUE 0x451 /* 16 bit EMI */

#else

#define CTRL_PORT (volatile unsigned long *)0x50000000;   /* amc_ctrl_port_ */
#define DATA_PORT (volatile unsigned long *)0x50000004;   /* amc_data_port_ */

#endif

#define EMI_REG0 0x2010
#define EMI_REG2 0x2018

#elif defined(USE_BANK_2)

#define CTRL_PORT (volatile unsigned long *)0x60000000;   /* amc_ctrl_port_ */
#define DATA_PORT (volatile unsigned long *)0x60000004;   /* amc_data_port_ */
#define EMI_REG0 0x2020
#define EMI_REG2 0x2028
#define EMI_REG0_VALUE 0x451 /* 16 bit EMI */

#else

#error Bank not defined

#endif

/*#ifdef  __amc_ctrl_tag__*/

#pragma ST_device  (amc_ctrl_port)
#pragma ST_device  (amc_data_port)

volatile amctag_t *amc_ctrl_port;
volatile amctag_t *amc_data_port;

/*#endif*/

#define codetest_init() \
{ \
     *(unsigned short *) EMI_REG0 = EMI_REG0_VALUE; \
     *(unsigned short *) EMI_REG2 = 0x100; \
    amc_ctrl_port = CTRL_PORT; \
    amc_data_port = DATA_PORT; \
}

/* End of codetest.h */

