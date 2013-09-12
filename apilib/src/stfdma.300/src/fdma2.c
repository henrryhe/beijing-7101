/* For cut detection */
#include "stcommon.h"

#if  defined(ST_5517)
/* Stub - This loader is not used for STi5517 */
#include "fdma5100.h"

#elif defined(ST_5100) || defined (ST_5101) || defined (ST_5301)
#include "fdma5100.h"

#elif defined (ST_5105)
#include "fdma5105.h"

#elif defined (ST_5107)
#include "fdma5107.h"

#elif defined (ST_5188)
#include "fdma5188.h"

#elif defined (ST_5162)
#include "fdma5162.h"

#elif defined (ST_7710)
#include "fdma7710.h"

#elif defined (ST_7100)
#include "fdma7100.h"

#elif defined (ST_7109)
#include "fdma7109.h"

#elif defined (ST_5525)
#include "fdma5525_1.h"
#include "fdma5525_2.h"

#elif defined (ST_7200)
#include "fdma7200_1.h"
#include "fdma7200_2.h"

#elif defined (ST_7111)
#include "fdma7111_1.h"
#include "fdma7111_2.h"

#elif defined (ST_7105)
#include "fdma7105_1.h"
#include "fdma7105_2.h"

#else
#error Unsupported device
#endif

#include "local.h"

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#if !defined(ST_OSLINUX)
#pragma ST_device(device_t)
#endif
#endif

typedef volatile unsigned long device_t;

/*#define DEBUG*/

#if defined(DEBUG)
#define DO_DEBUG(x) x
#else
#define DO_DEBUG(x)
#endif

int stfdma_FDMA2Conf(void *ProgAddr, void *DataAddr, STFDMA_Block_t DeviceNo)  /* Need to place code and data separately */
{
    device_t *ptr;	/* pointer to a 32 bit device register */
    int i;
    int j;

    assert(sizeof(device_t) == 4);

    DO_DEBUG(printf("Load %08X %08X ", ProgAddr, DataAddr);)

#if defined (ST_7109)
#if !defined(ST_498) /*Some times ST_GetCutRevision() returns wrong value causing wrong F/W load*/
    if(ST_GetCutRevision() < 0xC0)
    {
        MetaTable[0].Table      = DATA_2_0;
        MetaTable[0].Name       = "DATA";
        MetaTable[0].Mode       = DATA_MODE;
        MetaTable[0].Base       = DATA_BASE_ADDR;
        MetaTable[0].Used       = TABLE_LEN(DATA_2_0);
        MetaTable[0].Length     = DATA_LENGTH_2_0;

        MetaTable[1].Table      = PROG_2_0;
        MetaTable[1].Name       = "PROG";
        MetaTable[1].Mode       = PROG_MODE;
        MetaTable[1].Base       = PROG_BASE_ADDR;
        MetaTable[1].Used       = TABLE_LEN(PROG_2_0);
        MetaTable[1].Length     = PROG_LENGTH_2_0;
    }
    else if(ST_GetCutRevision() >= 0xC0)  
#else
	/*to remove warnings on 498*/
	MetaTable[0].Table      = DATA_2_0;
	MetaTable[1].Table      = PROG_2_0;
#endif
    {  
        MetaTable[0].Table      = DATA_3_0;
        MetaTable[0].Name       = "DATA";
        MetaTable[0].Mode       = DATA_MODE;
        MetaTable[0].Base       = DATA_BASE_ADDR;
        MetaTable[0].Used       = TABLE_LEN(DATA_3_0);
        MetaTable[0].Length     = DATA_LENGTH_3_0;

        MetaTable[1].Table      = PROG_3_0;
        MetaTable[1].Name       = "PROG";
        MetaTable[1].Mode       = PROG_MODE;
        MetaTable[1].Base       = PROG_BASE_ADDR;
        MetaTable[1].Used       = TABLE_LEN(PROG_3_0);
        MetaTable[1].Length     = PROG_LENGTH_3_0;
    }

    MetaTable[2].Table      = (unsigned long *)VERSION;
    MetaTable[2].Name       = "VERSION";
    MetaTable[2].Mode       = VERSION_MODE;
    MetaTable[2].Base       = VERSION_BASE_ADDR;
    MetaTable[2].Used       = TABLE_LEN((unsigned long *)VERSION);
    MetaTable[2].Length     = VERSION_LENGTH;
#endif

    if(DeviceNo == STFDMA_1)
    {
        for (j = 0; (j < TABLE_LEN(MetaTable)); j++)
        {
            /* init memory */
            switch(MetaTable[j].Mode)
            {
              case 0x0:
              {
                ptr= (device_t *)((char*)DataAddr + MetaTable[j].Base);
                for (i= 0; i < MetaTable[j].Used; i++)
                {
                    STSYS_WriteRegDev32LE(((void*)(ptr + i)),(MetaTable[j].Table[i]));
                }
                /* clear unused memory */
                for(; i < MetaTable[j].Length; i++)
                {
                    STSYS_WriteRegDev32LE(((void*)(ptr + i)), 0);
                }
              }
              case 0x1:
              {
                ptr= (device_t *)((char*)ProgAddr + MetaTable[j].Base);
                for (i= 0; i < MetaTable[j].Used; i++)
                {
                    STSYS_WriteRegDev32LE(((void*)(ptr + i)), (MetaTable[j].Table[i]));
                }
                /* clear unused memory */
                for(; i < MetaTable[j].Length; i++)
                {
                    STSYS_WriteRegDev32LE(((void*)(ptr + i)), 0);
                }
              }
              default:
              {
                break;        /* ignore all other table types */
              }
            }
        }
    }
#if defined (ST_5525) || defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
    else if(DeviceNo == STFDMA_2)
    {
        for (j = 0; (j < TABLE_LEN(MetaTable1)); j++)
        {
            /* init memory */
            switch(MetaTable1[j].Mode)
            {
              case 0x0:
              {
                ptr= (device_t *)((char*)DataAddr + MetaTable1[j].Base);
                for (i= 0; i < MetaTable1[j].Used; i++)
                {
                    STSYS_WriteRegDev32LE(((void*)(ptr + i)), (MetaTable1[j].Table[i]));
                }
                /* clear unused memory */
                for(; i < MetaTable1[j].Length; i++)
                {
                     STSYS_WriteRegDev32LE(((void*)(ptr + i)), 0);
                }
              }
              case 0x1:
              {
                ptr= (device_t *)((char*)ProgAddr + MetaTable1[j].Base);
                for (i= 0; i < MetaTable1[j].Used; i++)
                {
                    STSYS_WriteRegDev32LE(((void*)(ptr + i)), (MetaTable1[j].Table[i]));
                }
                /* clear unused memory */
                for(; i < MetaTable1[j].Length; i++)
                {
                    STSYS_WriteRegDev32LE(((void*)(ptr + i)), 0);
                }
              }
              default:
              {
                break;        /* ignore all other table types */
              }
            }
        }
    }
#else
    else
    {
        ;
    }
#endif

/*#if defined (ST_OSLINUX)
    printk(KERN_ALERT "FDMA firmware version is 0x%08X\n", (U32)STSYS_ReadRegDev32LE(DataAddr));
#else
    DO_DEBUG(printf("[id:%08X]\n", *(device_t*)DataAddr);)
#endif*/

    return 0;
}

ST_ErrorCode_t STFDMA_GetFirmwareRevision(STFDMA_Block_t DeviceNo, U32 *FWRevision)
{
#ifndef STFDMA_NO_PARAMETER_CHECK
	if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
#endif
	
#ifndef STFDMA_NO_PARAMETER_CHECK
    if (FWRevision == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

	if (DeviceNo == STFDMA_1)
	{
#if defined (ST_7109)
		*FWRevision = DATA_3_0[0];
#else
		*FWRevision = DATA[0];
#endif
	}
#if defined (ST_5525) || defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
	else if (DeviceNo == STFDMA_2)
	{
		*FWRevision = DATA1[0];
	}
#endif
	return ST_NO_ERROR;
}
