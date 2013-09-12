#define DEBUG_NOTIFY_STR "  " DEBUG_FUNC_STR ": "
#define DEBUG_ERROR_STR  "**" DEBUG_FUNC_STR ": "

#ifdef DEBUG

#ifdef __KERNEL__
#define MME_Info(_y) printk _y;
#else
#define MME_Info(_y) printf _y;
#endif /* __KERNEL__ */

#else
#define MME_Info(_y)
#endif

/* Need to coordinate numbers - Nick Haydock reckons he starts at 240 */
#define ALLOCATE_MAJOR_NUM 241
#define ALLOCATE_DEV_NAME  "mmetest"

#define ALLOCATE_IOCTL_MAGIC 'k'

typedef struct Allocate_s {
	/* IN */
	unsigned size;
	unsigned cached;
        /* OUT */
	unsigned mapSize;
	unsigned offset;
} Allocate_t;

typedef enum IoCtlCmds_e {
        Allocate_c = 1,
        Free_c
} IoCtlCmds_t;

#define ALLOCATE_IOX_ALLOCATE _IOWR(ALLOCATE_IOCTL_MAGIC, Allocate_c, Allocate_t)
#define ALLOCATE_IOX_FREE     _IOWR(ALLOCATE_IOCTL_MAGIC, Free_c,     int)

#if defined __LINUX__ && defined __KERNEL__

#include <asm/uaccess.h>

#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/bigphysarea.h>
#include <linux/version.h>

/* Kernel side driver */

MODULE_DESCRIPTION("MME Test Allocate Module");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("Copyright 2004 STMicroelectronics, All rights reserved");

static int Allocate(struct file* filp, unsigned long arg);
static int Free    (struct file* filp, unsigned long arg);

static int allocate_open   (struct inode* inode, struct file* filp);
static int allocate_release(struct inode* inode, struct file* filp);
static int allocate_ioctl  (struct inode* inode, struct file* filp, unsigned int command, unsigned long arg);
static int allocate_mmap   (struct file* filp, struct vm_area_struct* vma);

static struct file_operations allocate_ops = {
	open:    allocate_open,
	release: allocate_release,
	ioctl:   allocate_ioctl,
	mmap:    allocate_mmap,
	owner:   THIS_MODULE
};

/* ==========================================================================
 * 
 * Called by the kernel when the module is loaded
 *
 * ========================================================================== 
 */

#define DEBUG_FUNC_STR "init_module"
int init_module(void)
{
	MME_Info((DEBUG_NOTIFY_STR "Initializing /dev/%s - major num %d\n", 
                 ALLOCATE_DEV_NAME, ALLOCATE_MAJOR_NUM));

	int result =  register_chrdev(ALLOCATE_MAJOR_NUM, ALLOCATE_DEV_NAME, &allocate_ops);

        MME_Info((DEBUG_NOTIFY_STR "Driver %s registered\n", result?"*NOT* ":"" ));

	return result;
}
#undef DEBUG_FUNC_STR

/* ==========================================================================
 *
 * Called by the kernel when the module is unloaded
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "release_module"
void cleanup_module(void)
{
	MME_Info((DEBUG_NOTIFY_STR "unsregistering driver %s\n", ALLOCATE_DEV_NAME));

	unregister_chrdev(ALLOCATE_MAJOR_NUM, ALLOCATE_DEV_NAME);

	MME_Info((DEBUG_NOTIFY_STR "<<<<\n"));
}
#undef DEBUG_FUNC_STR

/* ==========================================================================
 *
 * Called by the kernel when the device is opened by an app (the mme user lib)
 *
 * ========================================================================== 
 */

#define DEBUG_FUNC_STR "allocate_open"
static int allocate_open(struct inode* inode, struct file* filp) 
{
	MME_Info((DEBUG_NOTIFY_STR ">>>>\n"));
#if 0	
	InstanceInfo_t* instanceInfo;


	instanceInfo = (InstanceInfo_t*) EMBX_OS_MemAlloc(sizeof(InstanceInfo_t));
	if (!instanceInfo) {
		goto nomem;
        }

	MME_Info((DEBUG_NOTIFY_STR "Instance 0x%08x\n", (int)instanceInfo));

	filp->private_data = instanceInfo;
#endif
	MME_Info((DEBUG_NOTIFY_STR "<<<< (0)\n"));
	return 0;
#if 0
nomem:
	MME_Info((DEBUG_NOTIFY_STR "<<<< (-ENOTTY)\n"));
	return -ENOTTY;
#endif	
}
#undef DEBUG_FUNC_STR 

/* ==========================================================================
 *
 * Called by the kernel when the device is closed by an app (the mme user lib)
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "allocate_release"
static int allocate_release(struct inode* inode, struct file* filp) 
{
#if 0
	EMBX_OS_MemFree(filp->private_data);
#endif
	MME_Info((DEBUG_NOTIFY_STR "<<<< (0)\n"));
	return 0;
}
#undef DEBUG_FUNC_STR 

/* ==========================================================================
 * 
 * Called by the kernel when an ioctl sys call is made
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "allocate_iooctl"
static int allocate_ioctl(struct inode* inode, struct file* filp, unsigned int command, unsigned long arg)
{
	int magic  = _IOC_TYPE(command);
	int op     = _IOC_NR(command);
	int result = -ENOTTY;
	MME_Info((DEBUG_NOTIFY_STR ">>>> command 0x%08x, op 0x%08x\n", command, op));

	if (ALLOCATE_IOCTL_MAGIC != magic) {
		MME_Info((DEBUG_ERROR_STR "<<<< -ENOTTY - command 0x%08x\n", command));
		return -ENOTTY;
	}

	switch (op) {
	case Allocate_c:
		MME_Info((DEBUG_NOTIFY_STR "Command Allocate\n"));
		result = Allocate(filp, arg);
		break;
	case Free_c:
		MME_Info((DEBUG_NOTIFY_STR "Command Free\n"));
		result = Free(filp, arg);
		break;
	default:
		MME_Info((DEBUG_ERROR_STR "Invalid ioctl command\n"));
		result =  -ENOTTY;
		break;
	}

	MME_Info((DEBUG_NOTIFY_STR "<<<< (%d)\n", result));
	return result;
}
#undef DEBUG_FUNC_STR 

/* ==========================================================================
 * 
 * allocate_mmap()
 * Called via mmap sys call from the mme user library
 *
 * ========================================================================== 
 */

#define DEBUG_FUNC_STR "allocate_mmap"
static int allocate_mmap(struct file* filp, struct vm_area_struct* vma)
{
	int                        result;
	unsigned long              size         = vma->vm_end-vma->vm_start;
	unsigned long              physical     = vma->vm_pgoff * PAGE_SIZE;

	/* Set the VM_IO flag to signify the buffer is mmapped
           from an allocated data buffer */
	vma->vm_flags |= VM_RESERVED | VM_IO;

#if defined __LINUX__ && defined __SH4__
        if (P2SEG == (physical & P2SEG)) {
	        /* Mark the physical area as uncached */
	        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
        }
        MME_Info((DEBUG_NOTIFY_STR "VM Prot 0x%08x", vma->vm_page_prot));
	MME_Info((DEBUG_NOTIFY_STR "physical adress 0x%08x\n", physical));
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        if (remap_page_range(vma->vm_start, physical, size, vma->vm_page_prot) < 0)
#else
        if (remap_pfn_range(vma, vma->vm_start, physical>>PAGE_SHIFT, size, vma->vm_page_prot) < 0)
#endif
	{
 	        MME_Info((DEBUG_ERROR_STR 
                          "Failed remap_page_range - start 0x%08x, phys 0x%08x, size %d\n", 
                          (int)(vma->vm_start), (int)physical, (int)size));
		result = -EAGAIN;
		goto exit;
	}

	MME_Info((DEBUG_NOTIFY_STR "Mapped virt 0x%08x len %d to phys 0x%08x\n", 
                 (int)vma->vm_start, (int)size, (int)physical));

	result = 0;

exit:
	MME_Info((DEBUG_NOTIFY_STR "<<<< (%d)\n", result));		
	return result;
}
#undef DEBUG_FUNC_STR 

#define DEBUG_FUNC_STR "Allocate"
static int Allocate(struct file* filp, unsigned long arg) {
	Allocate_t* allocateInfoPtr = (Allocate_t*)arg; 

	Allocate_t  allocateInfo;
	if (copy_from_user(&allocateInfo, allocateInfoPtr, sizeof(Allocate_t) )) {
		return -EFAULT;
	}
	
        unsigned int pages = (0 == allocateInfo.size?
                     0:
                     ((allocateInfo.size  - 1) / PAGE_SIZE) + 1);

       	unsigned alignedAddr = (unsigned) bigphysarea_alloc_pages(pages, 1, GFP_KERNEL);

        MME_Info(("bigphysarea_alloc_pages 0x%08x, pages %d pages\n", alignedAddr, pages));

        if (!alignedAddr) {
            return -ENOMEM;
        }

#if defined(__SH4__)
#if 0
        /* ensure there are no cache entries covering this address */
        dma_cache_wback_inv(alignedAddr, allocateInfo.size);
#endif
        /* finally warp the address into an (un)cached memory pointer */
        alignedAddr = allocateInfo.cached? 
                      P1SEGADDR(alignedAddr):
                      P2SEGADDR(alignedAddr);  

	allocateInfo.mapSize = pages * PAGE_SIZE;
	allocateInfo.offset  = alignedAddr;

	if (copy_to_user(allocateInfoPtr, &allocateInfo, sizeof(Allocate_t) )) {
		return -EFAULT;
	}

	return 0;
#endif
}

#undef DEBUG_FUNC_STR 
#define DEBUG_FUNC_STR "Free"
static int Free(struct file* filp, unsigned long address)
{
	/* Map virt to phys address for free */		
#if 0	
	struct vm_area_struct *	vma = find_vma(current->mm, address);
#endif
	/* Page table lookup */
	struct mm_struct *mm = current->mm;
	int write = 0;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *ptep, pte;
	struct page * thePage = NULL;

	spin_lock(&mm->page_table_lock);

	pgd = pgd_offset(mm, address);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		goto out;
   
	pmd = pmd_offset(pgd, address);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		goto out;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
	ptep = pte_offset(pmd, address);
#else
	ptep = pte_offset_map(pmd, address);
#endif
	if (!ptep) 
		goto out;
        
	pte = *ptep;
	if (pte_present(pte)) {
		if (!write || (pte_write(pte) && pte_dirty(pte)))
			thePage = pte_page(pte);
	}
out:
	spin_unlock(&mm->page_table_lock);
		
	if (thePage) {
		void* physAddress = P1SEGADDR(__pa(page_address(thePage)));
		bigphysarea_free_pages((void*)physAddress);
	}
	return 0;
}

#elif defined __LINUX__

/* User library */

/* For mmap() */
#include <unistd.h>
#include <sys/mman.h>
/* For open() */
#include <sys/types.h>
#include <sys/stat.h>

/* For ioctl() */
#include <sys/ioctl.h>

#include <fcntl.h>


#include <stdio.h>

static int fileDescriptor = -1;

#undef  DEBUG_FUNC_STR
#define DEBUG_FUNC_STR "KernelAllocate"

void* KernelAllocate(unsigned size, unsigned cached, unsigned* mapSize) {
	int result;
	void* address = NULL;
	Allocate_t allocInfo;

	allocInfo.size = size;
	allocInfo.cached = cached;
  
	result = ioctl(fileDescriptor, ALLOCATE_IOX_ALLOCATE, &allocInfo);
	MME_Info((DEBUG_ERROR_STR "Alloc  result %d\n", result));

	if (result >= 0) {
		*mapSize = allocInfo.mapSize;
		address = mmap(0, 
		               allocInfo.mapSize, 
		               PROT_READ | PROT_WRITE, MAP_SHARED,
			       fileDescriptor, 
			       allocInfo.offset);
		MME_Info((DEBUG_ERROR_STR "mmap address 0x%08x\n", address));
	        if (MAP_FAILED == address) {
	        	address = NULL;
	        }
	}
	MME_Info((DEBUG_ERROR_STR "<<<< address 0x%08x\n", address));
	return address;
}

#undef  DEBUG_FUNC_STR
#define DEBUG_FUNC_STR "KernelFree"
int KernelFree(void* address, unsigned length) {
	int result;

	result = ioctl(fileDescriptor, ALLOCATE_IOX_FREE, address);
	if (result<0) {
		return 0;
        }
	MME_Info( (DEBUG_ERROR_STR "Free ioctl result %d\n", result));

	result = munmap(address, length);
	MME_Info( (DEBUG_ERROR_STR "munmap result %d\n", result));

	return (0 == result)?1:0;
}

#undef  DEBUG_FUNC_STR
#define DEBUG_FUNC_STR "KernelAllocateInit"
int KernelAllocateInit() {
	if (fileDescriptor>=0) {
		return 0;
	}
	fileDescriptor = open("/dev/mmetest", O_RDWR);
	MME_Info( (DEBUG_ERROR_STR "Open %d\n", fileDescriptor));
	return (fileDescriptor>=0)?1:0;  
}

#undef  DEBUG_FUNC_STR
#define DEBUG_FUNC_STR "KernelAllocateDeinit"
int KernelAllocateDeinit() {
	if (fileDescriptor<0) {
		return 0;
	}
	close(fileDescriptor);
	MME_Info( (DEBUG_ERROR_STR "Closed %d\n", fileDescriptor));
	fileDescriptor = -1;
	return 1;
}
#endif
