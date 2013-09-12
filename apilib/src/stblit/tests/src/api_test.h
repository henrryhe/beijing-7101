/* api_test.h                                    */
/*                                               */
/*                                               */
/* --------------------------------------------- */

/* Define to prevent recursive inclusion */
#ifndef __API_TEST_H
#define __API_TEST_H

/* Include ----------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */


#ifdef __cplusplus
extern "C" {
#endif


/* Exported Macros and Defines -------------------------------------------- */
#if defined(mb317) || defined(mb317a) || defined(mb317b)
	#define TEST_SHARED_MEM_BASE_ADDRESS           NULL	/*For mb317 The SDRAM_BASE_ADDRESS is not defined*/
#else
    #define TEST_SHARED_MEM_BASE_ADDRESS           SDRAM_BASE_ADDRESS
#endif

#define WORK_BUFFER_SIZE   (40 * 16) /* The workbuffer is used in the driver for storing 5-TAP filter coefficients */

/* Exported Types --------------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

#ifdef __cplusplus
}
#endif


#endif /* __API_TEST_H */
