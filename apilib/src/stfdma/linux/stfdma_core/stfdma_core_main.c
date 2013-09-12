/*****************************************************************************
 *
 *  Module      : fdma
 *  Date        : 29-02-2005
 *  Author      : WALKERM
 *  Description :
 *
 *****************************************************************************/

/* Requires   MODULE         defined on the command line */
/* Requires __KERNEL__       defined on the command line */
/* Requires __SMP__          defined on the command line for SMP systems   */
/* Requires EXPORT_SYMTAB    defined on the command line to export symbols */

#include <asm/io.h>
#include <linux/init.h>    /* Initiliasation support */
#include <linux/module.h>  /* Module support */
#include <linux/kernel.h>  /* Kernel support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>      /* File operations (fops) defines */
#include <linux/cdev.h>    /* Charactor device support */
#include <linux/netdevice.h> /* ??? SET_MODULE_OWNER ??? */
#include <linux/ioport.h>  /* Memory/device locking macros   */
#include <linux/dmapool.h> /* DMA Poll management */
#include <linux/bigphysarea.h> /* bigphysarea management */
#include <asm-sh/dma.h>

/* UTS_RELEASE             - Kernel version  (text) */
/* LINUX_VERSION_CODE      - Kernel version  (hex)  */

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/sched.h>   /* Defines pointer (current) to current task */

#include "stfdma_core_types.h"       /* Modules specific defines */

#include "stfdma.h"

#include "stos.h"
#ifndef STFDMA_CORE_MAJOR
#define STFDMA_CORE_MAJOR   0
#endif

/*** MODULE INFO *************************************************************/

MODULE_AUTHOR("WALKERM");
MODULE_DESCRIPTION("");
MODULE_SUPPORTED_DEVICE("");
MODULE_LICENSE("ST Microelectronics");

/*** PROTOTYPES **************************************************************/

static int  fdma_init_module(void);
static void fdma_cleanup_module(void);

#include "stfdma_core_fops.h"

#if defined (ST_OSLINUX)
#if defined (CONFIG_STM_DMA)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,11)
#define NUM_CHANNELS 16
#else
/* Private Linux DMAC driver functions ------------------------------------*/
static int setup_freerunning_nodelist(struct dma_channel *chan, unsigned long flags);
static int setup_sg_nodelist(struct dma_channel *chan,unsigned long flags);

/* Public Linux DMAC driver functions -------------------------------------*/
static int linux_wrapper_fdma_request(struct dma_channel *chan);
static int linux_wrapper_fdma_free(struct dma_channel *chan);
static int linux_wrapper_fdma_configure(struct dma_channel *channel,unsigned long flags);
static int linux_wrapper_fdma_get_residue(struct dma_channel *chan);
static int linux_wrapper_fdma_xfer(struct dma_channel *chan);
static int linux_wrapper_fdma_pause(int flush,struct dma_channel * chan);
static void linux_wrapper_fdma_unpause(struct dma_channel * chan);
static int linux_wrapper_fdma_stop(struct dma_channel *cha);

/* Extern Linux functions --------------------------------------------------*/
extern void Hook_7100_fdma2(int *);
extern void unregister_fdma(void);

/* Node pool allocation  */

static char* g_NodePoolName = "FDMA_LinuxNodePool";
struct dma_pool* g_NodePool;

#define NUM_CHANNELS 16

STFDMA_Node_t* Node_p[NUM_CHANNELS];
dma_addr_t physical_address[NUM_CHANNELS];
static STFDMA_TransferId_t TransferId[NUM_CHANNELS] ;

typedef int (*fp)(struct dma_channel *,unsigned long);

typedef struct channel_status{
	u32    locked;
	u32    *llu_virt_addr;
	u32	llu_bus_addr;
	u32	transfer_sz;
	u32	alloc_mem_sz;/*number of bytes of dynamic mem alloced for llu*/
	u32 	list_len;
	char	dynamic_mem;
	char	ch_term;
	char	ch_pause;
	char	is_xferring;

/*This must be the last structure member to prevent scribbling
 * over the initialied queue * at the end of transfer*/
/* This is done by the memset in free_fdma_channel() */
	wait_queue_head_t cfg_blk;
}channel_status;

typedef struct fdma_chip{
	channel_status channel[NUM_CHANNELS];
	spinlock_t fdma_lock;
	spinlock_t channel_lock;
}fdma_chip;

static fdma_chip chip;

#endif
#endif
#endif

/* Output appears in /var/log/syslog */
int  printk(char const *format, ...);

/*** MODULE PARAMETERS *******************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
unsigned int     irq_number   = 4;
MODULE_PARM      (irq_number,   "i");
MODULE_PARM_DESC (irq_number,   "Interrupt request line");
#endif

/*** GLOBAL VARIABLES *********************************************************/

#if defined (ST_7100) || defined (ST_7109)
unsigned int    *BaseAddress_p;
#elif defined (ST_7200)
unsigned int    *BaseAddress0_p, *BaseAddress1_p;
#else
#error Unknown silicon
#endif

#if !defined (TEST_HARNESS)
#if defined (ST_7100) || defined (ST_7109)
static char     *FDMADeviceName = "STFDMA";
#elif defined (ST_7200)
static char     *FDMA0DeviceName = "STFDMA0", *FDMA1DeviceName = "STFDMA1";
#else
#error Unknown silicon
#endif
#endif

#if defined (CONFIG_STM_DMA)
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)
int Lock_7100_fdma2[NUM_CHANNELS];
#endif
unsigned int STFDMA_MaxChannels;
unsigned int STFDMA_Mask;
#endif

/*** EXPORTED SYMBOLS ********************************************************/

#define EXPORT_SYMTAB
#if defined (EXPORT_SYMTAB)
EXPORT_SYMBOL(STFDMA_StartGenericTransfer);
EXPORT_SYMBOL(STFDMA_StartTransfer);
EXPORT_SYMBOL(STFDMA_ResumeTransfer);
EXPORT_SYMBOL(STFDMA_FlushTransfer);
EXPORT_SYMBOL(STFDMA_AbortTransfer);
EXPORT_SYMBOL(STFDMA_AllocateContext);
EXPORT_SYMBOL(STFDMA_DeallocateContext);
EXPORT_SYMBOL(STFDMA_ContextGetSCList);
EXPORT_SYMBOL(STFDMA_ContextSetSCList);
EXPORT_SYMBOL(STFDMA_ContextGetSCState);
EXPORT_SYMBOL(STFDMA_ContextSetSCState);
EXPORT_SYMBOL(STFDMA_ContextSetESBuffer);
EXPORT_SYMBOL(STFDMA_ContextSetESReadPtr);
EXPORT_SYMBOL(STFDMA_ContextSetESWritePtr);
EXPORT_SYMBOL(STFDMA_ContextGetESReadPtr);
EXPORT_SYMBOL(STFDMA_ContextGetESWritePtr);
EXPORT_SYMBOL(STFDMA_GetTransferStatus);
EXPORT_SYMBOL(STFDMA_LockChannelInPool);
EXPORT_SYMBOL(STFDMA_LockChannel);
EXPORT_SYMBOL(STFDMA_SetAddDataRegionParameter);
#if defined (CONFIG_STM_DMA)
EXPORT_SYMBOL(STFDMA_LockFdmaChannel);
#endif
EXPORT_SYMBOL(STFDMA_UnlockChannel);
EXPORT_SYMBOL(STFDMA_SetTransferCount);
EXPORT_SYMBOL(STFDMA_GetRevision);
EXPORT_SYMBOL(STFDMA_Init);
EXPORT_SYMBOL(STFDMA_Term);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)
#if defined (CONFIG_STM_DMA)
EXPORT_SYMBOL(Lock_7100_fdma2);
#endif
#endif
#endif

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

#define MAX_DEVICES    4

#if defined (ST_7100) || defined (ST_7109)
#define FDMA_PHYSICAL_ADDRESS       0x19220000
#define FDMA_REGISTER_REGION_SIZE   0xBFFF
#define FDMA_INTERRUPT              140
#elif defined (ST_7200)
#define FDMA0_PHYSICAL_ADDRESS      0xFD810000
#define FDMA1_PHYSICAL_ADDRESS      0xFD820000
#define FDMA_REGISTER_REGION_SIZE   0x10000
#define FDMA0_INTERRUPT             33
#define FDMA1_INTERRUPT             35
#else
#error FDMA_PHYSICAL_ADDRESS undefined
#endif

/*** LOCAL VARIABLES *********************************************************/
static unsigned int major = STFDMA_CORE_MAJOR;

struct semaphore fdma_lock; /* Global lock for the module */

static struct cdev         *fdma_cdev  = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
static struct class_simple *fdma_class = NULL;
#endif

static struct file_operations fdma_fops =
{
    open    : fdma_open,
    read    : fdma_read,
    write   : fdma_write,
    ioctl   : fdma_ioctl,
 /* flush   : fdma_flush,    */
 /* lseek   : fdma_lseek,    */
 /* readdir : fdma_readdir,  */
    poll    : fdma_poll,
 /* mmap    : fdma_mmap,     */
 /* fsync   : fdma_fsync,    */
    release : fdma_release,     /* close */
};

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)
#if defined (CONFIG_STM_DMA)
static struct dma_ops linux_wrapper_fdma_ops =
{
        .request                = linux_wrapper_fdma_request,
        .free                   = linux_wrapper_fdma_free,
        .get_residue    	    = linux_wrapper_fdma_get_residue,
        .xfer                   = linux_wrapper_fdma_xfer,
        .configure              = linux_wrapper_fdma_configure,
        .pause                  = linux_wrapper_fdma_pause,
        .unpause                = linux_wrapper_fdma_unpause,
        .stop                   = linux_wrapper_fdma_stop,
};

static struct dma_info linux_wrapper_fdma_info =
{
        .name                   = "LINUX WRAPPER",
        .nr_channels    	    = NUM_CHANNELS,
        .ops                    = &linux_wrapper_fdma_ops,
        .flags                  = DMAC_CHANNELS_TEI_CAPABLE,
};
#endif
#endif

/*** METHODS ****************************************************************/

/*=============================================================================

   fdma_init_module

   Initialise the module (initialise the access routines (fops) and set the
   major number. Aquire any resources needed and setup internal structures.

  ===========================================================================*/
static int __init fdma_init_module(void)
{
    /*** Initialise the module ***/

    int     err                     = 0;  /* No error */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    int     entries                 = 0;
#endif

#if defined (ST_7100) || defined (ST_7109)
    struct  resource *fdma_resource = NULL;
#elif defined (ST_7200)
    struct  resource *fdma0_resource = NULL, *fdma1_resource = NULL;
#else
#error Unknown silicon
#endif

#if defined (ST_OSLINUX)
#if defined (CONFIG_STM_DMA)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    static struct dma_info *info = &linux_wrapper_fdma_info;
    int i;
#endif
#endif
#endif
#if !defined (TEST_HARNESS)
    STFDMA_InitParams_t FDMAInitParams;
#endif

    dev_t base_dev_no;

    SET_MODULE_OWNER(&fdma_fops);

    /*** Set up the modules global lock ***/

    sema_init(&(fdma_lock), 1);

    /*
     * Register the major number. If major = 0 then a major number is auto
     * allocated. The allocated number is returned.
     * The major number can be seen in user space in '/proc/devices'
     */

    if (major == 0)
    {
        if (alloc_chrdev_region(&base_dev_no, 0, MAX_DEVICES, "fdma"))
        {
            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for fdma by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }

        major = MAJOR(base_dev_no);
    }
    else
    {
    	base_dev_no = MKDEV(major, 0);

        if (register_chrdev_region(base_dev_no, MAX_DEVICES, "fdma"))
        {
            err = -EBUSY;
            printk(KERN_ALERT "No major numbers for fdma by %s (pid %i)\n", current->comm, current->pid);
            goto fail;
        }
    }

    if (NULL == (fdma_cdev = cdev_alloc()))
    {
         err = -EBUSY;
         printk(KERN_ALERT "No major numbers for fdma by %s (pid %i)\n", current->comm, current->pid);
         goto fail_reg;
    }

    fdma_cdev->owner = THIS_MODULE;
    fdma_cdev->ops   = &fdma_fops;
    kobject_set_name(&(fdma_cdev->kobj), "fdma%d", 0);

    /*** Register any register regions ***/

    /*** Do the work ***/

    /* Appears in /var/log/syslog */
    printk(KERN_ALERT "Load module fdma [%d] by %s (pid %i)\n", major, current->comm, current->pid);

    /*** Register the device nodes for this module ***/

    /* Add the char device structure for this module */

    if (cdev_add(fdma_cdev, base_dev_no, MAX_DEVICES))
    {
        err = -EBUSY;
        printk(KERN_ALERT "Failed to add a charactor device for fdma by %s (pid %i)\n", current->comm, current->pid);
        goto fail_remap;
    }

    /* Create a class for this module */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    fdma_class = class_simple_create(THIS_MODULE, "fdma");

    if (IS_ERR(fdma_class))
    {
        printk(KERN_ERR "Error creating fdma class.\n");
        err = -EBUSY;
        goto fail_final;
    }

    /* Add entries into /sys for each minor number we use */

    for (; (entries < MAX_DEVICES); entries++)
    {
	    struct class_device *class_err;

        class_err = class_simple_device_add(fdma_class, MKDEV(major, entries), NULL, "fdma%d", entries);

    	if (IS_ERR(class_err))
    	{
            printk(KERN_ERR "Error creating fdma device %d.\n", entries);
            err = PTR_ERR(class_err);
            goto fail_dev;
        }
    }
#endif

    /* Reserve the fdma memory region */

#if defined (ST_7100) || defined (ST_7109)
    fdma_resource = request_region(FDMA_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE, "fdma");
    if (NULL == fdma_resource)
    {
        printk(KERN_ERR "Failed to lock fdma register region.\n");
        goto fail_dev;
    }

    BaseAddress_p = (unsigned int *)ioremap(FDMA_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE);
    if (NULL == BaseAddress_p)
    {
        printk(KERN_ERR "Failed to remap fdma io region.\n");
        goto fail_dev;
    }
#elif defined (ST_7200)
    fdma0_resource = request_region(FDMA0_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE, "fdma0");
    if (NULL == fdma0_resource)
    {
        printk(KERN_ERR "Failed to lock fdma0 register region.\n");
        goto fail_dev;
    }

    BaseAddress0_p = (unsigned int *)ioremap(FDMA0_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE);
    if (NULL == BaseAddress0_p)
    {
        printk(KERN_ERR "Failed to remap fdma0 io region.\n");
        goto fail_dev;
    }

    fdma1_resource = request_region(FDMA1_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE, "fdma1");
    if (NULL == fdma1_resource)
    {
        printk(KERN_ERR "Failed to lock fdma1 register region.\n");
        goto fail_dev;
    }

    BaseAddress1_p = (unsigned int *)ioremap(FDMA1_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE);
    if (NULL == BaseAddress1_p)
    {
        printk(KERN_ERR "Failed to remap fdma1 io region.\n");
        goto fail_dev;
    }
#else
#error Unknown silicon
#endif

#if !defined (TEST_HARNESS)
#if defined (ST_7100) || defined (ST_7109)
    FDMAInitParams.FDMABlock  = STFDMA_1;
    FDMAInitParams.DeviceType = STFDMA_DEVICE_FDMA_2;
    FDMAInitParams.DriverPartition_p = (void*)0x80000000;   /* Unused */
    FDMAInitParams.NCachePartition_p = (void*)0x80000000;   /* Unused */
    FDMAInitParams.InterruptNumber = FDMA_INTERRUPT;
    FDMAInitParams.InterruptLevel = 14;                     /* Unused */
    FDMAInitParams.NumberCallbackTasks = 1;
    FDMAInitParams.BaseAddress_p = (void*)BaseAddress_p;
    FDMAInitParams.ClockTicksPerSecond = HZ;

    if (0 != STFDMA_Init(FDMADeviceName, &FDMAInitParams))
    {
    	printk("STFDMA_Init Failed \n");
        return -ERESTARTSYS;
    }
#elif defined (ST_7200)
    FDMAInitParams.FDMABlock  = STFDMA_1;
    FDMAInitParams.DeviceType = STFDMA_DEVICE_FDMA_2;
    FDMAInitParams.DriverPartition_p = (void*)0x80000000;   /* Unused */
    FDMAInitParams.NCachePartition_p = (void*)0x80000000;   /* Unused */
    FDMAInitParams.InterruptNumber = FDMA0_INTERRUPT;
    FDMAInitParams.InterruptLevel = 14;                     /* Unused */
    FDMAInitParams.NumberCallbackTasks = 1;
    FDMAInitParams.BaseAddress_p = (void*)BaseAddress0_p;
    FDMAInitParams.ClockTicksPerSecond = HZ;

    if (0 != STFDMA_Init(FDMA0DeviceName, &FDMAInitParams))
    {
    	printk("STFDMA_Init Failed \n");
        return -ERESTARTSYS;
    }

    FDMAInitParams.FDMABlock  = STFDMA_2;
    FDMAInitParams.DeviceType = STFDMA_DEVICE_FDMA_2;
    FDMAInitParams.DriverPartition_p = (void*)0x80000000;   /* Unused */
    FDMAInitParams.NCachePartition_p = (void*)0x80000000;   /* Unused */
    FDMAInitParams.InterruptNumber = FDMA1_INTERRUPT;
    FDMAInitParams.InterruptLevel = 14;                     /* Unused */
    FDMAInitParams.NumberCallbackTasks = 1;
    FDMAInitParams.BaseAddress_p = (void*)BaseAddress1_p;
    FDMAInitParams.ClockTicksPerSecond = HZ;

    if (0 != STFDMA_Init(FDMA1DeviceName, &FDMAInitParams))
    {
    	printk("STFDMA_Init Failed \n");
        return -ERESTARTSYS;
    }
#else
#error Unknown silicon
#endif

#endif

/*    printk(KERN_ALERT "BaseAddress_p = 0x%08X\n", (unsigned int)BaseAddress_p);*/

#if defined (ST_OSLINUX)
#if defined (CONFIG_STM_DMA)
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)
    {
    	/* Memory pool allocation for FDMA nodes */

    	g_NodePool = dma_pool_create(g_NodePoolName,
                                 	NULL,
                                	sizeof(STFDMA_GenericNode_t),
                                	32,
                                	PAGE_SIZE);

    	if (NULL == g_NodePool)
    	{
        	printk(KERN_ALERT "Failed to create node pool\n");
        	//ErrorCode = ST_ERROR_NO_MEMORY;
        	goto fail_dev;
    	}

    	/* Allocate the nodes for DMA */
    	for (i = 0;i < NUM_CHANNELS; i ++)
    	{
    	    Node_p[i] = NULL;
    	    /* Allocate a node for DMA */
    	    Node_p[i] = (STFDMA_Node_t *)dma_pool_alloc(g_NodePool, GFP_KERNEL, &physical_address[i]);

    	    if (Node_p[i] == NULL)
    	    {
        		printk("*** ERROR: Node creation: Not enough memory\n");
        		goto fail_dev;
    	    }
    	}

    	spin_lock_init(&chip.channel_lock);
    	spin_lock_init(&chip.fdma_lock);
	    memset((u32*)&chip.channel[0],0,sizeof(channel_status)* NUM_CHANNELS);

    	register_dmac(info);

    	printk("STFDMA calling  7100_fdma Hook \n");

    	/* Hook to the 7100_fdma2 to get the current driver configuration  */

    	Hook_7100_fdma2(&Lock_7100_fdma2[0]);

#if !defined (TEST_HARNESS)
    	err = STFDMA_LockFdmaChannel(&Lock_7100_fdma2[0], STFDMA_1);
#endif
    }
#else
    err = STFDMA_LockFdmaChannel(STFDMA_1);
#endif
#endif
#endif

    return (0);  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

fail_dev:

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    /* Remove any /sys entries we have added so far */
    for (entries--; (entries >= 0); entries--)
    {
        class_simple_device_remove(MKDEV(major, entries));
    }

    /* Remove the device class entry */
    class_simple_destroy(fdma_class);

fail_final:
#endif

    /* Remove the char device structure (has been added) */
    cdev_del(fdma_cdev);
    goto fail_reg;

fail_remap:

    /* Remove the char device structure (has not been added) */
    kobject_put(&(fdma_cdev->kobj));

fail_reg:

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

fail:

    return (err);
}

/*=============================================================================

   fdma_cleanup_module

   Realease any resources allocaed to this module before the module is
   unloaded.

  ===========================================================================*/
static void __exit fdma_cleanup_module(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    int i;
#endif

#if !defined (TEST_HARNESS)
#if defined (ST_7100) || defined (ST_7109)
    STFDMA_Term(FDMADeviceName, TRUE, STFDMA_1);
#elif defined (ST_7200)
    STFDMA_Term(FDMA0DeviceName, TRUE, STFDMA_1);
    STFDMA_Term(FDMA1DeviceName, TRUE, STFDMA_2);
#else
#error Unknown silicon
#endif
#endif

#if defined (ST_7100) || defined (ST_7109)
    iounmap(BaseAddress_p);
    release_region(FDMA_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE);
#elif defined (ST_7200)
    iounmap(BaseAddress0_p);
    release_region(FDMA0_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE);
    iounmap(BaseAddress1_p);
    release_region(FDMA1_PHYSICAL_ADDRESS, FDMA_REGISTER_REGION_SIZE);
#else
#error Unknown silicon
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
    /* Release the /sys entries for this module */

    /* Remove any devices */
    for (i = 0; (i < MAX_DEVICES); i++)
    {
        class_simple_device_remove(MKDEV(major, i));
    }

    /* Remove the device class entry */
    class_simple_destroy(fdma_class);
#endif

    /* Remove the char device structure (has been added) */
    cdev_del(fdma_cdev);

    /* Unregister the module */
    unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);

    /* Free any module resources */
    fdma_CleanUp();

#if defined (ST_OSLINUX)
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)
#if defined (CONFIG_STM_DMA)
    /* Memory pool de-allocation for FDMA nodes */
    /* Free the nodes for DMA */
    for (i = 0; i < NUM_CHANNELS; i ++)
    {
    	if (Node_p[i] != NULL)
    	{
    	    dma_pool_free(g_NodePool, Node_p[i], physical_address[i]);
    	    Node_p[i] = NULL;
    	    physical_address[i] = (dma_addr_t)NULL;
    	}
    }
    if (NULL != g_NodePool)
    {
    	dma_pool_destroy(g_NodePool);
	    g_NodePool = NULL;
    }
#endif
#endif
#endif

    printk(KERN_ALERT "Unload module fdma by %s (pid %i)\n", current->comm, current->pid);
}

#if defined (ST_OSLINUX)
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)
#if defined (CONFIG_STM_DMA)
/****************************************************************************
 Application Interface for standard linux drivers using Stapi FDMA
 These functions mimick the behaviour of a standard Linux DMAC driver :
 	- linux_wrapper_fdma_request
 	- linux_wrapper_fdma_free
 	- linux_wrapper_fdma_configure
 	- linux_wrapper_fdma_get_residue
 	- linux_wrapper_fdma_xfer
 	- linux_wrapper_fdma_stop
 	- linux_wrapper_fdma_pause
 	- linux_wrapper_fdma_unpause
 	- linux_wrapper_fdma_wait_for_completion

 The STAPI FDMA driver wrapper allows the dma management for 2 modes :
 	freerunning mode (standard mode : using channel 5 to 15)
 	scatter/gather mode
 The specific channels (0 to 4) cannot be used by the wrapper. It always uses the
 STFDMA_DEFAULT_POOL
****************************************************************************/
static int linux_wrapper_fdma_request(struct dma_channel *chan)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	STFDMA_ChannelId_t ChannelId;

 	ErrorCode = STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL, &ChannelId, STFDMA_1);

	if (ErrorCode != ST_NO_ERROR)
	{
		return -ENODEV;
	}

	/* Set the flag to indicate the channel is requested by a Standard Linux driver via
	  the Generic DMA Linux driver */
	ErrorCode = STFDMA_SetSTLinuxChannel(ChannelId, STFDMA_1);

	/* printk("linux_wrapper_fdma_request in Default POOL : allocated channel = %d\n",ChannelId); */
	/* Save the local ChannelId in the channel structure. this information is not used by the dma-api.
	   It will be used only by the wrapper. The ChannelId is also returned to the requesting driver
	   (for dma channel) */
	chan->chan = ChannelId;

	return (int)ChannelId;
}

static int linux_wrapper_fdma_free(struct dma_channel *chan)
{
	STFDMA_ChannelId_t ChannelId = chan->chan;
	unsigned long flags;

	/* printk("linux_wrapper_fdma_free : channel = %d\n",chan->chan); */
	STFDMA_UnlockChannel(ChannelId, STFDMA_1);

	spin_lock_irqsave(&chip.channel_lock,flags);
	if(chip.channel[ChannelId].dynamic_mem)
	{
		bigphysarea_free((void*)chip.channel[ChannelId].llu_virt_addr,
	    chip.channel[ChannelId].alloc_mem_sz);
	}

	memset(&chip.channel[ChannelId].llu_virt_addr,0,sizeof(channel_status) - sizeof(u32) - sizeof(wait_queue_head_t));

	spin_unlock_irqrestore(&chip.channel_lock,flags);

	return 0;
}

static int linux_wrapper_fdma_configure(struct dma_channel *chan, unsigned long flags)
{
	STFDMA_TransferGenericParams_t Params;
	int Status;

    unsigned long start_addr=0;
    unsigned long node_sz_bytes =0;
    fp nodelist_configure=0;
    int list_len=0;
    int ret=0;
	unsigned long iflags;

	/*printk("linux_wrapper_fdma_configure In IRQ : channel = %d, flags = 0x%x \n",chan->chan, flags);*/

#ifndef STFDMA_NO_PARAMETER_CHECK
	if(chan->chan <0 || chan->chan >15)
		return -ENODEV;
#endif

	switch(chan->mode)
	{
		default:
			printk("FDMA no mode specified -Using Freerunning\n");

		case MODE_FREERUNNING:
			if(flags & DIM_0_x_SG )
			{
				list_len = chan->dst_sg_len;
				nodelist_configure = setup_sg_nodelist;
			}
			else if(flags & DIM_SG_x_0)
			{
				list_len = chan->src_sg_len;
				nodelist_configure = setup_sg_nodelist;
			}
			else if (flags & DIM_SG_x_SG)
			{}
			else
			{
				list_len = chan->list_len;
				nodelist_configure =  setup_freerunning_nodelist;
            }
			node_sz_bytes= sizeof(STFDMA_Node_t);
			break;
    }

    spin_lock_irqsave(&chip.channel_lock,iflags);

	if(chip.channel[chan->chan].is_xferring)
	{

		spin_unlock_irqrestore(&chip.channel_lock,iflags);
		printk("FDMA_CH%d busy\n",chan->chan);
		return -EBUSY;
	}
	/*we are reconfiguring a channel with alloced mem,
	 * but we haven't started an xfer so it is ok to re-config*/
	else if(chip.channel[chan->chan].dynamic_mem)
	{
		/*printk(KERN_CRIT"FDMA_CH%d reconfiguring configured channel\n", chan->chan); */
		/* Do as in free_fdma_channel */
		if(chip.channel[chan->chan].dynamic_mem)
		{
		    bigphysarea_free((void*)chip.channel[chan->chan].llu_virt_addr,
			chip.channel[chan->chan].alloc_mem_sz);
		}
		memset(&chip.channel[chan->chan].llu_virt_addr,0,sizeof(channel_status) - sizeof(u32) - sizeof(wait_queue_head_t));
	}

	if(list_len >1 )
	{ /*we are doing SG so we must alloc some mem for the llu*/
		chip.channel[chan->chan].dynamic_mem =1;
        start_addr = (unsigned long)
        bigphysarea_alloc(node_sz_bytes*list_len);
    }
    else
    {
        start_addr = (unsigned long)Node_p[chan->chan];
	}

	if(!start_addr)
	{
		spin_unlock_irqrestore(&chip.channel_lock,iflags);
		return -ENOMEM;
	}

	chip.channel[chan->chan].alloc_mem_sz = node_sz_bytes * list_len;
	chip.channel[chan->chan].list_len = chan->list_len;
	chip.channel[chan->chan].llu_virt_addr = (u32*)start_addr;
	chip.channel[chan->chan].llu_bus_addr  = (u32) ioremap(start_addr,32);

	spin_unlock_irqrestore(&chip.channel_lock,iflags);

	/* Call the specific configurator function */
	ret = nodelist_configure(chan,flags);
	chan->flags = DMA_CONFIGURED;

	/* Flush cache for Node structure */
	dma_cache_wback(chip.channel[chan->chan].llu_virt_addr,
			chip.channel[chan->chan].alloc_mem_sz);

	/* Build and fill the Params structure */
	Params.ChannelId            = chan->chan;
	Params.Pool                 = STFDMA_DEFAULT_POOL;
	if(list_len >1 )
		/* Be carefull, __pa() cleans only bit31. In our case (non-cached memory),
		   bit 29 is still set.*/
		Params.NodeAddress_p    = (STFDMA_GenericNode_t*)(__pa(chip.channel[chan->chan].llu_virt_addr));
	else
		Params.NodeAddress_p 	= (STFDMA_GenericNode_t*)physical_address[chan->chan];
	Params.BlockingTimeout      = 0;
	Params.CallbackFunction     = (STFDMA_CallbackFunction_t)chan->comp_callback;
	Params.ApplicationData_p    = chan->comp_callback_param;
	Params.FDMABlock = STFDMA_1;

	/* Start the DMA by calling the STAPI STFDMA function */
	TransferId[chan->chan] = 0;
	Status=STFDMA_StartGenericTransfer(&Params, &TransferId[chan->chan]);

	return Status;
}

static int linux_wrapper_fdma_get_residue(struct dma_channel *chan)
{
	ST_ErrorCode_t			ErrorCode;
	STFDMA_TransferStatus_t 	TransferStatus;

	ErrorCode = STFDMA_GetTransferStatus(TransferId[chan->chan], &TransferStatus);

	if (TransferStatus.NodeBytesRemaining)
        	printk("NodeBytesRemaining = %d \n",TransferStatus.NodeBytesRemaining);

    return (TransferStatus.NodeBytesRemaining) ;
}

static int linux_wrapper_fdma_xfer(struct dma_channel *chan)
{

    /* Return immediatly without any action.
       The standart Linux XFER feature is to start the previously configured channel.
       With STAPI STFDMA, the channel is launched by the STFDMA_StartGenericTransfer() function */

    return 0;

}

static int linux_wrapper_fdma_pause(int flush,struct dma_channel * chan)
{
    panic ("linux_wrapper_fdma_pause : channel = %d . Non implemented \n",chan->chan);

	return 0;
}

static void linux_wrapper_fdma_unpause(struct dma_channel * chan)
{
    if(chan->chan < 0 || chan->chan >15)
        return;

    /* if(get_fdma_chan_status(chan->chan) == FDMA_CHANNEL_PAUSED){ } }*/
	panic ("linux_wrapper_fdma_unpause : channel = %d . Non implemented \n",chan->chan);

	return;
}

static int linux_wrapper_fdma_stop(struct dma_channel *chan)
{
    panic ("linux_wrapper_fdma_stop : channel = %d . Non implemented \n",chan->chan);

	return 0;
}



/***************************************************************
Routines imported from 7100_fdma drivers to configure the nodes
****************************************************************/

static void setup_node_strides(STFDMA_Node_t * node,
				struct dma_channel * chan,
				unsigned long flags)
{
	/*a flag value below the 1F range indicates a paced transfer is configured
	 * so no notion of dimensionality is applied.  Therefore line length == total bytes*/
	if(flags & 0x1F){
                node->SourceStride       =0;
                node->DestinationStride  =0;
                node->Length             = chan->count;
        }
        else {
            if(flags & DIM_0_x_0){
                node->SourceStride       = 0;
                node->DestinationStride  = 0;
                node->Length             = chan->src_sz;
            }
            else if(flags & DIM_0_x_1){
                node->SourceStride       = 0;
                node->DestinationStride  = chan->src_sz;
                node->Length             = chan->src_sz;
            }
            else if(flags & DIM_0_x_2){
                node->SourceStride       = 0;
                node->DestinationStride  = chan->dstride;
                node->Length             = chan->src_sz;
            }
            else if(flags & DIM_1_x_0){
                node->SourceStride       = chan->src_sz;
                node->DestinationStride  = 0;
                node->Length             = chan->src_sz;
            }
            else if(flags & DIM_1_x_1){
                node->SourceStride       = 0;
                node->DestinationStride  = 0;
                node->Length             = chan->src_sz;
            }
            else if(flags & DIM_1_x_2){
                node->SourceStride      = chan->dst_sz;
                node->DestinationStride = chan->dstride;
                node->Length            = chan->dst_sz;
            }
            else if(flags & DIM_2_x_0){
                node->SourceStride      = chan->sstride;
                node->DestinationStride = 0;
                node->Length            = chan->src_sz;
            }
            else if(flags & DIM_2_x_1){
                node->SourceStride       = chan->sstride;
                node->DestinationStride  = chan->src_sz;
                node->Length             = chan->src_sz;
            }
            else if(flags & DIM_2_x_2){
                node->SourceStride       = chan->sstride;
                node->DestinationStride  = chan->dstride;
                node->Length             = chan->src_sz;
            }
            else{
                printk(" NO stride setup specified-using def config\n");
                node->SourceStride       = 0;
                node->DestinationStride  = 0;
                node->Length             = chan->src_sz;
               /*we take the default config -simple mem -> mem move 1x1 */
	    }
	}
}


int setup_freerunning_nodelist(struct dma_channel *chan,unsigned long flags)
{
	int i=0;
	unsigned long iflags;
	STFDMA_Node_t llu[chan->list_len];
	unsigned long current_addr=chip.channel[chan->chan].llu_bus_addr;
	struct dma_channel * channel = chan;

	memset(&llu[0],0,sizeof(STFDMA_Node_t)* chan->list_len);
	while(i++ < channel->list_len){

		current_addr +=sizeof(STFDMA_Node_t);
		/*printk("current_addr = 0x%x  i = %d \n",current_addr,i); */

		llu[i-1].Next_p 		        = (STFDMA_Node_t *)virt_to_phys((u32 *)current_addr);
		llu[i-1].SourceAddress_p 	    = (void *)virt_to_phys((u32 *)chan->sar);
		llu[i-1].DestinationAddress_p 	= (void *)virt_to_phys((u32 *)chan->dar);
		llu[i-1].NumberBytes		    = chan->count;

		/* Channel Control register */
		llu[i-1].NodeControl.PaceSignal	= STFDMA_REQUEST_SIGNAL_NONE;
		if(flags & NODE_PAUSE_ISR)
			llu[i-1].NodeControl.NodeCompletePause = TRUE;
		if(flags & NODE_DONE_ISR)
			llu[i-1].NodeControl.NodeCompleteNotify = TRUE;

		/*FREERUNNING TRANSFER SELECTED*/
		/*due to a wierdness in the FDMA design,
		freerunning xfers must alwyas set increment
		addr for source/dest. This is an overloaded
		parameter for the slim core and will not actually
		produce incrementing addresses !! :-\ */
		llu[i-1].NodeControl.SourceDirection = STFDMA_DIRECTION_INCREMENTING;
		llu[i-1].NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
		llu[i-1].NodeControl.Reserved = 0;

		setup_node_strides(&llu[i-1],chan,flags);
		chip.channel[chan->chan].transfer_sz += chan->count;
		chan++;
	}

	if(flags & LIST_TYPE_LINKED)
		llu[channel->list_len-1].Next_p = (STFDMA_Node_t *)virt_to_phys((u32*)chip.channel[channel->chan].llu_bus_addr);
	else
		llu[channel->list_len-1].Next_p = (U32)0;


#if 0
	printk("NEXT    %08x\n", llu[0].Next_p);
	printk("CONTROL %08x\n", llu[0].NodeControl);
	printk("NBYTES  %08x\n", llu[0].NumberBytes);
	printk("SADDR   %08x\n", llu[0].SourceAddress_p);
	printk("DADDR   %08x\n", llu[0].DestinationAddress_p);
	printk("LENGTH  %08x\n", llu[0].Length);
	printk("SSTRIDE %08x\n", llu[0].SourceStride);
	printk("DSTRIDE %08x\n", llu[0].DestinationStride);


	printk("memcpy : addr = 0x%x , 0x%x, size = %d, channel = %d\n",
		(u32)chip.channel[channel->chan].llu_bus_addr,&llu[0],
		sizeof(STFDMA_Node_t)*channel->list_len, channel->chan);
#endif

	spin_lock_irqsave(&chip.channel_lock,iflags);
	memcpy_toio((u32)chip.channel[channel->chan].llu_bus_addr, &llu[0],
			sizeof(STFDMA_Node_t) * channel->list_len);
	spin_unlock_irqrestore(&chip.channel_lock,iflags);

	return 0;
}


static int setup_sg_nodelist(struct dma_channel *chan,unsigned long flags)
{
	int i=0;
	unsigned long iflags;
	unsigned long current_addr=chip.channel[chan->chan].llu_bus_addr;
	struct dma_channel * channel = chan;
	struct scatterlist * src_sg = chan->src_sg;
	struct scatterlist * dst_sg = chan->dst_sg;
	static u32 llu_len=0;

	STFDMA_Node_t llu[ flags & DIM_SG_x_0 ?
				chan->src_sg_len:
				chan->dst_sg_len  ];

	if(flags & DIM_SG_x_0)
		llu_len = chan->src_sg_len;
	else if (flags & DIM_0_x_SG)
		llu_len = chan->dst_sg_len;
	else if (flags & DIM_SG_x_SG) {}

	memset(&llu[0],0,sizeof(STFDMA_Node_t)* llu_len);

	while(i++ < llu_len){
		current_addr +=sizeof(STFDMA_Node_t);

		llu[i-1].Next_p = (STFDMA_Node_t *)virt_to_phys((u32*)current_addr);
		if(flags & DIM_0_x_SG){
			if((!dst_sg->dma_address) || (!dst_sg->length))
				return -EINVAL;

			llu[i-1].SourceStride  	        =0;
			llu[i-1].DestinationStride 	    =channel->src_sz;
			llu[i-1].Length 	            =channel->src_sz;
			/*this is essentially a 0x1 xfer */
			llu[i-1].SourceAddress_p        =(void *)virt_to_phys((u32 *)channel->sar);
			llu[i-1].DestinationAddress_p   =(void *)dst_sg->dma_address;
			llu[i-1].NumberBytes	        =dst_sg->length;
			chip.channel[chan->chan].list_len = dst_sg->length;
			dst_sg++;

		}
		else if(flags & DIM_SG_x_0){
			/** 1x0*/
			if((!src_sg->dma_address) || (! src_sg->length))
				return -EINVAL;

			llu[i-1].SourceStride 	        =channel->src_sz;
			llu[i-1].DestinationStride	    =0;
			llu[i-1].Length 	            =channel->src_sz;
			llu[i-1].DestinationAddress_p   =(void *)virt_to_phys((u32 *)channel->dar);
			llu[i-1].SourceAddress_p        =(void *)src_sg->dma_address;
			llu[i-1].NumberBytes	        =src_sg->length;
			chip.channel[chan->chan].list_len = dst_sg->length;
			src_sg++;
		}
		else{
			return -EINVAL;
			printk(" bad ip flags specified\n");
		}

		/* Channel Control register */
		llu[i-1].NodeControl.PaceSignal	= STFDMA_REQUEST_SIGNAL_NONE;
		if(flags & NODE_PAUSE_ISR)
			llu[i-1].NodeControl.NodeCompletePause = TRUE;
		if(flags  & NODE_DONE_ISR)
			llu[i-1].NodeControl.NodeCompleteNotify = TRUE;

		/*FREERUNNING TRANSFER SELECTED*/
		/*due to a wierdness in the FDMA design,
		freerunning xfers must alwyas set increment
		addr for source/dest. This is an overloaded
		parameter for the slim core and will not actually
		produce incrementing addresses !! :-\ */
		llu[i-1].NodeControl.SourceDirection = STFDMA_DIRECTION_INCREMENTING;
		llu[i-1].NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
		llu[i-1].NodeControl.Reserved = 0;

	}
	if(flags & LIST_TYPE_LINKED)
		llu[llu_len-1].Next_p =(STFDMA_Node_t *)virt_to_phys((u32 *)chip.channel[channel->chan].llu_bus_addr);
	else
		llu[llu_len-1].Next_p =NULL;

	spin_lock_irqsave(&chip.channel_lock,iflags);
	memcpy_toio((u32)chip.channel[channel->chan].llu_bus_addr, &llu[0],
			sizeof(STFDMA_Node_t)*llu_len);
	spin_unlock_irqrestore(&chip.channel_lock,iflags);

	return 0;
}
#endif /* CONFIG_STM_DMA */
#endif /* LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11) */
#endif /* ST_OSLINUX */

/*** MODULE LOADING ******************************************************/


/* Tell the module system these are our entry points. */

module_init(fdma_init_module);
module_exit(fdma_cleanup_module);

