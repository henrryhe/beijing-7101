/*****************************************************************************

File name   : klxload.c

Description : linux lxload device driver

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/

/* #define LXLOAD_DEBUG */

#if defined(ST_OSLINUX)
#define EXPORT_SYMTAB
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
#include <linux/moduleparam.h>
#endif /* LINUX_VERSION_CODE */
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/errno.h>


#include "ioclxload.h"
#include "lx_loader.h"
#include "stdevice.h"
#include "memory_map.h"
/* =================================================================
   Local constants
   ================================================================= */

#define MODULE_DESCRIPTION_DEF     "LXLOAD STAPI (FK)"

#if !defined(MODULE_DESCRIPTION_DEF)
#define MODULE_DESCRIPTION_DEF     "LXLOAD STAPI (--)"
#endif

#define LXLOAD_STAT_NAME_LEN            (sizeof(LXLOAD_LINUX_DEVICE_NAME) + 2)

#if defined(ST_7100) || defined(ST_7109)
#define MAX_LX_CODE_SIZE                0x200000
/* static U32  AllowedAudioCodeAddress[] = {0x07C00000, 0x04100000, 0x04200000}; */
/* static U32  AllowedVideoCodeAddress[] = {0x07E00000, 0x04000000}; */
static U32  LxBaseAddress[] = { 0x04000000,
                                0x04200000};
#define REGION_2	0xA0000000
#elif defined(ST_7200)
#define MAX_VIDEO_LX_CODE_SIZE            0x300000
#define MAX_AUDIO_LX_CODE_SIZE            0x200000
static U32  LxBaseAddress[] = { 0x08600000,
                                0x08000000,
                                0x08800000,
                                0x08300000 };
#define REGION_2	0xA0000000
#endif

int Max_Load_Size=0;

static unsigned long    StartAddress = (unsigned long)NULL;

static unsigned long     LxToInitialize = (unsigned long)1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
module_param(LxToInitialize, long, 0644);
#else
MODULE_PARM(LxToInitialize, "long");
#endif
MODULE_PARM_DESC(LxToInitialize, "LX number to load: 2|4 for video, 1|3 for audio");
static unsigned long    PhysicalStartAddress;


/* =================================================================
   Local types prototypes
   ================================================================= */

/* =================================================================
   Local variables declaration
   ================================================================= */
static struct proc_dir_entry   *STAPI_dir ;
static struct cdev             *stlxload_cdev  = NULL;
static struct class_simple     *stlxload_class = NULL;
static unsigned int             stlxload_major;

static unsigned long     NbStoredData = 0;
static void            * LxStorageAddress;

/* =================================================================
   Local functions prototypes
   ================================================================= */

static int mod_lxload_open(struct inode* inode, struct file* file );
static int mod_lxload_release(struct inode* inode, struct file* file );
static int mod_lxload_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
static ssize_t mod_lxload_write(struct file * file, const char * buffer, size_t length, loff_t *offset);
#ifdef LXLOAD_DEBUG
static ssize_t mod_lxload_read(struct file * file, char * buffer, size_t length, loff_t *offset);
#endif

/* =================================================================
   Global data
   ================================================================= */

static struct file_operations lxload_fops = {
    owner:   THIS_MODULE,
    open:    mod_lxload_open,           /*  open     */
    write:   mod_lxload_write,          /*  write    */
#ifdef LXLOAD_DEBUG
    read:    mod_lxload_read,           /*  read     */
#endif
    release: mod_lxload_release,        /*  release  */
    ioctl:   mod_lxload_ioctl           /*  ioctl    */
};


/* =================================================================
   Open device
   ================================================================= */

static int lxload_read_proc( char *page, char **start, off_t off, int count, int *eof, void *data )
{
    sprintf(page, "LXLOAD module: statistics to be defined\n");

    return strlen(page);
}


static int mod_lxload_open(struct inode* inode, struct file* file )
{
    struct proc_dir_entry  * lxload_status;
    char                     status_name[LXLOAD_STAT_NAME_LEN];

    printk( "lxload_open: major=%d minor=%d\n",
            MAJOR(inode->i_rdev),
            MINOR(inode->i_rdev) );

    /* Creation of /proc/STAPI/statusxx files */
    sprintf(status_name, "%s%.2d", LXLOAD_LINUX_DEVICE_NAME, MINOR(inode->i_rdev));
    lxload_status = create_proc_read_entry(status_name, 0444,
                                STAPI_dir, lxload_read_proc, NULL );
    if ( lxload_status == NULL )
    {
        printk(KERN_ERR "mod_lxload_open(): create_proc_read_entry() error !!!\n");
        return( -1 );
    }
    lxload_status->owner = THIS_MODULE;

    LxStorageAddress = (void*)StartAddress;
    NbStoredData = 0;

    switch (LxToInitialize)
    {
        case 1:
        case 2:
#if defined(ST_7100) || defined(ST_7109)
            LxBootSetup(LxToInitialize-1, TRUE);
#elif defined (ST_7200)
        case 3:
        case 4:
            LxBootSetup(LxToInitialize-1, FALSE);
#endif /* defined(ST_7100) || defined(ST_7109) */
            break;

        default:
            printk(KERN_ERR "Unknown load LX no %d !!!\n" , LxToInitialize);
            /* Deletion of /proc/STAPI/statusxx file */
            remove_proc_entry(status_name, STAPI_dir);
            return( -1 );
            break;
    }

    return 0;
}

/* =================================================================
   Close device
   ================================================================= */

static int mod_lxload_release(struct inode* inode, struct file* file )
{
    char status_name[LXLOAD_STAT_NAME_LEN];

    printk( KERN_INFO "lxload_release: major=%d minor=%d\n",
            MAJOR(inode->i_rdev),
            MINOR(inode->i_rdev) );

    switch (LxToInitialize)
    {
        case 1:
        case 2:
#if defined(ST_7100) || defined(ST_7109)
            LxReboot(LxToInitialize-1, TRUE);
#elif defined (ST_7200)
        case 3:
        case 4:
            LxReboot(LxToInitialize-1, FALSE);
#endif /* defined(ST_7100) || defined(ST_7109) */
            break;

        default:
            break;
    }
    printk(KERN_INFO "LX loaded:\n  Base: 0x%.8x\n  Last: 0x%.8x\n  Size:%d\n", (U32)LxStorageAddress, (U32)(LxStorageAddress + NbStoredData), (U32)NbStoredData);

    /* Deletion of /proc/STAPI/statusxx file */
    sprintf(status_name, "%s%.2d", LXLOAD_LINUX_DEVICE_NAME, MINOR(inode->i_rdev));
    remove_proc_entry(status_name, STAPI_dir);

    return 0;
}


/* =================================================================
   The ioctl() implementation
   ================================================================= */

static int mod_lxload_ioctl (struct inode *inode, struct file *filp,
                      unsigned int cmd, unsigned long arg)
{
    int                 ret = 0;    /* OK by default */

    switch(cmd)
    {
        default:
            ret = -ENOTTY;
            break;
    }

    return ret;
}


static ssize_t mod_lxload_write(struct file * file, const char * buffer, size_t length, loff_t *offset)
{
    int             ret = 0;    /* OK by default */
#ifdef LXLOAD_DEBUG
    unsigned long   i;
#endif
    ret = copy_from_user((void *)((long)LxStorageAddress + NbStoredData), buffer, length);
    NbStoredData += length;

#ifdef LXLOAD_DEBUG
    printk("Base: 0x%.8x, Reached: 0x%.8x, Stored: %d, Read:%d\n", (long)LxStorageAddress, (long)LxStorageAddress + NbStoredData, NbStoredData, length);

    for (i=0 ; i<10 ; i++)
    {
        printk("[0x%.8x]= 0x%.8x\n", ((unsigned long)LxStorageAddress + NbStoredData - length + i*4),
                                     *((unsigned long *)((unsigned long)LxStorageAddress + NbStoredData - length + i*4)));
    }
#endif

    return(length);
}

#ifdef LXLOAD_DEBUG
static ssize_t mod_lxload_read(struct file * file, char * buffer, size_t length, loff_t *offset)
{
    unsigned long   i, j;
    unsigned long   Adresses[] = {  StartAddress,
                                    StartAddress + 0x100,
                                    StartAddress + 0x1000,
                                    StartAddress + 0x2000,
                                    StartAddress + 0x3000,
                                    (unsigned long)NULL};
    unsigned long   Read;


    for (j=0;;j++)
    {
        if ((Read = Adresses[j]) == (unsigned long)NULL)
            return(0);

        for (i=0 ; i<16 ; i++)
        {
            if (! (i%4))
                printk("\n0x%.8x:  ", Read + i*4);

            printk("0x%.8x ", *((unsigned long *)(Read + i*4)));
        }
        printk("\n");
    }
    return(0);
}
#endif

/* =================================================================
   Module installation
   ================================================================= */

static int __init mod_lxload_init(void)
{
    U32                     i;

    printk("\nmod_lxload_init: Initializing LXLOAD kernel Module, LX no %d\n", LxToInitialize);

    /* Check StartAdress */
#if defined(ST_7200)
    if (LxToInitialize < 1 || LxToInitialize > 4)
    {
        printk("Incorrect LX number !!!\n   Allowed values: 1, 2, 3, and 4\n");

        printk("      Audio: 1, 3\n");
        printk("      Video: 2, 4\n");

        return( -1 );
    }
#elif defined(ST_7100) || defined(ST_7209)
    if (LxToInitialize < 1 || LxToInitialize > 2)
    {
        printk("Incorrect LX number !!!\n   Allowed values: 1 and 2\n");

        printk("      Video: 1\n");
        printk("      Audio: 2\n");

        return( -1 );
    }
#endif /* ST_7200 */
    /* Register the device with the kernel */

#ifdef STLXLOAD_MAJOR
    stlxload_major = STLXLOAD_MAJOR;
#else
    stlxload_major = 0;
#endif
    if (STLINUX_DeviceRegister(&lxload_fops, 1, LXLOAD_LINUX_DEVICE_NAME,
                               &stlxload_major, &stlxload_cdev, &stlxload_class) != 0)
    {
        printk(KERN_ERR "%s(): STLINUX_DeviceRegister() error !!!\n", __FUNCTION__);
        return(-1);
    }

    /* Creation if not existing of /proc/STAPI directory */
    STAPI_dir = proc_mkdir(STAPI_STAT_DIRECTORY, NULL);
    if ( STAPI_dir == NULL )
    {
        printk(KERN_ERR "mod_lxload_init: proc_mkdir() error !!!\n");

        STLINUX_DeviceUnregister(1, LXLOAD_LINUX_DEVICE_NAME, stlxload_major, stlxload_cdev, stlxload_class);
        return(-1);
    }

    switch (LxToInitialize)
    {
		case 1:
#if defined(ST_7100) || defined(ST_7109)
		    Max_Load_Size = MAX_LX_CODE_SIZE;
#elif defined(ST_7200)
		    Max_Load_Size = MAX_AUDIO_LX_CODE_SIZE;
#endif
		    break;

		case 2:
#if defined(ST_7100) || defined(ST_7109)
            Max_Load_Size = MAX_LX_CODE_SIZE;
#elif defined(ST_7200)
			Max_Load_Size = MAX_VIDEO_LX_CODE_SIZE;
#endif
			break;

#if defined(ST_7200)
		case 3:
			Max_Load_Size = MAX_AUDIO_LX_CODE_SIZE;
            break;
		case 4:
			Max_Load_Size = MAX_VIDEO_LX_CODE_SIZE;
			break;
#endif

		default:
			printk(" Wrong LX to initiliase  %d\n ", LxToInitialize);
            return(-1);

	}


    /* base address */
    StartAddress = (unsigned long)STLINUX_MapRegion((void *)(((U32)LxBaseAddress[LxToInitialize-1])), Max_Load_Size, "LXLOADER");
    if (StartAddress == (unsigned long)NULL)
    {
		printk("STLINUX_MapRegion failedx\n");
		STLINUX_DeviceUnregister(1, LXLOAD_LINUX_DEVICE_NAME, stlxload_major, stlxload_cdev, stlxload_class);
        return(-1);
    }

    printk("LXLOADER virtual io kernel address of phys 0x%.8x = 0x%.8x\n", (U32)LxBaseAddress[LxToInitialize-1] , (U32)StartAddress);

    return (0);

    /* KERN_ERR: 3
       KERN_WARNING: 4
       KERN_NOTICE: 5
       KERN_INFO: 6
       KERN_DEBUG: 7 */
}

/* =================================================================
   Module removal
   ================================================================= */

static void __exit mod_lxload_exit(void)
{
    printk(KERN_INFO "mod_lxload_exit: Exiting LXLOAD kernel Module\n");

    STLINUX_UnmapRegion((void *)StartAddress, Max_Load_Size);
	Max_Load_Size =0;
    STLINUX_DeviceUnregister(1, LXLOAD_LINUX_DEVICE_NAME, stlxload_major, stlxload_cdev, stlxload_class);
}


module_init(mod_lxload_init);
module_exit(mod_lxload_exit);


MODULE_AUTHOR("Cyril Dailly <cyril.dailly@st.com>");
MODULE_DESCRIPTION(MODULE_DESCRIPTION_DEF);
MODULE_LICENSE("ST Microelectronics");

#endif /* ST_OSLINUX */

/* End of file */
