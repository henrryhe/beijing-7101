#include <linux/types.h>
#include <linux/slab.h>

#include <asm/uaccess.h>    /* copy (to|from) user etc. */
#include <asm/semaphore.h>
#include <asm/io.h>         /* writel and friends.      */

#include "kernel-wrapper.h"

#define KMALLOC_LIMIT       (128 * 1024)

long int traceEnabled = 0;

/******************************************************************************/

int semaphore_init_fifo (semaphore_t* Semaphore, int Count)
{
	init_waitqueue_head(&(Semaphore->wait_queue));
    Semaphore->Count = 1 - Count;

    return 0;
}

/******************************************************************************/

int semaphore_init_fifo_timeout (semaphore_t* Semaphore, int Count)
{
	init_waitqueue_head(&(Semaphore->wait_queue));
    Semaphore->Count = 1 - Count;

    return 0;
}

/******************************************************************************/

int semaphore_wait (semaphore_t* Semaphore)
{
    wait_event_interruptible(Semaphore->wait_queue, Semaphore->Count <= 0);
    (Semaphore->Count)++;
            
    return 0;
}

/******************************************************************************/

int     semaphore_wait_timeout (semaphore_t* Semaphore, const clock_t* timeout_p)
{
    int ret = -1;
    
    if (TIMEOUT_IMMEDIATE == timeout_p)
    {
        if (wait_event_interruptible_timeout(Semaphore->wait_queue, Semaphore->Count <= 0, 1) != 0)
    	{
            (Semaphore->Count)++;
            ret = 0;
        }
    }
    else if (TIMEOUT_INFINITY == timeout_p) /* evil */
    {
        ret = semaphore_wait(Semaphore);
    }
    else
    {
        clock_t timeout;

        timeout = time_minus (*timeout_p, time_now());

        if ((U32)timeout > 0x80000000)
        {
            printk("\n\n\nWARNING!!!\nTimeout may have looped (%d) ...\nPlease check ...\n\n\n", (int)timeout);
        }
    
    	if (wait_event_interruptible_timeout(Semaphore->wait_queue, Semaphore->Count <= 0, timeout) != 0)
    	{
            (Semaphore->Count)++;
            ret = 0;
        }
    }

    return ret;
}

/******************************************************************************/

int semaphore_signal (semaphore_t* Semaphore)
{
    (Semaphore->Count)--;
    wake_up_interruptible(&(Semaphore->wait_queue));

    return 0;
}

/******************************************************************************/

void    semaphore_delete (semaphore_t* Semaphore)
{
    /*kfree(Semaphore);*/
}

/******************************************************************************/

void*   memory_allocate (partition_t* Partition, size_t Requested)
{
    void *result;

    if (Requested < KMALLOC_LIMIT)
    {
        result = kmalloc(Requested, GFP_KERNEL);
    }
    else
    {
        printk(KERN_ALERT "memory_allocate - asked to allocate %i bytes\n",
                Requested);
        printk(KERN_ALERT "returning NULL\n");
        result = NULL;
    }

    return result;
}

void*   memory_allocate_clear (
    partition_t*    Partition, 
    size_t          num_elems, 
    size_t          element_size)
{
    void*   result;
    size_t  Requested = num_elems * element_size;

    if (Requested < KMALLOC_LIMIT)
    {
        result = kmalloc(Requested, GFP_KERNEL);
        if (result != NULL)
        {
            memset(result, 0, Requested);
        }
    }
    else
    {
        printk(KERN_ALERT "memory_allocate_clear - asked to allocate %i bytes\n",
                Requested);
        printk(KERN_ALERT "returning NULL\n");
        result = NULL;
    }

    return result;
}

void    memory_deallocate (partition_t* Partition, void* Block)
{
    kfree(Block);
}

/* STSYS wrapping */
u8 STSYS_ReadRegDev8(u32 address)
{
    u8 byte = 0;
    rmb();

    byte = readb(address);

    if (traceEnabled)
        printk(KERN_ALERT "Read byte 0x%02x from 0x%08x\n", byte, address);
    return byte;
}

void STSYS_WriteRegDev8(u32 address, u8 value)
{
    writeb(value, address);
    if (traceEnabled)
        printk(KERN_ALERT "Wrote 0x%02x to 0x%08x\n", value, address);
    wmb();
}

u16 STSYS_ReadRegDev16LE(u32 address)
{
    u16 word = 0;
    rmb();
    word = readw(address);
    if (traceEnabled)
        printk(KERN_ALERT "Read short word 0x%04x from 0x%08x\n", word, address);
    return word;
}

void STSYS_WriteRegDev16LE(u32 address, u16 value)
{
    writew(value, address);
    if (traceEnabled)
        printk(KERN_ALERT "Wrote 0x%04x to 0x%08x\n", value, address);
    wmb();
}

u32 STSYS_ReadRegDev32LE(u32 address)
{
    u32 bigword = 0;

    rmb();

    bigword = readl(address);

    if (traceEnabled)
        printk(KERN_ALERT "Read 0x%08x from 0x%08x\n", bigword, address);
    return bigword;
}

void STSYS_WriteRegDev32LE(u32 address, u32 value)
{
    writel(value, address);
    if (traceEnabled)
        printk(KERN_ALERT "Wrote 0x%08x to 0x%08x\n", value, address);
    wmb();
}

#if 0
void STEVT_Notify(STEVT_Handle_t eventhandle, 
                  STEVT_EventID_t eventid, void *ptr)
{
}

ST_ErrorCode_t STEVT_UnsubscribeDeviceEvent (STEVT_Handle_t Handle,
                                             const ST_DeviceName_t RegistrantName,
                                             STEVT_EventConstant_t Event)
{
    return 0;
}

ST_ErrorCode_t STEVT_Close (STEVT_Handle_t Handle)
{
    return 0;
}
#endif

void sttbx_Print(const char *format, ...)
{
#if 1
    int ret;
    va_list args;
    char string[512];

    va_start(args, format);
    ret = vsnprintf(string, 512, format, args);
    va_end(args);

    if (ret > 512)
    {
        printk(KERN_ALERT "Formatted string too long\n");
    }
    else
    {
        if (string[strlen(string) - 1] != '\n')
            printk(KERN_ALERT "%s\n", string);
        else
            printk(KERN_ALERT "%s", string);
    }
#endif
}

void sttbx_Report(int level, const char *format, ...)
{
#if 1
    int ret;
    va_list args;
    char string[512];

    va_start(args, format);
    ret = vsnprintf(string, 512, format, args);
    va_end(args);

    if (ret > 512)
    {
        printk(KERN_ALERT "Formatted string too long\n");
    }
    else
    {
        if (string[strlen(string) - 1] != '\n')
            printk(KERN_ALERT "%s\n", string);
        else
            printk(KERN_ALERT "%s", string);
    }
#endif
}
