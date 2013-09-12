/*****************************************************************************

File name : sys_test.c

Description :  tests of system module API

COPYRIGHT (C) STMicroelectronics 2000.

 Date              Modification                                     Name
 ----              ------------                                     ----
02 Sep 2000        Created                                          HG
12 Jul 2000        Added STSYS_GetRevision()                        HG
19 Sep 2001        Add more print reports to control errors under   HSdLM
 *                 compilation option CPU Big Endian
*****************************************************************************/


/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include "stsys.h"
#include "stdevice.h"

#ifdef ST_OSWINCE
#define ST20_INTERNAL_MEMORY_SIZE            (4*1024)
#define KnActorPrivilege int
int actorPrivilege(int actor, void *actorP, int val) { return 0; }
#define K_MYACTOR	1
#define K_OK		0
#define K_SUPACTOR	1
#else
#endif
/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/*#define STSYS_ASSEMBLY*/

/* Private Variables (static)------------------------------------------------ */

/* Allow room for OS20 segments in internal memory */
#if ! (defined (ST_7100) || defined(ST_7109) || defined(ST_7200))
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
#endif
#if !defined(ST40_OS21)
static ST_Partition_t   the_internal_partition;
#endif /* ST40_OS21 */
ST_Partition_t          *internal_partition = NULL;

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
#if !defined(ST40_OS21)
static ST_Partition_t   the_system_partition;
#endif /* ST40_OS21 */
ST_Partition_t          *system_partition = NULL;

#define NCACHE_MEMORY_SIZE          0x80000
static unsigned char    NCacheMemory [NCACHE_MEMORY_SIZE];
#if !defined(ST40_OS21)
static ST_Partition_t   the_ncache_partition;
#endif /* ST40_OS21 */
ST_Partition_t          *ncache_partition = NULL;

#ifdef ARCHITECTURE_ST20
#pragma ST_section      (InternalBlock, "internal_section")
#pragma ST_section      (ExternalBlock, "system_section")
#pragma ST_section      (NCacheMemory , "ncache_section")
#endif /* ARCHITECTURE_ST20 */

#ifdef ARCHITECTURE_ST40
    static U32 TheTarget;
#endif

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
void sys_test(void *ptr);

/* Functions ---------------------------------------------------------------- */
void sys_test(void *ptr)
{
    volatile U8 u8;
    volatile U16 u16;
    volatile U32 u32;
    volatile void * Address_p;
    BOOL TheresErrors = FALSE;
    ST_Revision_t RevisionNumber;
#if defined(ST40_OS21)
	int result;
#endif /* ST40_OS21 */

   UNUSED_PARAMETER(ptr);

#ifdef ARCHITECTURE_ST20
    /* OS20 init */
    kernel_initialize();
    kernel_start();
#endif /* ARCHITECTURE_ST20 */

#if defined(ST40_OS21)
   /* Initialize the kernel */
	result = kernel_initialize(NULL);

	if (result != OS21_SUCCESS)
    {
        printf("error: Cannot initialise OS21 kernel !\n");
        return;
    }
    /* Start the kernel */
	result = kernel_start();
	if (result != OS21_SUCCESS)
    {
        printf("error: Cannot start OS21 kernel !\n");
        return;
    }
#endif /* ST40_OS21 */

#if defined(ST40_OS21) && defined(ST_5528)
    internal_partition = NULL;
    internal_partition = partition_create_heap(internal_block, sizeof(internal_block));
    if (internal_partition == NULL)
    {
        printf("Failed to create internal partition!\n");
        return;
    }
    system_partition = NULL;
    system_partition = partition_create_heap(external_block, sizeof(external_block));
    if (system_partition == NULL)
    {
        printf("Failed to create system partition!\n");
        return;
    }
    ncache_partition = NULL;
#if defined ST_5528
    ncache_partition = partition_create_heap((char *) ST40_NOCACHE_NOTRANSLATE(NCacheMemory), sizeof(NCacheMemory));
#else
    #error No chip defined supporting OS21 !
#endif
    if (NCacheMemory == NULL)
    {
        printf("Failed to create ncache partition!\n");
        return;
    }
#endif /* ST40_OS21 */

#ifdef ARCHITECTURE_ST20
    internal_partition = &the_internal_partition;
    partition_init_simple(&the_internal_partition, (U8*) InternalBlock, sizeof(InternalBlock));
    system_partition = &the_system_partition;
    partition_init_heap(&the_system_partition, (U8*) ExternalBlock, sizeof(ExternalBlock));
    ncache_partition = &the_ncache_partition;
    partition_init_heap(&the_ncache_partition, (U8*) NCacheMemory, sizeof(NCacheMemory));
#endif /* ARCHITECTURE_ST20 */

    printf("======================================================================\n");
    printf("                       STSYS Test Application \n");
#if defined(DVD_STAPI_DEBUG)
    printf("                       %s - %s \n", __DATE__, __TIME__  );
#endif /*DVD_STAPI_DEBUG*/
#if defined(TESTED_PLATFORM) && defined (TESTED_CHIP)
    printf("             Tested platform : Chip %s on board %s\n", TESTED_CHIP, TESTED_PLATFORM);
#endif
#ifdef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE
    printf("                         CPU is Big Endian\n");
#else
    printf("                        CPU is Little Endian\n");
#endif
#ifdef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION
    printf("       Accesses are not optimized (only byte per byte accesses)\n");
#else
    printf("            Accesses are optimized (16 and 32 bits accesses)\n");
#endif
#ifdef STSYS_NO_PRAGMA
    printf("                    #pragma directive un-defined.\n");
#endif
    printf("======================================================================\n");

    RevisionNumber = STSYS_GetRevision();
    printf("STSYS_GetRevision() : %s\n", RevisionNumber);

#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; ldc 0; } /* Delimiter */
#endif

#ifdef ARCHITECTURE_ST20
    Address_p = (void *) ST20_MEMSTART;
#else
    Address_p = (void *) &TheTarget;
#endif

    /* Used with -S option to check assembly generated for the macros */
    printf("Starting STSYS tests...\n");

#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_ReadRegDev8() ... \t");
    *((volatile U32*) Address_p) = 0x12345678;
    u8 = STSYS_ReadRegDev8(Address_p);
    if (u8 != 0x78)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u8, 0x78);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_ReadRegMem8() ... \t");
    *(volatile U32*) Address_p = 0x98765432;
    u8 = STSYS_ReadRegMem8(Address_p);
    if (u8 != 0x32)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u8, 0x32);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_WriteRegDev8() ... \t");
    *(volatile U32*) Address_p = 0xA5A5A5A5;
    u8 = 0x27;
    STSYS_WriteRegDev8(Address_p, u8);
    u8 = (*(volatile U32*) Address_p & 0xFF);
    if (u8 != 0x27)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u8, 0x27);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_WriteRegMem8() ... \t");
    u8 = 0x72;
    STSYS_WriteRegMem8(Address_p, u8);
    u8 = (*(volatile U32*) Address_p & 0xFF);
    if (u8 != 0x72)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u8, 0x72);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_ReadRegDev16BE() ... \t");
    *(volatile U32*) Address_p = 0x12345678;
    u16 = STSYS_ReadRegDev16BE(Address_p);
    if (u16 != 0x7856)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u16, 0x7856);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_ReadRegMem16BE() ... \t");
    *(volatile U32*) Address_p = 0x98765432;
    u16 = STSYS_ReadRegMem16BE(Address_p);
    if (u16 != 0x3254)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u16, 0x3254);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_ReadRegDev16LE() ... \t");
    *(volatile U32*) Address_p = 0x12345678;
    u16 = STSYS_ReadRegDev16LE(Address_p);
    if (u16 != 0x5678)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u16, 0x5678);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_ReadRegMem16LE() ... \t");
    *(volatile U32*) Address_p = 0x98765432;
    u16 = STSYS_ReadRegMem16LE(Address_p);
    if (u16 != 0x5432)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u16, 0x5432);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_WriteRegDev16BE() ... \t");
    *(volatile U32*) Address_p = 0xA5A5A5A5;
    u16 = 0x2719;
    STSYS_WriteRegDev16BE(Address_p, u16);
    u16 = (*(volatile U32*) Address_p & 0xFFFF);
    if (u16 != 0x1927)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u16, 0x1927);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_WriteRegMem16BE() ... \t");
    u16 = 0x7291;
    STSYS_WriteRegMem16BE(Address_p, u16);
    u16 = (*(volatile U32*) Address_p & 0xFFFF);
    if (u16 != 0x9172)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u16, 0x9172);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_WriteRegDev16LE() ... \t");
    *(volatile U32*) Address_p = 0xA5A5A5A5;
    u16 = 0x1639;
    STSYS_WriteRegDev16LE(Address_p, u16);
    u16 = (*(volatile U32*) Address_p & 0xFFFF);
    if (u16 != 0x1639)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u16, 0x1639);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_WriteRegMem16LE() ... \t");
    u16 = 0x6193;
    STSYS_WriteRegMem16LE(Address_p, u16);
    u16 = (*(volatile U32*) Address_p & 0xFFFF);
    if (u16 != 0x6193)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u16, 0x6193);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_ReadRegDev24BE() ... \t");
    *(volatile U32*) Address_p = 0x12345678;
    u32 = STSYS_ReadRegDev24BE(Address_p);
    u32 &= 0x00FFFFFF;
    if (u32 != 0x785634)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u32, 0x785634);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_ReadRegMem24BE() ... \t");
    *(volatile U32*) Address_p = 0x98765432;
    u32 = STSYS_ReadRegMem24BE(Address_p);
    u32 &= 0x00FFFFFF;
    if (u32 != 0x325476)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u32, 0x325476);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_ReadRegDev24LE() ... \t");
    *(volatile U32*) Address_p = 0x12345678;
    u32 = STSYS_ReadRegDev24LE(Address_p);
    u32 &= 0x00FFFFFF;
    if (u32 != 0x345678)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u32, 0x345678);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_ReadRegMem24LE() ... \t");
    *(volatile U32*) Address_p = 0x98765432;
    u32 = STSYS_ReadRegMem24LE(Address_p);
    u32 &= 0x00FFFFFF;
    if (u32 != 0x765432)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u32, 0x765432);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_WriteRegDev24BE() ... \t");
    *(volatile U32*) Address_p = 0xA5A5A5A5;
    u32 = 0xA5648721;
    STSYS_WriteRegDev24BE(Address_p, u32);
    u32 = (*(volatile U32*) Address_p & 0xFFFFFF);
    if ( u32 != 0x218764)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u32, 0x218764);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_WriteRegMem24BE() ... \t");
    u32 = 0xA5467812;
    STSYS_WriteRegMem24BE(Address_p, u32);
    u32 = (*(volatile U32*) Address_p & 0xFFFFFF);
    if ( u32 != 0x127846)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u32, 0x127846);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_WriteRegDev24LE() ... \t");
    *(volatile U32*) Address_p = 0xA5A5A5A5;
    u32 = 0xA5134679;
    STSYS_WriteRegDev24LE(Address_p, u32);
    u32 = (*(volatile U32*) Address_p & 0xFFFFFF);
    if ( u32 != 0x134679)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u32, 0x134679);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_WriteRegMem24LE() ... \t");
    u32 = 0xA5316497;
    STSYS_WriteRegMem24LE(Address_p, u32);
    u32 = (*(volatile U32*) Address_p & 0xFFFFFF);
    if ( u32 != 0x316497)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u32, 0x316497);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_ReadRegDev32BE() ... \t");
    *(volatile U32*) Address_p = 0x12345678;
    u32 = STSYS_ReadRegDev32BE(Address_p);
    if (u32 != 0x78563412)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u32, 0x78563412);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_ReadRegMem32BE() ... \t");
    *(volatile U32*) Address_p = 0x98765432;
    u32 = STSYS_ReadRegMem32BE(Address_p);
    if (u32 != 0x32547698)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u32, 0x32547698);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_ReadRegDev32LE() ... \t");
    *(volatile U32*) Address_p = 0x12345678;
    u32 = STSYS_ReadRegDev32LE(Address_p);
    if (u32 != 0x12345678)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u32, 0x12345678);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_ReadRegMem32LE() ... \t");
    *(volatile U32*) Address_p = 0x98765432;
    u32 = STSYS_ReadRegMem32LE(Address_p);
    if (u32 != 0x98765432)
    {
        printf("error: read 0x%0x instead of 0x%0x.\n",u32, 0x98765432);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_WriteRegDev32BE() ... \t");
    *(volatile U32*) Address_p = 0xA5A5A5A5;
    u32 = 0x15482659;
    STSYS_WriteRegDev32BE(Address_p, u32);
    u32 = *(volatile U32*) Address_p;
    if ( u32 != 0x59264815)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u32, 0x59264815);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_WriteRegMem32BE() ... \t");
    u32 = 0x96857452;
    STSYS_WriteRegMem32BE(Address_p, u32);
    u32 = *(volatile U32*) Address_p;
    if ( u32 != 0x52748596)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u32, 0x52748596);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


#ifdef STSYS_ASSEMBLY
    __asm{ ldc 0; } /* Delimiter */
#endif
    printf("STSYS_WriteRegDev32LE() ... \t");
    *(volatile U32*) Address_p = 0xA5A5A5A5;
    u32 = 0x86537542;
    STSYS_WriteRegDev32LE(Address_p, u32);
    u32 = *(volatile U32*) Address_p;
    if ( u32 != 0x86537542)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u32, 0x86537542);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }

    printf("STSYS_WriteRegMem32LE() ... \t");
    u32 = 0x68573524;
    STSYS_WriteRegMem32LE(Address_p, u32);
    u32 = *(volatile U32*) Address_p;
    if ( u32 != 0x68573524)
    {
        printf("error: wrote 0x%0x instead of 0x%0x.\n",u32, 0x68573524);
        TheresErrors = TRUE;
    }
    else
    {
        printf("Ok.\n");
    }


    /* Report summary of results */
    if (TheresErrors)
    {
        printf("There were errors while testing STSYS !\n");
    }
    else
    {
        printf("No error while testing STSYS.\n");
    }
} /* End of sys_test() function */


int main(int argc, char *argv[])
{
#ifdef ST_OS21
    setbuf(stdout, NULL);
    #if 0 /*Not really used*/
        #ifdef ST_OS21
            sys_test(NULL);
        #else
            OS20_main(argc, argv, sys_test);
        #endif  /*End of ST40_OS21*/
    #endif
#endif

   UNUSED_PARAMETER(argc);
   UNUSED_PARAMETER(argv);
   sys_test(NULL);

   printf ("\n --- End of main --- \n");
   fflush (stdout);

    exit (0);
} /* end of main() */

/* End of sys_test.c */
