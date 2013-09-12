
#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <asm/semaphore.h>
#else
#include <assert.h>
#endif

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
#include "fdma7200.h"

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

#if !defined (ST_7200)
    if(DeviceNo == STFDMA_1)
#endif
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
#if defined (ST_OSLINUX)
                    iowrite32(MetaTable[j].Table[i], ((void*)(ptr + i)));
#else
                    ptr[i]= MetaTable[j].Table[i];
#endif
                }
                /* clear unused memory */
                for(; i < MetaTable[j].Length; i++)
                {
#if defined (ST_OSLINUX)
                    iowrite32(0, ((void*)(ptr + i)));
#else
                    ptr[i]= 0;
#endif
                }
              }
              case 0x1:
              {
                ptr= (device_t *)((char*)ProgAddr + MetaTable[j].Base);
                for (i= 0; i < MetaTable[j].Used; i++)
                {
#if defined (ST_OSLINUX)
                    iowrite32(MetaTable[j].Table[i], ((void*)(ptr + i)));
#else
                    ptr[i]= MetaTable[j].Table[i];
#endif
                }
                /* clear unused memory */
                for(; i < MetaTable[j].Length; i++)
                {
#if defined (ST_OSLINUX)
                    iowrite32(0, ((void*)(ptr + i)));
#else
                    ptr[i]= 0;
#endif
                }
              }
              default:
              {
                break;        /* ignore all other table types */
              }
            }
        }
    }
#if defined (ST_5525)
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
#if defined (ST_OSLINUX)
                    iowrite32(MetaTable1[j].Table[i], ((void*)(ptr + i)));
#else
                    ptr[i]= MetaTable1[j].Table[i];
#endif
                }
                /* clear unused memory */
                for(; i < MetaTable1[j].Length; i++)
                {
#if defined (ST_OSLINUX)
                    iowrite32(0, ((void*)(ptr + i)));
#else
                    ptr[i]= 0;
#endif
                }
              }
              case 0x1:
              {
                ptr= (device_t *)((char*)ProgAddr + MetaTable1[j].Base);
                for (i= 0; i < MetaTable1[j].Used; i++)
                {
#if defined (ST_OSLINUX)
                    iowrite32(MetaTable1[j].Table[i], ((void*)(ptr + i)));
#else
                    ptr[i]= MetaTable1[j].Table[i];
#endif
                }
                /* clear unused memory */
                for(; i < MetaTable1[j].Length; i++)
                {
#if defined (ST_OSLINUX)
                    iowrite32(0, ((void*)(ptr + i)));
#else
                    ptr[i]= 0;
#endif
                }
              }
              default:
              {
                break;        /* ignore all other table types */
              }
            }
        }
    }
#endif
#if !defined (ST_7200)
    else
    {
        ;
    }
#endif

/*#if defined (ST_OSLINUX)
    printk(KERN_ALERT "FDMA firmware version is 0x%08X\n", (U32)ioread32(DataAddr));
#else
    DO_DEBUG(printf("[id:%08X]\n", *(device_t*)DataAddr);)
#endif*/

    return 0;
}
