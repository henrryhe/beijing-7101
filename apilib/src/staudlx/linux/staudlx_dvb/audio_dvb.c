/* ************************************************************************
   Copyright (c) Jan 2007, STMicroelectronics, NV. All Rights Reserved
   
   Author:        Udit Kumar
   ST Division:  HPC 
   ST Group:    CSD - MMSU Audio
   Project:       Linux STB wrapper on STAUDLX
   Purpose:       Linux Audio driver
   Date:          Last revision: Jan 2007
   Release:       1.0.0
   ************************************************************************ */
   

/* ************************************************************************
   I N C L U D E S
   ************************************************************************ */

/* System headers */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/kmod.h> /* for kerneld and devfsd support */
#include <linux/init.h>    /* Initiliasation support */
#include <linux/cdev.h>    /* Charactor device support */
#include <linux/netdevice.h> /* ??? SET_MODULE_OWNER ??? */
#include <linux/ioport.h>  /* Memory/device locking macros   */ 

/* UTS_RELEASE             - Kernel version  (text) */
/* LINUX_VERSION_CODE      - Kernel version  (hex)  */

#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/sched.h>   /* Defines pointer (current) to current task */
#if CONFIG_MODVERSIONS==1
#define MODVERSIONS
#include <linux/modversions.h>
#endif /* CONFIG_MODVERSIONS==1 */


#ifndef STAUDLX_DVB_MAJOR
#define STAUDLX_DVB_MAJOR   0
#endif

static unsigned int major = STAUDLX_DVB_MAJOR;
static struct cdev         *staudlx_dvb_cdev  = NULL;

#include "audio.h"
#include "audio_dvb.h"

#define MAJOR_NUMBER           188
#define DEVICE_NAME_PREFIX     "staudlx_dvb"
#define MAX_USERS              1
#define DEFAULT_VOLUME_LEFT    255
#define DEFAULT_VOLUME_RIGHT   255
/* Stream Types */

//#define DG_PRINTF(params)  {printk params; } 
#define DG_PRINTF(params)  { } 


/*Debugging Udit */


/*??? device contect allocation now static - should change to dynamic*/
#define MAX_AUDIO_DEVICES      8
static ST_DeviceName_t   ClockRecoveryDevice = "STCLK0";
static ST_DeviceName_t   EventDevice = "EVT0";
MODULE_DESCRIPTION("LINUX DVB Audio driver");
MODULE_AUTHOR("STMicroelectronics Ltd.");
MODULE_LICENSE("Proprietary");
module_param(ClockRecoveryDevice,charp,0644);
MODULE_PARM_DESC(ClockRecoveryDevice, "Clock Recovery Name");
module_param(EventDevice,charp,0644);
MODULE_PARM_DESC(EventDevice, "Event Device Name");

typedef enum AUDDVB_PlayState_e {
	AUDDVB_PLAYING,
	AUDDVB_PAUSED,
	AUDDVB_STOPPED 
}
AUDDVB_PlayState_t;


typedef enum AUDDVB_WriteState_e {
	AUDDVB_WRITE_STATE_AWAITING_DATA,
	AUDDVB_WRITE_STATE_AWAITING_DATA_REQUEST,
	AUDDVB_WRITE_STATE_INJECTING_DATA
}
AUDDVB_WriteState_t;

typedef struct STAUD_Audio_s {
	
	STAUD_Handle_t                 AUDHndl;
	ST_DeviceName_t                AUDDeviceName;
	STAUD_Object_t                 DecoderObject;
	STAUD_Object_t                 InputObject;
	int                            InputSource;
	STAUD_StreamParams_t           StreamParams;
	STAVMEM_BlockHandle_t          AVMEMBlockHandle;
	/* Fix current volume levels */
	STAUD_Attenuation_t            CurrentMixerSet;
	/* Write buffer data (local) */
	aud_shared_buffer_t        *SharedBuffer_p;
	struct semaphore               SharedBufferFree;
	AUDDVB_PlayState_t            DecoderState;
	AUDDVB_WriteState_t           WriteState;
	
	/* device number for linux device*/
	unsigned int                   DeviceNum;

	/* sync state */
        BOOL                           Synchronised;	

} STAUD_Audio_t;

typedef struct audio_device_s {
	audio_status_t             audio_status;
	unsigned int               audio_cap;
	struct semaphore           audio_mutex;
	
	/* This structure is used to store the address of STAUD internal buffer and info about *
	 * it's total and actual size and write (tail) and read (head) pointers.               */
	aud_shared_buffer_t    audio_write_buffer;
	
	wait_queue_head_t          shared_buffer_notfull_queue;
	unsigned int               max_attenuation_dac;
	
	audio_mixer_t              mixer_default;
	audio_mixer_t              mix_u, mix_current_setting, mix_internal;
	
	int                        users;

        int                        bypass_mode;
	STAUD_Handle_t                 AUDHndl;
	STAUD_Audio_t           STAUD_Device;	
	BOOL				Initailized;
} AUDDVB_Device_t;

static unsigned int audio_max_devices;
static AUDDVB_Device_t audio_device[MAX_AUDIO_DEVICES];
static BOOL AudioEnableSync = TRUE; 

static ST_ErrorCode_t AUDIO_Stop(STAUD_Audio_t *AudioDev_p, STAUD_Stop_t StopMode, STAUD_Fade_t *Fade_p); 
static ST_ErrorCode_t AUDIO_Start(STAUD_Audio_t *Audio_p); 
void staud_set_streamtype(STAUD_Audio_t  *Audio_p , int type); 
void  aud_audio_term(int i);
int  aud_audio_init(int i, ST_DeviceName_t Clock, ST_DeviceName_t Evt ); 
int staud_start_decoding(STAUD_Audio_t *Audio_p); 
int staud_stop(AUDDVB_Device_t  * Audio_p); 
int staud_set_channel(STAUD_Audio_t * Audio_p, int ChannelSelect); 


/* ==============================================

FILE OPERTAIONS OPEN/CLOSE/IOCTL ETC
================================================*/


/* WA to Terminate Audio Driver Properly */
/*One command to terminate [Device Number][Command ID]*/
static U8 DVBCommand[1][1]={{0}};
static task_t * LinuxDVBTask_p=NULL;
static semaphore_t * LinuxDVBTaskSem_p=NULL;
static BOOL TaskTerminateCommand = FALSE;

int LinuxDVBTask(void *Params)
{
	ST_ErrorCode_t Error=ST_NO_ERROR;
	STOS_TaskEnter(NULL);
	
	while(1)
	{
		STOS_SemaphoreWait(LinuxDVBTaskSem_p);

		{
				STAUD_TermParams_t Term;
				
				if(DVBCommand[0][0]==1)
				{
					DG_PRINTF((KERN_INFO "Terminating Audio 0 \n"));	
					Error=STAUD_Term (audio_device[0].STAUD_Device.AUDDeviceName, &Term);
					DVBCommand[0][0] = 0;
					DG_PRINTF((KERN_INFO  "Terminated Audio 0 Error =%d\n", Error));	
					
				}
			
				
				if(Error!=0)
				{
					printk(KERN_ALERT "STAUD Term Failed Erorr =%d \n ", Error );
				}
				if(TaskTerminateCommand==TRUE)
				{
						DG_PRINTF((KERN_INFO  "ALSA Task Stop \n"));
						
						STOS_SemaphoreDelete(NULL, LinuxDVBTaskSem_p); 
						LinuxDVBTask_p = NULL; 
						return 0;
				}

		}

	}
}


/* ************************************************************************
   F U N C T I O N S
   ************************************************************************ */


static ST_ErrorCode_t AUDIO_GetWriteAddress(void * const Handle, void ** const Address_p)
{
	AUDDVB_Device_t *Audio_p = (AUDDVB_Device_t *) Handle;
	int Head, Tail, Size;
	
	spin_lock(Audio_p->audio_write_buffer.lock);
	Size = Audio_p->audio_write_buffer.size;
	Tail = Audio_p->audio_write_buffer.tail;
	Head = Audio_p->audio_write_buffer.head;
	spin_unlock(&Audio_p->audio_write_buffer.lock);
	
	if (Size < Audio_p->audio_write_buffer.bufsize){
		*Address_p = (void*)(Audio_p->audio_write_buffer.buffer + Tail);
	}
	else {
		/* We never tell video that the buffer is completely full */
		if (Tail >= 4){
			*Address_p = (void*) (Audio_p->audio_write_buffer.buffer + Tail - 4);
		}
		else{
			*Address_p = (void*) (Audio_p->audio_write_buffer.buffer + Tail + Audio_p->audio_write_buffer.bufsize - 4);
		}
	}
	
	/*DG_PRINTF((KERN_INFO "audio: AUDIO_GetWriteAddress returns 0x%x\n", (unsigned int)(*Address_p)));*/
	
	return ST_NO_ERROR;
}

static ST_ErrorCode_t AUDIO_InformReadAddress(void * const Handle, void * const Address_p)
{
	AUDDVB_Device_t *Audio_p = (AUDDVB_Device_t *) Handle;
	int Head, NewHead, BytesRead, Full, Size;
	void *LogAddress_p;

	LogAddress_p = Address_p;
	
	NewHead = ((U8*)LogAddress_p) - Audio_p->audio_write_buffer.buffer;
	
	spin_lock(Audio_p->audio_write_buffer.lock);
	Size = Audio_p->audio_write_buffer.size;
	Head = Audio_p->audio_write_buffer.head;
	Full = (Audio_p->audio_write_buffer.size == (Audio_p->audio_write_buffer.bufsize));
	
	BytesRead = NewHead - Head;
	if (BytesRead < 0)
	    BytesRead += Audio_p->audio_write_buffer.bufsize;
	
	Audio_p->audio_write_buffer.size -= BytesRead;
	Audio_p->audio_write_buffer.head += BytesRead;
	if (Audio_p->audio_write_buffer.head >= Audio_p->audio_write_buffer.bufsize)
	    Audio_p->audio_write_buffer.head -= Audio_p->audio_write_buffer.bufsize;
	
	spin_unlock(&Audio_p->audio_write_buffer.lock);
	
	
	/*DG_PRINTF((KERN_INFO "consumed %d bytes\n", BytesRead));*/
	/*DG_PRINTF((KERN_INFO "New size = %d\n", Audio_p->SharedBuffer_p->size));*/
	
	if (Full){
	    if (BytesRead > 0){
		/*buffer was previously full - we can wake up the writer */
		/*DG_PRINTF((KERN_INFO "Signalling write manager\n"));*/
		wake_up(&Audio_p->shared_buffer_notfull_queue);
	    }
	}
	
	/*DG_PRINTF((KERN_INFO "audio: AUDIO_InformReadAddress returns 0x%x\n", (unsigned int)(Address_p)));*/
	
	return ST_NO_ERROR;
}




/* stdoc ******************************************************************

   Name:        AUDIO_DisableSynchronization []
   
   Purpose:     
   
   Arguments:   <AUD_Audio_t *Audio_p>
   
   ReturnValue: ( ST_ErrorCode_t )
   
   Errors:      
   
   Example:     
   
   SeeAlso:     
   
   Note:        
   
   Public:      No
   
   ************************************************************************ */

static ST_ErrorCode_t AUDIO_DisableSynchronization(STAUD_Audio_t *Audio_p)
{
	ST_ErrorCode_t   ErrCode;

	DG_PRINTF((KERN_INFO "audio: >>> AUDIO_DisableSynchronization()\n"));
	
	ErrCode = STAUD_DisableSynchronisation(Audio_p->AUDHndl);
	Audio_p->Synchronised = FALSE;
	if(ErrCode!=ST_NO_ERROR)
	{
		printk(KERN_ALERT "audio:  %x = AUDIO_DisableSynchronization()\n", ErrCode);
	}
	DG_PRINTF((KERN_INFO "audio: <<< %x = AUDIO_DisableSynchronization()\n", ErrCode));
	
	return ErrCode;
}





/* stdoc ******************************************************************

   Name:        AUDIO_EnableSynchronization []
   
   Purpose:     
   
   Arguments:   <AUD_Audio_t *Audio_p>
   
   ReturnValue: ( ST_ErrorCode_t )
   
   Errors:      
   
   Example:     
   
   SeeAlso:     
   
   Note:        
   
   Public:      No
   
   ************************************************************************ */

static ST_ErrorCode_t AUDIO_EnableSynchronization(STAUD_Audio_t *Audio_p)
{
    ST_ErrorCode_t   ErrCode=0;
    AUDDVB_PlayState_t DecoderState;
    STAUD_Stop_t    StopMode;
    STAUD_Fade_t    Fade;

    DG_PRINTF((KERN_INFO "audio: >>> AUDIO_EnableSynchronization()\n"));
    DecoderState = Audio_p->DecoderState;
	if (AudioEnableSync == TRUE)
	  {

		if (Audio_p->Synchronised == FALSE)
		{
		    
		    if (DecoderState == AUDDVB_PLAYING)
		    {
			StopMode = STAUD_STOP_NOW;  /* Default Setting */
			Fade.FadeType = STAUD_FADE_NONE;
			ErrCode = AUDIO_Stop(Audio_p, StopMode, &Fade);
			if (ErrCode != ST_NO_ERROR)
		   	 {
				printk(KERN_ALERT "audio: AUDIO_EnableSynchronization(): AUDIO_Stop() failed with %d \n", ErrCode);
		    	 }
		   
			ErrCode = AUDIO_Start(Audio_p);
			if (ErrCode != ST_NO_ERROR)
		   	 {
				printk(KERN_ALERT "audio: AUDIO_EnableSynchronization(): AUDIO_Start() failed with %d n \n", ErrCode);
		    	 }
		    }

		    /*enable sync right after audio start*/
		    ErrCode = STAUD_EnableSynchronisation(Audio_p->AUDHndl);
		    if (ErrCode != ST_NO_ERROR)
		    {
			printk(KERN_ALERT "audio: AUDIO_EnableSynchronization(): STAUD_EnableSynchronization() failed with %d \n", ErrCode);
		    }
		    

		    Audio_p->Synchronised = TRUE;
		}
		else
		{
		    DG_PRINTF((KERN_INFO "audio: AUDIO_EnableSynchronization(): already synchronised\n"));
		}
   	 }
  	  else
	   {
			DG_PRINTF((KERN_INFO "audio: AUDIO_EnableSynchronization(): synchronisation forced to DISABLED\n"));
			ErrCode = STAUD_DisableSynchronisation(Audio_p->AUDHndl);
			 if (ErrCode != ST_NO_ERROR)
			{
				printk(KERN_ALERT "audio: AUDIO_EnableSynchronization(): STAUD_DisableSynchronisation() failed with %d \n", ErrCode);
		    	}
	    }

    DG_PRINTF((KERN_INFO "audio: <<< %x = AUDIO_EnableSynchronization()\n", ErrCode));
    
    return ErrCode;
}


/* stdoc ******************************************************************

   Name:        aud_open [ Open Sti710x Audio device]
   
   Purpose:     Open Sti710x Audio Decoder
   
   Arguments:   <struct *inode>:       Inode for audio device file
                <struct file *file>:   File Pointer
                
		ReturnValue: (Int) Error Code
   
		Errors:      ENODEV:       Device Driver not loaded/available
                EINTERNAL:    Internal error
		EBUSY:        Device or resource busy
		EINVAL:       Invalid Argument
		0     :       No Error

   Example:     None
   
   SeeAlso:     aud_close
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_open(struct inode *inode, struct file *file)
{
	int minor,i=0;
	
	minor = MINOR(inode->i_rdev);
	
	/* Start the open fops for audio module */
	DG_PRINTF((KERN_INFO "audio: >>> aud_open()\n"));
	
	if (minor >= audio_max_devices) 
	{
		printk(KERN_ERR "audio: <<< ENODEV = aud_open ()\n");
		return -ENODEV;
	}
	
	if ((audio_device[minor].users >= MAX_USERS) && ((file->f_flags & O_ACCMODE) != O_RDONLY)) 
	{
		printk(KERN_ERR "audio: <<< EBUSY = aud_open()\n");
		return -EBUSY;
	}
	
	file->private_data = &audio_device[minor];
	
	if ((file->f_flags & O_ACCMODE) == O_RDWR) 
	{
		if (audio_device[minor].users == 0) 
		{
			DG_PRINTF((KERN_INFO "audio: <<< aud_open() opening device RDWR \n"));
			audio_device[minor].users++;
		}
		else 
		{
			printk(KERN_ERR "audio: <<< EBUSY for Mode= aud_open()\n");
			return -EBUSY;
		}
	} 
	else if ((file->f_flags & O_ACCMODE) == O_RDONLY) 
	{
		DG_PRINTF((KERN_INFO "audio: <<< aud_open() opening device ReadOnly \n"));
	} 
	else 
	{
		printk(KERN_ERR "audio: <<<  EINVAL = _aud_open() \n");
		return -EINVAL;
	}
	
	
	
	for (i = 0; i < audio_max_devices; i++)
	{
		if(audio_device[i].Initailized == FALSE)
		{
			if (aud_audio_init(i,ClockRecoveryDevice,EventDevice) != 0)
			{
					printk(KERN_ERR "audio: aud_open: staud_init failed (%d) failed \n", i);
									
				for (i--; i >= 0; i--)
				{
					aud_audio_term(i);
				}
			
				printk(KERN_ERR "audio: <<< -EINTERNAL = aud_open() \n");
				
				return -EINTERNAL;
			}
			audio_device[i].Initailized = TRUE;
		}
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_open()\n"));
	return 0;
	
}/* aud_open */



/* stdoc ******************************************************************

   Name:        aud_close
   
   Purpose:     Close the Sti710x Audio Decoder Device
   
   Arguments:   <struct inode *inode>:   Inode of the audio device
                <struct file *file>:     Audio device File Pointer
   
   ReturnValue: (int) Error Code
   
   Errors:      0:   No Error
   EBADF:       I:   valid file descriptor
   
   Example:     None
   
   SeeAlso:     aud_open
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_close(struct inode *inode, struct file *file)
{
	int minor = MINOR(inode->i_rdev);
	int i=0;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_close()\n"));
	
	if((file->f_flags & O_ACCMODE) == O_RDONLY) 
	{
		DG_PRINTF((KERN_INFO "audio: RD Mode aud_close()\n"));
		
	} 
	else if ((file->f_flags & O_ACCMODE) == O_RDWR) 
	{
		
		DG_PRINTF((KERN_INFO "audio: RDWR  Mode  aud_close()\n"));
		audio_device[minor].users--;
	} 
	else 
	{
		printk(KERN_ALERT "audio: <<< EBADF = aud_close()\n");
		return -EBADF;
	}

	if((file->f_flags & O_ACCMODE) == O_RDWR)
	{
		for (i = 0; i < audio_max_devices; i++) 
		{
			aud_audio_term(i);
		}
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_close()\n"));
	return 0;
	
}/* aud_close */



/* stdoc ******************************************************************

   Name:        aud_read
   
   Purpose:     Read from Audio device
   
   Arguments:   <struct inode *inode>: Audio stream inode
                <struct file *file>:   Audio file pointer
		<const void *buffer>:  Audio stream memory buffer
		<size_t count>:        Audio stream memory buffer size
   
   ReturnValue: None
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     aud_write
   
   Note:        This is a dummy implemetation
   
   Public:      No
   
   ************************************************************************ */

static ssize_t aud_read(struct file *file, char *buf_read, size_t len_read, loff_t *offset_read)
{
        return 0;
}




/* stdoc ******************************************************************

   Name:        write
   
   Purpose:     Inject the audio stream from memory to Audio Bit Buffer
   
   Arguments:   <struct inode *inode>: Audio stream inode
                <struct file *file>:   Audio file pointer
		<const void *buffer>:  Audio stream memory buffer
		<size_t count>:        Audio stream memory buffer size
   
   ReturnValue: (int): error code
   
   Errors:      0= write success; negative value: error occurs
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static ssize_t aud_write(struct file *file, const char *buf, size_t len, loff_t *offset)
{
	int    size, first_seg_size;
	__u32  head, tail;
	AUDDVB_Device_t *dev = file->private_data;
	wait_queue_t mywait;
	
	if (dev->audio_status.stream_source != AUDIO_SOURCE_MEMORY) 
	{
		printk(KERN_ALERT  "audio: <<< EPERM = aud_write()\n");
		return -EPERM;
	}
	
	if (len == 0) 
	{
		return 0;
	}
	
	spin_lock(dev->audio_write_buffer.lock);
	size = dev->audio_write_buffer.size;
	head = dev->audio_write_buffer.head;
	tail = dev->audio_write_buffer.tail;
	spin_unlock(dev->audio_write_buffer.lock);
	
	if (file->f_flags & O_NONBLOCK) 
	{
		if (size == dev->audio_write_buffer.bufsize)
		{
			printk(KERN_ALERT  "audio: <<< ENOMEM = aud_write()\n");
			return -ENOMEM;
	    	}
	} 
	else 
	{
	    while (size == (dev->audio_write_buffer.bufsize)) 
		{
			init_wait(&mywait);
			prepare_to_wait(&dev->shared_buffer_notfull_queue, &mywait, TASK_INTERRUPTIBLE);

			spin_lock(dev->audio_write_buffer.lock);
			size = dev->audio_write_buffer.size;
			head = dev->audio_write_buffer.head;
			tail = dev->audio_write_buffer.tail;
			spin_unlock(dev->audio_write_buffer.lock);
			
			if (size == (dev->audio_write_buffer.bufsize))
			    schedule( );
			finish_wait(&dev->shared_buffer_notfull_queue, &mywait);
	/*				interruptible_sleep_on(&dev->shared_buffer_notfull_queue);*/
			if (signal_pending(current))
			{
			    printk(KERN_ERR "audio: <<< ERESTARTSYS = aud_write()\n");
			    return -ERESTARTSYS;
			}
			
			spin_lock(dev->audio_write_buffer.lock);
			size = dev->audio_write_buffer.size;
			head = dev->audio_write_buffer.head;
			tail = dev->audio_write_buffer.tail;
			spin_unlock(dev->audio_write_buffer.lock);
			
		};
	}
	
	if (len > (dev->audio_write_buffer.bufsize) - size) 
	{
		len = (dev->audio_write_buffer.bufsize) - size;
		/*DG_PRINTF((KERN_ERR "audio: length resized to %d\n", len));*/
	}
	
	/* Now we write the data passed by the user inside STAPI buffer, that is pointed to by buf. *
	 * The number of bytes to write is stored inside the variable len, which has been modified  *
	 * (if necessary) in case there isn't enough free space in the destination buffer. We also  *
	 * handle the possible wrap of the pointer.                                                 */
	
	if (len + tail > dev->audio_write_buffer.bufsize) 
	{
		first_seg_size = (dev->audio_write_buffer.bufsize) - tail;
		
		if (copy_from_user(dev->audio_write_buffer.buffer + tail, buf, first_seg_size) != 0) 
		{
		        printk(KERN_ALERT  "audio: <<< EFAULT = aud_write()\n");
			return -EFAULT;
		}
		
		if (copy_from_user(dev->audio_write_buffer.buffer, buf + first_seg_size, len - first_seg_size) != 0) 
		{
		        printk(KERN_ALERT  "audio: <<< EFAULT = aud_write()\n");
			return -EFAULT;
		}
	} 
	else 
	{
		if ((copy_from_user(dev->audio_write_buffer.buffer + tail, buf, len)) != 0) 
		{
		       printk(KERN_ALERT KERN_ERR "audio: <<< EFAULT 2 = aud_write()\n");
			return -EFAULT;
		}
	}
	
	spin_lock(dev->audio_write_buffer.lock);
	size = dev->audio_write_buffer.size;
	dev->audio_write_buffer.size += len;
	dev->audio_write_buffer.tail += len;
		
	if (dev->audio_write_buffer.tail >= (dev->audio_write_buffer.bufsize))
	{
		dev->audio_write_buffer.tail -= (dev->audio_write_buffer.bufsize);
	}
	
	spin_unlock(dev->audio_write_buffer.lock);
	
	return len;
	
} /*  write */



/* stdoc ******************************************************************
   
   Name:        aud_poll
   
   Purpose:     Queue the current process in any wait wueue
   
   Arguments:   <struct file *file>: File pointer
                <poll_table *wait>:  Process table
   
   ReturnValue: Result of the Poll system call(Process permis.)
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static unsigned int aud_poll(struct file *file, poll_table *wait)
{
	int result = 0, size;
	AUDDVB_Device_t *dev = file->private_data;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_poll()\n"));
	poll_wait(file, &dev->shared_buffer_notfull_queue, wait);
	result = 0;
	
	spin_lock(dev->audio_write_buffer.lock);
	size = dev->audio_write_buffer.size;
	spin_unlock(dev->audio_write_buffer.lock);
	
	if (size < (dev->audio_write_buffer.bufsize)) 
	{
		DG_PRINTF((KERN_INFO "audio: Size of Audio Buffer: %d\n", size));
		result |= (POLLOUT | POLLWRNORM);
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< %d =  aud_poll()\n", result));
	return result;
	
} /* aud_poll */




/* stdoc ******************************************************************

   Name:        aud_play
   
   Purpose:     Ask to Sti710x Audio Decoder to play audio stream
   
   Arguments:   None
   
   ReturnValue: (int):       Error Code
   
   Errors:      0:         No Errors
                EINTERNAL: Internal error
   
   Example:     None
   
   SeeAlso:     aud_pause, aud_stop
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_play(AUDDVB_Device_t *dev)
{
	int err_code;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_play()\n"));
	
	if (dev->audio_status.play_state == AUDIO_PAUSE) 
	{
		DG_PRINTF((KERN_INFO "audio: <<< EINVAL = aud_play()\n"));
		return -EINVAL;
	}
	
	if (dev->audio_status.play_state == AUDIO_PLAYING) 
	{
		DG_PRINTF((KERN_INFO "audio: aud_play(): device is already in play mode\n"));
		DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_play()\n"));
		return 0;
	}
	else 
	{
		err_code = staud_start_decoding(&dev->STAUD_Device);

		if (dev->STAUD_Device.InputSource == AUDDVB_SOURCE_MEMORY)
		{
		    
			DG_PRINTF((KERN_INFO "audio: aud_play(): switching to MEMORY interface\n"));
			err_code = STAUD_IPSetDataInputInterface(dev->AUDHndl,
								STAUD_OBJECT_INPUT_CD0,
								AUDIO_GetWriteAddress,
								AUDIO_InformReadAddress,
								(void * const)dev);
		    
		}
			
		if (err_code == 0) 
		{
			dev->audio_status.play_state = AUDIO_PLAYING;
			DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_play()\n"));
			return 0;
		}
		else 
		{
			printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_play() Failure error=%d \n",err_code);
			return -EINTERNAL;
		}
	}
	DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_play()\n"));
	return 0;
	
} /* aud_play */


/* stdoc ******************************************************************

   Name:        aud_stop
   
   Purpose:     Asks to Sti710x Audio Decoder to stop playing the stream
   
   Arguments:   None
   
   ReturnValue: (int):  Error Code
   
   Errors:      0:           No Errors occurs
                EINTERNAL:   Internal error
		
   Example:     None
   
   SeeAlso:     aud_play, aud_pause
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_stop(AUDDVB_Device_t *dev)
{
	int err_code;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_stop()\n"));
	
	if ((dev->audio_status.play_state == AUDIO_PLAYING) || (dev->audio_status.play_state == AUDIO_PAUSED)) {
		err_code = staud_stop(dev);
		
		if (err_code == 0) {
			dev->audio_status.play_state = AUDIO_STOPPED;
			DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_stop()\n"));
			return 0;
		}
		else 
		{
			printk(KERN_ALERT "audio: aud_stop(): aud_stop failur error =%d \n",err_code);
			printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_stop()\n");
			return -EINTERNAL;
		}
	}
	else 
	{
		DG_PRINTF((KERN_INFO "audio: aud_stop:  audio decoder is already in STOP mode\n"));
		DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_stop()\n"));
		return 0;
	}
	
	return 0;
	
} /* aud_stop */



/* stdoc ******************************************************************

   Name:        aud_pause
   
   Purpose:     Pause Mode for Sti710x Audio Decoder
   
   Arguments:   None
   
   ReturnValue: (int):  Error Code
   
   Errors:      ST_NO_ERROR:    No Errors
                Other Value:    Error condition
   
   Example:     None
   
   SeeAlso:     aud_start_decoding
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_pause(STAUD_Audio_t *Audio_p)
{
	ST_ErrorCode_t ErrCode;
	STAUD_Fade_t   Fade;
	
	
	DG_PRINTF((KERN_INFO "audio: >>> staud_pause()\n"));
	
	ErrCode = STAUD_DRPause(Audio_p->AUDHndl, Audio_p->InputObject, &Fade);
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT "audio: staud_pause(): STAUD_DRPause() Failure %x\n", ErrCode);
		printk(KERN_ALERT  "audio: <<< -1 = staud_pause()\n");
		return -1;
	}

	Audio_p->DecoderState = AUDDVB_PAUSED;
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = staud_pause()\n"));
	
	return 0;
	
} /* aud_pause */

/* stdoc ******************************************************************
   
   Name:        aud_pause
   
   Purpose:     Pause mode for Sti710x Audio Decoder
   
   Arguments:   None
   
   Return Value: (int): ErroCode
   
   Errors:      0:         No Errors
                EINTERNAL: Internal error
                
  Example:     None
   
   SeeAlso:     aud_play
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_pause(AUDDVB_Device_t *dev)
{
	int err_code = 0;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_pause()\n"));
	
	if (dev->audio_status.play_state == AUDIO_PAUSED) 
	{
		DG_PRINTF((KERN_ERR "audio:aud_pause(): Device is already in pause mode\n"));
		DG_PRINTF((KERN_ERR "audio: <<< 0 = aud_pause()\n"));
		return 0;
	}
	
	if (dev->audio_status.play_state == AUDIO_PLAYING)
	{
		err_code = staud_pause(&dev->STAUD_Device);
		
		if (err_code == 0) 
		{
			DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_pause() Success\n"));
			dev->audio_status.play_state = AUDIO_PAUSED;
			return 0;
		} 
		else 
		{
			printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_pause(), error =%d\n", err_code);
			return -EINTERNAL;
		}
	}
	else
	{
		printk(KERN_ALERT  "audio: The Audio Decoder is not playing\n");
		printk(KERN_ALERT  "audio: <<< EINVAL = aud_pause() \n");
		return -EINVAL;
	}
} /* aud_pause */



/* stdoc ******************************************************************

   Name:        aud_continue
   
   Purpose:     Resume Sti710x Audio Decoder from PAUSE mode
   
   Arguments:   None
   
   ReturnValue: (int):   Error Code
   
   Errors:      ST_NO_ERROR:                No Errors
                ST_ERROR_INVALID_HANDLE:    Invalid Audio handle
                STAUD_ERROR_INVALID_DEVICE: Unrecognized Audio Decoder
   
   Example:     None
   
   SeeAlso:     aud_pause
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_continue(STAUD_Audio_t *Audio_p)
{
	static ST_ErrorCode_t ErrCode;

	
	DG_PRINTF((KERN_INFO "audio: >>> staud_continue()\n"));
	
	ErrCode = STAUD_DRResume(Audio_p->AUDHndl, Audio_p->InputObject);
	
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "audio: staud_continue(): STAUD_DRResume() error %x\n", ErrCode);                
		printk(KERN_ALERT  "audio: -1 = <<< staud_continue() error\n");		
		return -1;
	}

	
	Audio_p->DecoderState = AUDDVB_PLAYING;
	
	DG_PRINTF((KERN_INFO "audio: 0 = <<< staud_continue()\n"));
	
	return 0;
	
} /* aud_continue */



/* stdoc ******************************************************************

   Name:        aud_continue
   
   Purpose:     Resumes the Sti710x audio decoder from the paused state
   
   Arguments:   None
   
   ReturnValue: (int):   Error code
   
   Errors:      0:  No errorrs
                EINVAL:     Invalid arguments
   
   Example:     None
   
   SeeAlso:     aud_pause
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_continue(AUDDVB_Device_t*dev)
{
	int err_code;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_continue()\n"));
	
	if (dev->audio_status.play_state != AUDIO_PAUSED) 
	{
		printk(KERN_ALERT  "audio: aud_continue(): device not paused\n");
		printk(KERN_ALERT "audio: <<< EINVAL = aud_continue()\n");
		return -EINVAL;
	}
	
	err_code = staud_continue(&dev->STAUD_Device);
	
	if (err_code == 0) 
	{
		dev->audio_status.play_state = AUDIO_PLAYING;
		DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_continue()\n"));
		return 0;
	}
	else 
	{
		printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_continue() error=%d \n",err_code);
		return -EINTERNAL;
	}
} /* aud_continue */


/* stdoc ******************************************************************

   Name:        aud_ioctl
   
   Purpose:     Defines IOCTL for STI710x Linux Audio Device Driver
   
   Arguments:   <struct inode *inode>:   Inode for the audio devie file
                <struct file *file>:     File pointer
                <unsigned int cmd>:      Select the IOCTL
                <unsigned long arg>:     IOCTL Arguments 
   
   ReturnValue: (int):        Error Code
   
   Errors:      0:            No Errors
                ERROR:        Error Condition
                
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */



/* stdoc ******************************************************************

   Name:        aud_select_source
   
   Purpose:     Select the stream source between DEMUX and MEMORY
   
   Arguments:   <int mode>: Switch the stream source
   
   ReturnValue: (int):  Error Code
   
   Errors:      ST_NO_ERROR:   No Errors
                Other Values:  Error condition
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_select_source(STAUD_Audio_t *Audio_p, int mode)
{
	
	
	
	DG_PRINTF((KERN_ERR "audio: >>> staud_select_source(%d)\n", mode));
	
	Audio_p ->InputSource = mode;
	
	switch(mode){
		case AUDDVB_SOURCE_MEMORY:
			
			DG_PRINTF((KERN_INFO "audio: staud_select_source(): switching to MEMORY interface\n"));
			
			break;
			    
		case AUDDVB_SOURCE_DEMUX:
			DG_PRINTF((KERN_INFO "audio: staud_select_source(): switching to DEMUX interface\n"));
		       break;
		
		default:
			printk(KERN_ALERT  "audio: <<< -1 = staud_select_source()\n");
			return -1;
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = staud_select_source()\n"));
	
	return 0;
	
}




/* stdoc ******************************************************************

   Name:        aud_select_source
   
   Purpose:     This IOCTL call informs the audio device which source
                shall be used for the input data.
   
   Arguments:   <audio_stream_source_t source>:  Select Audio stream source
   
   ReturnValue: (int):   Error code
   
   Errors:      0:    No Errors
                EINVAL:       Illegal input parameters 
   
   Example:     None
   
   SeeAlso:     aud_play, aud_write
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_select_source(AUDDVB_Device_t *dev, audio_stream_source_t source)
{
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_select_source()\n"));
	
	switch(source){
		
		case AUDIO_SOURCE_DEMUX:
			DG_PRINTF((KERN_INFO "audio: aud_select_source() Source: DEMUX\n"));
			if (aud_stop(dev) != 0) 
			{
				printk(KERN_ALERT  "audio: <<< -EIO = aud_select_source() \n");
				return -EIO;
			}
			
			if (staud_select_source(&dev->STAUD_Device, AUDDVB_SOURCE_DEMUX) != 0) 
			{
				printk(KERN_ALERT "audio: <<< -EINVAL = aud_select_source() \n");
				return -EINVAL;
			}

			
			dev->audio_status.stream_source = AUDIO_SOURCE_DEMUX;
			
			/* To avoid to restart the decoder before it's done *
			 * stopping - which would result in a misfunction.  */
			
			
			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout(100);
			
			aud_play(dev);
			
			
			DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_select_source()\n"));
			return 0;
			break;
		
		case AUDIO_SOURCE_MEMORY:
			DG_PRINTF((KERN_INFO "audio: aud_select_source() Source: MEMORY\n"));
			
			if (aud_stop(dev) != 0) 
			{
				printk(KERN_ALERT  "audio: <<< -EIO = M aud_select_source() \n");
				return -EIO;
			}
			
			
			if (staud_select_source(&dev->STAUD_Device, AUDDVB_SOURCE_MEMORY) != 0) 
			{
				printk(KERN_ALERT  "audio: aud_select_source(): M aud_select_source() failed\n");
				printk(KERN_ALERT  "audio: <<< -EIO = M aud_select_source() \n");
				return -EIO;
			}
			else 
			{

				if (STAUD_IPSetDataInputInterface(dev->AUDHndl,
							    STAUD_OBJECT_INPUT_CD0,
							    AUDIO_GetWriteAddress,
							    AUDIO_InformReadAddress,
							    dev))
							    return -1;


				spin_lock(dev->audio_write_buffer.lock);
				
				dev->audio_write_buffer.size = 0;
				dev->audio_write_buffer.head = 0;
				dev->audio_write_buffer.tail = 0;
							
				/* unlock the shared buffer (against the write method) */
				spin_unlock(dev->audio_write_buffer.lock);
				
							
				dev->audio_status.stream_source = AUDIO_SOURCE_MEMORY;
/*Should here we call aud_play or not udit*/				
				DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_select_source() \n"));
				return 0;
			}
			
		default:
		       printk(KERN_ALERT  "audio: <<< -EINVAL = aud_select_source() \n");
			return -EINVAL;
			break;
	}
}




/* stdoc ******************************************************************

   Name:        aud_mute
   
   Purpose:     STi710x Audio Decoder Pause Mode
   
   Arguments:   None
   
   ReturnValue: (int):          Error Code
   
   Errors:      ST_NO_ERROR:    No Errors
                Other Values:   Error condition
   
   Example:     None
   
   SeeAlso:     aud_startdecoding
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_mute(STAUD_Audio_t *Audio_p )
{
	ST_ErrorCode_t ErrCode;

	
	DG_PRINTF((KERN_INFO "audio: >>> staud_mute()\n"));
	
	/*ErrCode = STAUD_Mute (Audio_p->AUDHndl, TRUE, TRUE);*/
	ErrCode = STAUD_DRMute (Audio_p->AUDHndl, Audio_p->DecoderObject);
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "audio: staud_mute(): STAUD_DRMute failure %d\n", ErrCode);
		printk(KERN_ALERT  "audio: <<< -1 = staud_mute()\n");
		return -1;
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = staud_mute()\n"));
	
	return 0;
	
} /* aud_mute */



/* stdoc ******************************************************************

   Name:        aud_unmute
   
   Purpose:     Un Mute Speaker on Sti710x platform
   
   Arguments:   None
   
   ReturnValue: (int):          ErrCode
   
   Errors:      ST_NO_ERROR:    No Errors
                Other Values:   Error Condition
   
   Example:    None
   
   SeeAlso:    aud_mute
   
   Note:       None
   
   Public:     No
   
   ************************************************************************ */

int staud_unmute(STAUD_Audio_t *Audio_p )
{
	ST_ErrorCode_t ErrCode;
	
	
	DG_PRINTF((KERN_INFO "audio: >>> staud_unmute()\n"));
	
	/*ErrCode = STAUD_Mute (Audio_p->AUDHndl, FALSE, FALSE);*/
	ErrCode = STAUD_DRUnMute (Audio_p->AUDHndl, Audio_p->DecoderObject);
	
	if (ErrCode == ST_NO_ERROR)
	{
		DG_PRINTF((KERN_INFO "audio: %x =<<< staud_unmute() Success\n", ErrCode));
	}
	else
	{
		printk(KERN_ALERT  "audio: %x =<<< staud_unmute() Failure\n", ErrCode);
	}
	
	return (int) ErrCode;
	
} /* aud_unmute */



/* stdoc ******************************************************************

   Name:        aud_set_mute
   
   Purpose:     This call asks the audio device to mute the Speakers
   
   Arguments:   None
   
   ReturnValue: (int):        Error Code
   
   Errors:      0:        No Errors
                EINVAL:   Illegal input parameter
		EINTERNAL:Internal error
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_set_mute(AUDDVB_Device_t *dev, int state)
{
	int err_code;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_set_mute()\n"));
	
	switch(state) 
	{
		
		case 1:
			
			if (dev->audio_status.mute_state == 1) 
			{
				DG_PRINTF((KERN_INFO "audio: aud_set_mute(): is already ON\n"));
				DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_mute()\n"));
				return 0;
			}
			else 
			{
				err_code = staud_mute(&dev->STAUD_Device);
				if (err_code == 0) 
				{
					DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_mute()\n"));
					dev->audio_status.mute_state = 1;
					return 0;
				}
				else 
				{
					printk(KERN_ALERT  "audio: aud_set_mute() failure ON \n");
					printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_set_mute()\n");
					return -EINTERNAL;
				}
			}
			
			break;
		
		case 0:
			
			if (dev->audio_status.mute_state == 0) 
			{
				DG_PRINTF((KERN_INFO "audio: aud_set_mute(): is already OFF\n"));
				DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_mute()\n"));
				return 0;
			}
			else 
			{
				err_code = staud_unmute(&dev->STAUD_Device);
				if (err_code == 0)
				{
					DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_mute():\n"));
					dev->audio_status.mute_state = 0;
					return 0;
				}
				else
				{
					printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_set_mute OFF ()\n");
					return -EINVAL;
				}
			}
			
			break;
		
		default:
			return -EINVAL;
			break;
	}
} /* aud_set_mute */




/* stdoc ******************************************************************

   Name:        aud_enablesync
   
   Purpose:     Start the Sti710x Audio Decoding Syncronization
   
   Arguments:   None
   
   ReturnValue: None
   
   Errors:      ST_NO_ERROR:   No Errors
                Other Values:  Error Condition
   
   Example:     None
   
   SeeAlso:     aud_start_decoding
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_enablesync(STAUD_Audio_t *Audio_p)
{

	ST_ErrorCode_t   ErrCode = 0;
	
	DG_PRINTF((KERN_INFO "audio: >>> staud_enablesync()\n"));
	
	ErrCode = AUDIO_EnableSynchronization(Audio_p);
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT "audio: staud_enablesync: AUDIO_EnableSynchronization() failure %x\n", ErrCode);
		printk(KERN_ALERT   "audio: <<< -1 = staud_enablesync()\n");
		return -1;
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = staud_enablesync()\n"));
	
	return 0;
	
} /* aud_EnableSync */



/* stdoc ******************************************************************

   Name:        aud_disablesync
   
   Purpose:     Terms the Sti710x Audio Decoding Syncronization
   
   Arguments:   None
   
   ReturnValue: None
   
   Errors:      ST_NO_ERROR:   No Errors
                Other Values:  Error Condition
   
   Example:     None
   
   SeeAlso:     aud_start_decoding, aud_enablesync
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_disablesync(STAUD_Audio_t  *Audio_p)
{
	ST_ErrorCode_t ErrCode;

	
	DG_PRINTF((KERN_INFO "audio: >>> staud_disablesync()\n"));
	
	ErrCode = AUDIO_DisableSynchronization (Audio_p);
	
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "audio: staud_disablesync(): STAUD_OPDisableSynchronization() failure %x\n", ErrCode);
		printk(KERN_ALERT  "audio: <<< -1 = staud_disablesync()\n");
		return -1;
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = staud_disablesync()\n"));
	
	return 0;
	
} /* aud_disablesync */


/* stdoc ******************************************************************

   Name:        aud_set_avsync
   
   Purpose:     Turn ON or OFF the Audio Video Syncronization
   
   Arguments:   <int state>:    Defines the State of the AV Sync.
   
   ReturnValue: (int):    Error Code
   
   Errors:      0:      No errors
                EINVAL: Invalid parameters
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_avsync(AUDDVB_Device_t *dev, int state)
{
	int err_code = 0;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_set_avsync()\n"));
	
	if (state != 0) 
	{
		if(state>1)
		{
			printk(KERN_ALERT "audio <<< aud_set_avsync : EINVALD \n "); 
			return -EINVAL;
		}
		if (dev->audio_status.AV_sync_state == 1) 
		{
		        DG_PRINTF((KERN_INFO "audio: aud_avsync(): sync is already ON\n"));
			DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_avsync()\n"));
			return 0;
		}
		else
		{
			err_code = staud_enablesync(&dev->STAUD_Device);
			
			if (err_code == 0) 
			{
				AudioEnableSync = TRUE;
				dev->audio_status.AV_sync_state = 1;
				DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_avsync() ON\n"));
				return 0;
			}
			else 
			{
				printk(KERN_ALERT "audio: aud_set_avsync(): aud_enablesync() failure\n");
				printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_set_avsync()\n");
				return -EINTERNAL;
			}
		}
	}
	else 
	{
		if (dev->audio_status.AV_sync_state == 0) 
		{
		        DG_PRINTF((KERN_INFO "audio: aud_avsync(): sync is already OFF\n"));
			DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_avsync()\n"));
			return 0;
		}
		else 
		{
			err_code = staud_disablesync(&dev->STAUD_Device);
			if (err_code == 0) 
			{
				AudioEnableSync= FALSE; 
				dev->audio_status.AV_sync_state = 0;
				DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_avsync()\n"));
				return 0;
			}
			else 
			{
				printk(KERN_ALERT  "audio: aud_set_avsync(): aud_disablesync() failure\n");
				printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_set_avsync()\n");
				return -EINTERNAL;
			}
		}
	} /* state == 0 */
	
	return 0;
	
} /* aud_set_avsync */




/* stdoc ******************************************************************

   Name:        aud_channel_select
   
   Purpose:     Asks the Audio Decoder to select the requested channel
   
   Arguments:   <audio_channel_select_t source>:  Defines the Channel Source
   
   ReturnValue: (int): Error code
   
   Errors:      0: No Errors
                EINVAL:    Illegal parameters
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_channel_select(AUDDVB_Device_t *dev, audio_channel_select_t source)
{
	int err_code = 0;
	int mode;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_channel_select()\n"));
	
	switch(source) {
		
		case AUDIO_STEREO:
		mode = AUDDVB_CHANNEL_SELECT_MODE_STEREO;
		break;
		
		case AUDIO_MONO_LEFT:
		mode = AUDDVB_CHANNEL_SELECT_MODE_DUAL_LEFT;
		break;
		
		case AUDIO_MONO_RIGHT:
		mode = AUDDVB_CHANNEL_SELECT_MODE_DUAL_RIGHT;
		break;
		
		case AUDIO_MONO:
		mode = AUDDVB_CHANNEL_SELECT_MODE_DUAL_MONO;
		break;
		
		case AUDIO_AUTO:
		mode = AUDDVB_CHANNEL_SELECT_MODE_AUTO;
		break;
		
		default:
		printk(KERN_ALERT  "audio: aud_channel_select(): invalid source %d \n", source);
		printk(KERN_ALERT  "audio: <<< EINVAL = aud_channel_select()\n");
		return -EINVAL;
		
	} /*  switch(source) */
	

	err_code = staud_set_channel(&dev->STAUD_Device, mode);
			
	if (err_code == 0) 
	{
		dev->audio_status.channel_select = source;
		DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_channel_select()\n"));
		return 0;
	}
	else
	{
		printk(KERN_ALERT "audio: aud_channel_select(): aud_set_channel() failure error=%d \n",err_code);
		printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_channel_select() \n");
		return -EINTERNAL;
	}

	return 0;
	
}/* aud_channel_select */




/* stdoc ******************************************************************

   Name:        aud_get_capability
   
   Purpose:     Return the capability of the Sti710x Audio Decoder
   
   Arguments:   <unsigned int capability>:  The Audio capabilities
   
   ReturnValue: (int):  Error Code
   
   Errors:      ST_NO_ERROR:              No Errors
                ST_ERROR_BAD_PARAMETER:   Bad Audio parameters
                ST_ERROR_UNKNOWN_DEVICE:  Audio Device bad configuration
   
   Example:     None
   
   SeeAlso:     
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_get_capability(STAUD_Audio_t *Audio_p, unsigned int *capability)
{
	int                      ErrCode;
	STAUD_DRCapability_t     DecoderCapability;
	
	
	DG_PRINTF((KERN_INFO "audio: >>> staud_get_capability()\n"));
	
	ErrCode = STAUD_DRGetCapability (Audio_p->AUDDeviceName,
					 Audio_p->DecoderObject,
					&DecoderCapability);
	
	if  (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "audio: staud_get_capability(): STAUD_DRGetCapability() failed error=%d\n",ErrCode);
		printk(KERN_ALERT  "audio: <<< -1 = aud_get_capability()\n");
		return -1;
	}
	
	*capability = (unsigned int) DecoderCapability.SupportedStreamContents;
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = staud_get_capability()\n"));
	
	return 0;
	
} /* aud_getcapability */



/* stdoc ******************************************************************

   Name:        aud_get_capability
   
   Purpose:     Returns informations about Sti710x Audio Dec. capabilities
   
   Arguments:   <unsigned int cap>:    Describes the Audio Dec. capabilities
   
   ReturnValue: (int):  Error Code
   
   Errors:      0:     No errors
                EFAULT:        Parameters point an invalid address
                EINVAL:        Invalid parameters
   
   Example:     None
   
   SeeAlso:     aud_get_status
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_get_capability(AUDDVB_Device_t  *dev, unsigned int *cap)
{
	int err_code;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_get_capability()\n"));
	
	err_code = staud_get_capability(&dev->STAUD_Device, &dev->audio_cap);
	
	if (err_code == 0) 
	{
		if (((dev->audio_cap)&(AUDDVB_CAP_DTS)) != 0) 
		{
			*cap |= AUDIO_CAP_DTS;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_LPCM)) != 0)
		{
			*cap |= AUDIO_CAP_LPCM;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_MPEG1)) != 0) 
		{
			*cap |= AUDIO_CAP_MP1;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_MPEG2)) != 0)
		{
			*cap |= AUDIO_CAP_MP2;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_MP3)) != 0) 
		{
			*cap |= AUDIO_CAP_MP3;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_MPEG_AAC)) != 0) 
		{
			*cap |= AUDIO_CAP_AAC;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_HE_AAC)) != 0) 
		{
			*cap |= AUDIO_CAP_HE_AAC;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_OGG)) != 0) 
		{
			*cap |= AUDIO_CAP_OGG;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_SDDS)) != 0) 
		{
			*cap |= AUDIO_CAP_SDDS;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_AC3)) != 0) 
		{
			*cap |= AUDIO_CAP_AC3;
		}
		if (((dev->audio_cap)&(AUDDVB_CAP_PCM)) != 0)
		{
			*cap |= AUDIO_CAP_PCM;
		}

		DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_get_capability() \n"));
		return 0;
	}
	else 
	{
		printk(KERN_ALERT  "audio: aud_get_capability(): aud_get_capability() failed error = %d\n",err_code);
		printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_get_capability() Failure\n");
		return -EINTERNAL;
	}
	return 0;
} /* aud_get_capability */





/* stdoc ******************************************************************

   Name:        aud_set_bypass_mode []
   
   Purpose:     
   
   Arguments:   <aud_handle_t AudioHandle>
                <int BypassMode>
   
   ReturnValue: (int) 0 on success, -1 on failure
   
   Errors:      
   
   Example:     
   
   SeeAlso:     
   
   Note:        
   
   Public:      ???
   
   ************************************************************************ */

int staud_set_bypass_mode(STAUD_Audio_t  *Audio_p, int BypassMode)
{
	ST_ErrorCode_t        ErrCode;

	STAUD_DigitalOutputConfiguration_t DigitalOutParams = {0};
	
	DG_PRINTF((KERN_INFO "audio: >>> staud_set_bypass_mode()\n"));


	DigitalOutParams.DigitalMode = (BypassMode != 0) ?
	    STAUD_DIGITAL_MODE_NONCOMPRESSED : STAUD_DIGITAL_MODE_COMPRESSED;

	ErrCode = STAUD_SetDigitalOutput(Audio_p->AUDHndl, DigitalOutParams);
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT "audio: staud_set_bypass_mode(): STAUD_OPSetDigitalMode() failed with error %d\n", ErrCode);
		printk(KERN_ALERT  "audio: -1 = <<< staud_set_bypass_mode() mode=%d\n", DigitalOutParams.DigitalMode);
		return -1;
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = staud_set_bypass_mode()\n"));
	
	return 0;
	
}/* aud_clear_buffer */



/* stdoc ******************************************************************

   Name:        aud_set_bypass_mode
   
   Purpose:     This call asks the Audio Decoder to bypass Audio decoding
   
   Arguments:   None
   
   ReturnValue: (int): Error Code
   
   Errors:      0:        No Errors
                ENOTSUPP: ioctl() not supported
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        Not Supported!
   
   Public:      No
   
   ************************************************************************ */

static int aud_set_bypass_mode(AUDDVB_Device_t *dev, int bypass_mode)
{
	int err_code;
	audio_play_state_t last_state;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_set_bypass_mode()\n"));

	last_state = dev->audio_status.play_state;
	
	if (last_state == AUDIO_PLAYING || last_state == AUDIO_PAUSED)
	{
	    if (aud_stop(dev) != 0) 
		{
			printk(KERN_ALERT "audio: aud_set_bypass_mode(): failed to stop audio \n");
			printk(KERN_ALERT  "audio: <<< EIO = aud_set_bypass_mode() \n");
			return -EIO;
	    }
	}

	if(bypass_mode >1 )
	{
		printk(KERN_ALERT " audio :<<< aud_set_bypass_mode EINVAL \n");
		return -EINVAL;
	}
	err_code = staud_set_bypass_mode(&dev->STAUD_Device, bypass_mode);
		
	if (err_code != 0) 
	{
	    printk(KERN_ALERT "audio: aud_set_bypass_mode(): failed to set bypass mode \n");
	    printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_set_bypass_mode(%d) Failure\n", bypass_mode);
	   return -EINTERNAL; 
	}

	dev->audio_status.bypass_mode = bypass_mode;

	if (last_state == AUDIO_PLAYING || last_state == AUDIO_PAUSED) 
	{
	    if (aud_play(dev) != 0)
		{
			printk(KERN_ALERT  "audio: aud_set_bypass_mode(): failed to restart audio \n");
			printk(KERN_ALERT  "audio: <<< EIO = aud_set_bypass_mode() \n");
			return -EIO;
	    }
	}

	if (last_state == AUDIO_PAUSED) 
	{
	    if (aud_pause(dev) != 0) 
		{
			printk(KERN_ALERT  "audio: aud_set_bypass_mode(): failed to resume audio \n");
			printk(KERN_ALERT  "audio: <<< EIO = aud_set_bypass_mode() \n");
			return -EIO;
	    }
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_bypass_mode(%d) Success\n", bypass_mode));
	
	return 0;
}



/* stdoc ******************************************************************

   Name:        audio_set_id
   
   Purpose:     This ioctl selects whioch sub-stream is to be decoded
   
   Arguments:   <int id>:   Audio Stream ID
   
   ReturnValue: (int):   Error code
   
   Errors:       0:        No Errors
                 ENOTSUPP: ioctl() not supported
   
   Example:      None
   
   SeeAlso:      None 
   
   Note:         None
   
   Public:       No
   
   ************************************************************************ */

static int aud_set_id(AUDDVB_Device_t *dev, int id)
{
	ST_ErrorCode_t ErrCode;
	U32 val=id;
	DG_PRINTF((KERN_INFO " audio :>>> aud_set_id()\n "));
	if(dev->STAUD_Device.StreamParams.StreamType==STAUD_STREAM_TYPE_ES)
	{
		printk(KERN_ALERT "audio :<<< aud_set_id() EINVAL -- Set to ES ");
		return -EINVAL;
	}
	switch(dev->STAUD_Device.StreamParams.StreamContent)
	{
		case STAUD_STREAM_CONTENT_AC3:
			if(val<0x80 || val >0x87)
			{
				printk(KERN_ALERT "audio :<<< aud_set_id() EINVAL -- AC3  ");
				return -EINVAL;
			}
			break;
			
			case STAUD_STREAM_CONTENT_MPEG1:
			case STAUD_STREAM_CONTENT_MPEG2:
			case STAUD_STREAM_CONTENT_MPEG_AAC:
			if(val<0xc0 || val>0xDF)
			{	
				printk(KERN_ALERT "audio :<<< aud_set_id() EINVAL --MPEG");
				return -EINVAL;
			}
			break;
			
			case STAUD_STREAM_CONTENT_LPCM:
			case STAUD_STREAM_CONTENT_LPCM_DVDA:
			if(val< 0xA0 || val> 0xA7)
			{
				printk(KERN_ALERT "audio :<<< aud_set_id() EINVAL -- LPCM ");
				return -EINVAL;
			}
			break; 
			default :
			{
				printk(KERN_ALERT "audio :<<< aud_set_id() EINVAL --default ");
				return -EINVAL;
				
			}
			break;
	}
	ErrCode =  STAUD_IPSetStreamID(dev->AUDHndl,STAUD_OBJECT_INPUT_CD0,val);
	if(ErrCode!=0)
	{
		printk(KERN_ALERT "audio:<<< aud_set_id STAUD failed error =%d \n ", ErrCode);
		return -EINTERNAL;
	}
	DG_PRINTF((KERN_INFO " audio :0 <<< aud_set_id()\n "));
	return 0;
} /* aud_set_id */




/* stdoc ******************************************************************

   Name:        aud_log2linear
   
   Purpose:     Converts the logaritmic value into linear value. Used for
                converting the volume expressed in logarithmic scale into
		linear value
   
   Arguments:   <unsigned int max_attenuation_dac>: Max volume
                <unsigned int log_volume>: logarithmic value
   
   ReturnValue: <unsigned int>: Linear value
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

unsigned int staud_log2linear(unsigned int max_attenuation_dac, unsigned int log_volume)
{
	static unsigned int vol2attenuation[256] = {
		48, 42, 38, 36, 34, 32, 31, 30, 29, 28, 27, 26, 25, 25, 24, 24,
		23, 23, 22, 22, 21, 21, 20, 20, 20, 19, 19, 19, 18, 18, 18, 18,
		17, 17, 17, 17, 16, 16, 16, 16, 15, 15, 15, 15, 15, 14, 14, 14,
		14, 14, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12,
		11, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10,
		9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  8,  8,  8,  8,  8,  8,
		8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
		7,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  5,
		5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  4,
		4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
		3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
		3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
		2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
	};
	
	if (log_volume > 255){
		log_volume = 255;
	}
	
	return vol2attenuation[log_volume];
} /* aud_log2linear */


       
/* stdoc ******************************************************************

   Name:        aud_set_volume
   
   Purpose:     Set the Audio volume fo Speakers
   
   Arguments:   <unsigned int volume_left>:   Set the volume for the left Sp.
                <unsigned int volume_right>:  Set the volume for the rigth Sp.
   
   ReturnValue: (int):  Error code
   
   Errors:      ST_NO_ERROR:                     No errors
                ST_ERROR_FEATURE_NOT_SUPPORTED:  Features not support
		ST_ERROR_INVALID_HANDLE:         Invalid Audio handle
		STAUD_ERROR_INVALID_DEVICE:      Invalid audio device
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_set_volume(STAUD_Audio_t *Audio_p,
		       unsigned int volume_left,
		       unsigned int volume_right)
{
	static ST_ErrorCode_t       ErrCode;
	static STAUD_Attenuation_t  Mixer;

	
	DG_PRINTF((KERN_INFO "audio: >>> staud_set_volume(...)\n"));
	
	Mixer.Left  = (U16) volume_left;
	Mixer.Right = (U16) volume_right;
	
	ErrCode = STAUD_SetAttenuation(Audio_p->AUDHndl, Mixer);
	
	if (ErrCode == ST_NO_ERROR)
	{
		DG_PRINTF((KERN_INFO "audio: 0 = staud_set_volume(SPEAKER LEFT ATTEN.(db):%d, SPEAKER RIGHT ATTEN.(db):%d) Success\n", Mixer.Left, Mixer.Right));
	}
	else
	{
		printk(KERN_ALERT  "audio: error %d = <<< staud_set_volume(SPEAKER LEFT ATTEN.(db):%d, SPEAKER RIGHT ATTEN.(db):%d) Failure\n",
			   ErrCode,
			   Mixer.Left,
			   Mixer.Right);
	}
	
	return (int) ErrCode;
	
} /* aud_set_volume */


/* stdoc ******************************************************************

   Name:        audio_set_mixer
   
   Purpose:     Set the Audio Volume for Audio Speakers
   
   Arguments:   <audio_mixer_t *mix>:    Mixer Settings
   
   ReturnValue: (int):    Errors Code
   
   Errors:      0:           No Errors
                EFAULT:      Invalid pointer
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_set_mixer(AUDDVB_Device_t *dev, audio_mixer_t *mix)
{
	int err_code;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_set_mixer()\n"));
	
	/* STAPI call */
	dev->mix_u.volume_left  = staud_log2linear(dev->max_attenuation_dac,
						     mix->volume_left);
	dev->mix_u.volume_right = staud_log2linear(dev->max_attenuation_dac,
						     mix->volume_right);
	
	if ((dev->mix_u.volume_left != -1) && (dev->mix_u.volume_right != -1)) 
	{
		
		dev->mix_internal.volume_left  = mix->volume_left;
		dev->mix_internal.volume_right = mix->volume_right;
		
		/* STAPI call */
		err_code = staud_set_volume(&dev->STAUD_Device,
					      dev->mix_u.volume_left,
					      dev->mix_u.volume_right);
		
		if (err_code == 0) 
		{
			DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_mixer(LEFT: %2d, RIGHT: %2d) Success\n",
				   dev->mix_u.volume_left,
				   dev->mix_u.volume_right));
			return 0;
		}
		else 
		{
			printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_set_mixer(LEFT: %2d, RIGHT: %2d) Failure\n",
				   dev->mix_u.volume_left,
				   dev->mix_u.volume_right);
			return -EINTERNAL;
		}
	} /* if ((mix_u.volume_left != -1)&&(mix_u.volume_right != -1)) */
	
	return 0;
	
} /* aud_set_mixer */




/* stdoc ******************************************************************

   Name:        aud_clear_buffer
   
   Purpose:     Clear the software/hardware buffer which contains stream
   
   Arguments:   None
   
   ReturnValue: <int>: err_code
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_clear_buffer(AUDDVB_Device_t *Audio_p)
{

	
	DG_PRINTF((KERN_INFO "audio: >>> aud_clear_buffer()\n"));
	
	/* It is not necessary to check the return code */
	/* lock the shared buffer (against the write method) */
	spin_lock(Audio_p->audio_write_buffer.lock);
	

	
	Audio_p->audio_write_buffer.size = 0;
	Audio_p->audio_write_buffer.head = 0;
	Audio_p->audio_write_buffer.tail = 0;
	
	
	
	/* unlock the shared buffer (against the write method) */
	spin_unlock(Audio_p->audio_write_buffer.lock);
	
	DG_PRINTF((KERN_INFO "audio: <<< aud_clear_buffer done\n"));
	
	return 0;
} /* aud_clear_buffer */




/* stdoc ******************************************************************

   Name:        aud_set_streamtype
   
   Purpose:     Defined the type for audio stream according to the Audio
                Decoder capabilities
   
   Arguments:   <int type>:  Defines the audio stream type
   
   ReturnValue: (int):   Error Code
   
   Errors:      0:  No errors
                E_INVAL:    Illegal parameters
   
   Example:     None
   
   SeeAlso:     aud_get_capability
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_set_streamtype(AUDDVB_Device_t *dev, int type)
{
	int err_code;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_set_streamtype()\n"));
	
	err_code = aud_get_capability(dev, &dev->audio_cap);
	
	DG_PRINTF((KERN_INFO "audio: capability %x\n", dev->audio_cap));
	
	if (err_code != 0) 
	{
		printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() aud_get_capability failed\n");
		return -EINVAL;
	}
	
	switch(type) 
	{
		
		case AUDIO_CAP_DTS:
			if (((dev->audio_cap)&(AUDDVB_CAP_DTS)) == 0) 
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() DTS unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_DTS);
				DG_PRINTF((KERN_INFO "audio: aud_set_streamtype(): SOURCE: DTS\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_LPCM:
			if (((dev->audio_cap)&(AUDDVB_CAP_LPCM)) == 0)
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() LPCM unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_LPCM);
				DG_PRINTF((KERN_INFO "audio: <<< 0 aud_set_streamtype(): SOURCE: LPCM\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_MP1:
			if (((dev->audio_cap)&(AUDDVB_CAP_MPEG1)) == 0) 
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() MP1 unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_MPEG1);
				DG_PRINTF((KERN_INFO "audio: <<< 0  aud_set_streamtype(): SOURCE: MP1\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_MP2:
			if (((dev->audio_cap)&(AUDDVB_CAP_MPEG2)) == 0) 
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() MP2 unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_MPEG2);
				DG_PRINTF((KERN_INFO "audio:  <<< 0  aud_set_streamtype(): SOURCE: MP2\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_MP3:
			if (((dev->audio_cap)&(AUDDVB_CAP_MP3)) == 0) 
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() MP3 unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_MP3);
				DG_PRINTF((KERN_INFO "audio:  <<< 0  aud_set_streamtype(): SOURCE: MP3\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_AAC:
			if (((dev->audio_cap)&(AUDDVB_CAP_MPEG_AAC)) == 0) 
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() AAC unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_MPEG_AAC);
				DG_PRINTF((KERN_INFO "audio:  <<< 0  aud_set_streamtype(): SOURCE: AAC\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_HE_AAC:
			if (((dev->audio_cap)&(AUDDVB_CAP_HE_AAC)) == 0) 
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() AAC unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_HE_AAC);
				DG_PRINTF((KERN_INFO "audio:  <<< 0  aud_set_streamtype(): SOURCE: HE_AAC\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_OGG:
			if (((dev->audio_cap)&(AUDDVB_CAP_OGG)) == 0) 
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() OGG unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_OGG);
				DG_PRINTF((KERN_INFO "audio:  <<< 0 aud_set_streamtype(): SOURCE: OGG\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_SDDS:
			if (((dev->audio_cap)&(AUDDVB_CAP_SDDS)) == 0)
			{
				printk(KERN_ALERT  "audio: <<< EINVAL = aud_set_streamtype() SDDS unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_SDDS);
				DG_PRINTF((KERN_INFO "audio:  <<< 0 aud_set_streamtype(): SOURCE: SDDS\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_AC3:
			if (((dev->audio_cap)&(AUDDVB_CAP_AC3)) == 0) 
			{
				printk(KERN_ALERT "audio: <<< EINVAL = aud_set_streamtype() AC3 unsupported\n");
				return -EINVAL;
			}
			else 
			{
				staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_AC3);
				DG_PRINTF((KERN_INFO "audio:  <<< 0 aud_set_streamtype(): SOURCE: AC3\n"));
				return 0;
			}
			break;
		
		case AUDIO_CAP_PCM:
			staud_set_streamtype(&dev->STAUD_Device, (int) AUDDVB_CAP_PCM);
			DG_PRINTF((KERN_INFO "audio:  <<< 0 aud_set_streamtype(): SOURCE: PCM\n"));
			return 0;
		break;
		
		case AUDIO_PES:
			staud_set_streamtype(&dev->STAUD_Device, (int) AUDIO_PES);
			DG_PRINTF((KERN_INFO "audio:  <<< 0  aud_set_streamtype() TYPE: PES\n"));
			return 0;
			break;
		
		case AUDIO_ES:
			staud_set_streamtype(&dev->STAUD_Device, (int) AUDIO_ES);
			DG_PRINTF((KERN_INFO "audio:  <<< 0 aud_set_streamtype() TYPE: ES\n"));
			return 0;
			break;
		
		default:
			printk(KERN_ALERT  "audio: aud_set_streamtype() streamtype %x unsupported\n", type);
			printk(KERN_ALERT "audio: <<< EINVAL = aud_set_streamtype()\n");
			return -EINVAL;
	}
	
	return -ENOTSUPP;
	
} /* aud_set_streamtype */



/* stdoc ******************************************************************

   Name:        aud_set_attributes
   
   Purpose:     Set the attributes of the audio stream
   
   Arguments:   <audio_attributes_t attr>:    Defines the Audio attributes
   
   ReturnValue: (int):   Error code
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        None
   
   Public:      No
   
   ************************************************************************ */

static int aud_set_attributes(AUDDVB_Device_t *dev, audio_attributes_t attr)
{
        return 0;
} /* aud_set_attributes */



/* stdoc ******************************************************************

   Name:        aud_set_ext_id
   
   Purpose:     None
   
   Arguments:   None
   
   ReturnValue: None
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        Do not support for this project
   
   Public:      No
   
   ************************************************************************ */

static int aud_set_ext_id (AUDDVB_Device_t *dev, int ext_id)
{
        return -ENOTSUPP;
} /* aud_set_ext_id */



/* stdoc ******************************************************************

   Name:        aud_set_karaoke
   
   Purpose:     None
   
   Arguments:   None
   
   ReturnValue: None
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     None
   
   Note:        Do not support for this project
   
   Public:      No
   
   ************************************************************************ */

static int aud_set_karaoke (AUDDVB_Device_t *dev, audio_karaoke_t *karaoke)
{
        return -ENOTSUPP;
}/* aud_set_karaoke */




int staud_set_format(STAUD_Audio_t *Audio_p,
		       int sample_rate,
		       int data_precision,
		       int channels)
{
	
	ST_ErrorCode_t  ErrCode;
	STAUD_PCMInputParams_t Params;
	
	DG_PRINTF((KERN_INFO "audio: >>> staud_set_format()\n"));
	
	Params.Frequency	= sample_rate;
	Params.DataPrecision	= (U8)data_precision;
	Params.NumChannels	= channels;
	
/*	ErrCode = STAUD_IPSetParams(Audio_p->AUDHndl, STAUD_OBJECT_INPUT_PCM1, &Params);*/
	ErrCode = STAUD_IPSetPCMParams(Audio_p->AUDHndl, STAUD_OBJECT_INPUT_CD0, &Params);
	
	if (ErrCode == ST_NO_ERROR)
	{
		DG_PRINTF((KERN_INFO "audio: <<< 0 = sraud_set_format()\n"));
	}
	else 
	{
		printk(KERN_ALERT  "audio: staud_set_format(): STAUD_IPSetParams() failure %x\n", ErrCode);
		printk(KERN_ALERT "audio: <<< -1 = staud_set_format()\n");
		return -1;
	}
	
	return 0;
}



/* stdoc ******************************************************************

   Name:        aud_set_format []
   
   Purpose:     
   
   Arguments:   <audio_device_t *dev>
                <audio_pcm_format_t *format>
   
   ReturnValue: (int) 0 on success, -1 on failure
   
   Errors:      
   
   Example:     
   
   SeeAlso:     
   
   Note:        
   
   Public:      No
   
   ************************************************************************ */

static int aud_set_format(AUDDVB_Device_t *dev, audio_pcm_format_t *format)
{
	int err_code = 0;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_set_format()\n"));
	
	err_code = staud_set_format(&dev ->STAUD_Device,
						format -> sample_rate,
						format -> data_precision,
						format -> channels);
	if (err_code != 0) 
	{
	        printk(KERN_ALERT  "audio: <<< aud_set_format(): staud_set_format() failure\n");
	        printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_set_format() Failure\n");
		return -EINTERNAL;
	}

	DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_set_format()\n"));
	return 0;
}


static int aud_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct audio_status	u_status;
	unsigned int		capability;
	int			res, ret_value;
	void *arg_p =		(void *) arg;
	audio_mixer_t		mixer;
	audio_karaoke_t		kar;
	AUDDVB_Device_t *dev = 	file->private_data;
	audio_pcm_format_t	u_format;
	
	DG_PRINTF((KERN_INFO "audio: >>> aud_ioctl()\n"));
	
	if ((file->f_flags & O_ACCMODE) == O_RDONLY) 
	{
		switch(cmd)
		{
			case AUDIO_GET_STATUS:
				
				DG_PRINTF((KERN_INFO "audio: >>> aud_get_status()\n"));
				
				u_status.AV_sync_state  = dev->audio_status.AV_sync_state;
				u_status.mute_state     = dev->audio_status.mute_state;
				u_status.play_state     = dev->audio_status.play_state;
				u_status.stream_source  = dev->audio_status.stream_source;
				u_status.channel_select = dev->audio_status.channel_select;
				u_status.bypass_mode    = dev->audio_status.bypass_mode;
				
				if(copy_to_user(arg_p, &u_status, sizeof(struct audio_status)))
				{
					printk(KERN_ALERT  "audio: <<< EFAULT = aud_ioctl() aud_get_status()\n");
					res = -EFAULT;
				}
				else 
				{
					DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_get_status()\n"));
					res = 0;
				}
				break;
			
			default:
				res = -EINVAL;
				break;
		}
	}
	else 
	{
		if (down_interruptible(&dev->audio_mutex)) 
		{
		    return -ERESTARTSYS;
		}
		    
		switch(cmd)
		{
			case AUDIO_PLAY:
				res = aud_play(dev);
				break;
			
			case AUDIO_STOP:
				res = aud_stop(dev);
				break;
			
			case AUDIO_PAUSE:
				res = aud_pause(dev);
				break;
			
			case AUDIO_CONTINUE:
				res = aud_continue(dev);
				break;
			
			case AUDIO_SELECT_SOURCE:
				res = aud_select_source(dev, (audio_stream_source_t) arg);
				break;
			
			case AUDIO_SET_MUTE:
				res = aud_set_mute(dev, (int) arg);
				break;
			
			case AUDIO_SET_AV_SYNC:
				res = aud_avsync(dev, (int) arg);
				break;
			
			case AUDIO_CHANNEL_SELECT:
				res = aud_channel_select(dev, (audio_channel_select_t) arg);
				break;
			
			case AUDIO_GET_STATUS:
				DG_PRINTF((KERN_INFO "audio: >>> aud_get_status()\n"));
				
				u_status.AV_sync_state  = dev->audio_status.AV_sync_state;
				u_status.mute_state     = dev->audio_status.mute_state;
				u_status.play_state     = dev->audio_status.play_state;
				u_status.stream_source  = dev->audio_status.stream_source;
				u_status.channel_select = dev->audio_status.channel_select;
				u_status.mixer_state    = dev->audio_status.mixer_state;
				u_status.bypass_mode    = dev->audio_status.bypass_mode;
				
				if(copy_to_user(arg_p, &u_status, sizeof(struct audio_status)))
				{
					printk(KERN_ALERT  "audio: =<<< EFAULT = aud_ioctl() aud_get_status()\n");
					res = -EFAULT;
				}
				else 
				{
					DG_PRINTF((KERN_INFO "audio: <<< 0 = aud_get_status()\n"));
					res = 0;
				}
				break;
			
			case AUDIO_GET_CAPABILITIES:
				ret_value = aud_get_capability(dev, &capability);
				if(ret_value!=0)
				{
					printk(KERN_ALERT  "audio: <<< EINTERNAL = aud_ioctl() aud_get_capability()\n");
					return -EINTERNAL;
				
				}
				if(copy_to_user(arg_p, &capability, sizeof(unsigned int)))
				{
					printk(KERN_ALERT  "audio: <<< EFAULT = aud_ioctl() aud_get_capability()\n");
					res = -EFAULT;
					return res;
				}
				res = 0;
				break;
			
			case AUDIO_SET_BYPASS_MODE:
				res = aud_set_bypass_mode(dev, (int) arg);
				break;
			
			case AUDIO_SET_ID:
				res = aud_set_id(dev, (int) arg);
				break;
			
			case AUDIO_SET_MIXER:
				if (copy_from_user(&mixer, (audio_mixer_t *) arg, sizeof(audio_mixer_t))) 
				{
					printk(KERN_ALERT  "audio: <<< EFAULT = aud_ioctl() aud_set_mixer()\n");
					res = -EFAULT;
				}
				res = aud_set_mixer(dev, &mixer);
				if (res == 0)
				{
				    dev->audio_status.mixer_state = mixer;
				}
				
				break;
			
			case AUDIO_CLEAR_BUFFER:
				if (dev->audio_status.play_state != AUDIO_STOPPED)
				{
					aud_stop(dev);
					res = aud_clear_buffer(dev);
					aud_play(dev);
				}
				else 
				{
					res = aud_clear_buffer(dev);
				}
				break;
			
			case AUDIO_SET_STREAMTYPE: 
				res = aud_set_streamtype(dev, arg);  
				break; 
			
			case AUDIO_SET_ATTRIBUTES:
				res = aud_set_attributes(dev, arg);
				break;
			
			case AUDIO_SET_EXT_ID:
				res = aud_set_ext_id(dev, arg);
				break;
			
			case AUDIO_SET_KARAOKE:
				if (copy_from_user(&kar, (audio_karaoke_t *) arg, sizeof(audio_karaoke_t))) 
				{
					printk(KERN_ALERT  "audio: <<< EFAULT = aud_ioctl() aud_set_karaoke()\n");
					res = -EFAULT;
				}
				res = aud_set_karaoke(dev, &kar);
				break;
			
			case AUDIO_PCM_SET_FORMAT:
				if(copy_from_user(&u_format, arg_p, sizeof(audio_pcm_format_t))) 
				{
					printk(KERN_ALERT  "audio: <<< EFAULT = aud_ioctl() aud_set_format()\n");
					res = -EFAULT;
				}
				else 
				{
					res = aud_set_format(dev, &u_format);
				}
				break;
			
			default:
				printk(KERN_ALERT  "audio: <<< EFAULT = aud_ioctl() default \n");
				res = -EINVAL;
				break;
			
		}
		
		up(&dev->audio_mutex);
	}
	
	return res;
	
} /* dvb_audio_ioctl */


static struct file_operations file_ops = {
	owner:       THIS_MODULE, 
	open:        aud_open, 
	release:     aud_close, 
	write:       aud_write, 
	read:        aud_read, 
	poll:        aud_poll, 
	ioctl:       aud_ioctl
};



/*For Debugging Purpose Use callback from audio driver */
static void AUDIO_Callback(STEVT_CallReason_t Reason, 
                         const ST_DeviceName_t RegistrantName, 
                         STEVT_EventConstant_t Event, 
                         const void *EventData, 
                         const void *SubscriberData_p)
{
/*	STAUD_SynchroUnit_t Synchro_clock_p;
	AUDDVB_Device_t *Audio_p = (AUDDVB_Device_t *) SubscriberData_p;*/
	
	static struct {
		int cdfifounderflow;
		int dataerr;
		int syncerr;
		int newframe;
		int fifooverflow;
	} cnts = {0,0,0,0,0};
	/*??? formalize above code better*/
	
	switch (Event) {
		case STAUD_LOW_DATA_LEVEL_EVT:
		{
			//CLKRV_NotifyAudioUnderflow(Audio_p->DeviceNum);
			//DG_PRINTF((KERN_INFO "audio: Event - Low data\n"));
		}
		break;
		
		case STAUD_CDFIFO_UNDERFLOW_EVT:
		{
			cnts.cdfifounderflow++;
			//DG_PRINTF((KERN_INFO "audio: Event - CD fifo underflow\n"));
			return;
		}
		break;
		
		case STAUD_FIFO_OVERF_EVT:
		{
			cnts.fifooverflow++;
			//DG_PRINTF((KERN_INFO "audio: Event - CD fifo overflow\n"));
			return;
		}
		break;
		
		case STAUD_DATA_ERROR_EVT:
		{
			cnts.dataerr++;
			//DG_PRINTF((KERN_INFO "audio: Event - Data error\n"));
			return;
		}
		break;
		
		case STAUD_SYNC_ERROR_EVT:
		{
			cnts.syncerr++;
			//DG_PRINTF((KERN_INFO "audio: Event - Sync error\n"));
		}
		break;
		
		case STAUD_PREPARED_EVT:
		{
			/* Currently, does not need any actions */
			//DG_PRINTF((KERN_INFO "audio: Event - Prepared\n"));
		}
		break;
	
		case STAUD_NEW_FRAME_EVT:
#if 0
		{
		    {
			STCLKRV_ExtendedSTC_t STC;
			STAUD_Event_t *Event_p = (STAUD_Event_t*)EventData;
			STAUD_PTS_t *PTS_p = (STAUD_PTS_t *)Event_p->EventData;
			
			cnts.newframe++;
			
			if ((CLKRV_GetSTC(0, &STC) == 0))
			{
			    if (PTS_p->Low != 0 && STC.BaseValue > PTS_p->Low)
			    {
				DG_PRINTF(("audio: late frame STC %d   PTS %d\n", STC.BaseValue, PTS_p->Low));
			    }
			}
		    }
		    
		    
		    //DG_PRINTF((KERN_INFO "audio: Event - New frame\n"));
		}
#endif
		break;

		case STAUD_PTS_EVT:
#if 0
		{
		    {
			STCLKRV_ExtendedSTC_t STC;
			STAUD_Event_t *Event_p = (STAUD_Event_t*)EventData;
			STAUD_PTS_t *PTS_p = (STAUD_PTS_t *)Event_p->EventData;
			
			cnts.newframe++;
			
			if ((CLKRV_GetSTC(0, &STC) == 0))
			{
			    if (PTS_p->Low != 0 && STC.BaseValue > PTS_p->Low)
			    {
				DG_PRINTF(("audio: late frame STC %d   PTS %d\n", STC.BaseValue, PTS_p->Low));
			    }
			}
		    }
		    
		    
		    DG_PRINTF((KERN_INFO "audio: Event - PTS\n"));
		}
#endif
		break;
	}
} /* audio event callback */


ST_ErrorCode_t AUDIO_GetBitBufferParams(int AudioDeviceNum, U8 **Base_p, int *RequiredSize_p){
	
	AUDDVB_Device_t *Audio_p = &audio_device[AudioDeviceNum];
	ST_ErrorCode_t Error;
	STAUD_BufferParams_t DataParams;
	
	DG_PRINTF((KERN_INFO "audio: >>> AUDIO_GetBitBufferParams()\n"));
	
	DG_PRINTF((KERN_INFO "audio: STAUD_IPGetInputBufferParams is called with the following params:\n"));
	DG_PRINTF((KERN_INFO "audio: \tDevice number = %d\n", AudioDeviceNum));
	DG_PRINTF((KERN_INFO "audio: \tHandle = 0x%x\n", Audio_p->AUDHndl));
	DG_PRINTF((KERN_INFO "audio: \tObject = 0x%x\n", STAUD_OBJECT_INPUT_CD0));
	
	Error = STAUD_IPGetInputBufferParams(Audio_p->AUDHndl,
					    STAUD_OBJECT_INPUT_CD0,
					     &DataParams);
	if(Error != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "audio: <<< AUDIO_GetBitBufferParams() Error %d!\n",Error);
		return Error;
	}
	
	DG_PRINTF((KERN_INFO "audio: STAUD_IPGetInputBufferParams returned 0x%x\n", Error));
	DG_PRINTF((KERN_INFO "audio: \tBase address = 0x%x\n", (int)DataParams.BufferBaseAddr_p));
	DG_PRINTF((KERN_INFO "audio: \tBuffer size = %d\n", DataParams.BufferSize));
	
	DG_PRINTF((KERN_INFO "audio: physical address = 0x%x\n", (int)DataParams.BufferBaseAddr_p));
	*Base_p = (U8*)(0xa0000000 | (unsigned int)DataParams.BufferBaseAddr_p);
	DG_PRINTF((KERN_INFO "audio: logical address = 0x%x\n", (int)(*Base_p)));
	*RequiredSize_p = DataParams.BufferSize;
	
	DG_PRINTF((KERN_INFO "audio: Input buffer address = 0x%x, size = %d\n", (int)*Base_p, *RequiredSize_p));
	DG_PRINTF((KERN_INFO "audio: <<< AUDIO_GetBitBufferParams() Success\n"));	
	
	return 0;
	
}



static ST_ErrorCode_t AUDIO_Stop(STAUD_Audio_t *AudioDev_p, STAUD_Stop_t StopMode, STAUD_Fade_t *Fade_p)
{
	ST_ErrorCode_t   ErrCode=0;

	DG_PRINTF((KERN_INFO "audio: >>> AUDIO_Stop()\n"));

	if (AudioDev_p->DecoderState == AUDDVB_PLAYING || AudioDev_p->DecoderState ==AUDDVB_PAUSED) 
	{
		ErrCode = STAUD_DRStop(AudioDev_p->AUDHndl, AudioDev_p->DecoderObject, StopMode, Fade_p);
		if (ErrCode == ST_NO_ERROR)
		{
			DG_PRINTF((KERN_INFO "audio: AUDIO_Stop(): ----- STAUDLX stopped\n"));
			AudioDev_p->DecoderState = AUDDVB_STOPPED;
			ErrCode = STAUD_DisableSynchronisation(AudioDev_p->AUDHndl);
		}
		else
		{
		    printk(KERN_ALERT "audio: STAUD_DRStop() failed with error %d\n", ErrCode);
		}
		
	}
	else 
	{
		DG_PRINTF((KERN_INFO "audio: AUDIO_Stop(): device already stopped\n"));
	}

	DG_PRINTF((KERN_INFO "audio: <<< %x =  AUDIO_Stop()\n", ErrCode));

	return ErrCode;
}



/* stdoc ******************************************************************

   Name:        AUDIO_Start []
   
   Purpose:     
   
   Arguments:   <AUD_Audio_t *Audio_p>
   
   ReturnValue: ( ST_ErrorCode_t )
   
   Errors:      
   
   Example:     
   
   SeeAlso:     
   
   Note:        
   
   Public:      No
   
   ************************************************************************ */

static ST_ErrorCode_t AUDIO_Start(STAUD_Audio_t *Audio_p)
{
	ST_ErrorCode_t   ErrCode=0;
	
	DG_PRINTF((KERN_INFO "audio: >>> AUDIO_Start()\n"));
	
	if (Audio_p->DecoderState == AUDDVB_STOPPED) 
	{
		ErrCode = STAUD_DRStart (Audio_p->AUDHndl, Audio_p->DecoderObject, &Audio_p->StreamParams);
		if (ErrCode == ST_NO_ERROR)
		{
			DG_PRINTF((KERN_INFO "audio: AUDIO_Start(): ----- STAUDLX started\n"));
			Audio_p->DecoderState = AUDDVB_PLAYING;
			if (Audio_p->Synchronised == TRUE) 
			{
				ErrCode = STAUD_EnableSynchronisation(Audio_p->AUDHndl);
			}
		}
		else 
		{
			printk(KERN_ALERT "audio: AUDIO_Start(): STAUD_DRStart failed with %d\n", ErrCode);
		}
	}
	else 
	{
		DG_PRINTF((KERN_INFO "audio: AUDIO_Start(): device already playing\n"));
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< %x =  AUDIO_Start()\n", ErrCode));
	
	return ErrCode;
}

/* stdoc ******************************************************************

   Name:        staud_stop []
   
   Purpose:     
   
   Arguments:   <aud_handle_t AudioHandle>
   
   ReturnValue: (int) 0 on success, -1 on failure
   
   Errors:      
   
   Example:     
   
   SeeAlso:     
   
   Note:        
   
   Public:      ???
   
   ************************************************************************ */

int staud_stop(AUDDVB_Device_t  * Audio_p)
{
	ST_ErrorCode_t  ErrCode;
	STAUD_Stop_t    StopMode;
	STAUD_Fade_t    Fade;
	int             Full;
	
	
	
	DG_PRINTF((KERN_INFO "audio: >>> staud_stop(...)\n"));
	
	
	
	StopMode = STAUD_STOP_NOW;  /* Default Setting */
	Fade.FadeType = STAUD_FADE_NONE;
	ErrCode = AUDIO_Stop(&Audio_p->STAUD_Device, StopMode, &Fade);
	
	if (ErrCode == ST_NO_ERROR) 
	{
		/*lock the shared buffer (against the write method)*/
		spin_lock(Audio_p->audio_write_buffer.lock);
		
		Full = (Audio_p->audio_write_buffer.size == (AUDDVB_SHARED_BUFFER_SIZE << 2));
		Audio_p->audio_write_buffer.size = 0;
		Audio_p->audio_write_buffer.head = 0;
		Audio_p->audio_write_buffer.tail = 0;
		
		/*unlock the shared buffer (against the write method)*/
		spin_unlock(Audio_p->audio_write_buffer.lock);
		if (Full) 
		{
			wake_up(&Audio_p->shared_buffer_notfull_queue);
			
		}
		
		DG_PRINTF((KERN_INFO "audio: %x =<<< aud_stop(...) Success\n", ErrCode));

		
		
	}
	else
	{
		printk(KERN_ALERT "audio: staud_stop(): AUDIO_Stop() failed with %d\n", ErrCode);
		printk(KERN_ALERT "audio: %x =<<< staud_stop(...) Failure\n", ErrCode);
	}
	
	
	
	return (int) ErrCode;
} /* aud_stop */



/* stdoc ******************************************************************

   Name:        aud_exit
   
   Purpose:     Terminates Sti710x Audio Decoder Initialization
   
   Arguments:   None
   
   ReturnValue: (int):  Error Code
   
   Errors:      ST_NO_ERROR:   No Errors
                Other Values:  Error condition
   
   Example:     None
   
   SeeAlso:     aud_open, aud_init
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_audio_term(int DeviceNum)
{
	ST_ErrorCode_t        ErrCode=0;
	
	AUDDVB_Device_t       *Audio_p = &audio_device[DeviceNum];
	STAUD_Audio_t		*STDevice_p =  &Audio_p->STAUD_Device; 
	DG_PRINTF((KERN_INFO "audio: >>> staud_audio_term()\n"));
	

	
	// DISP_UnregisterHDMISource(Audio_p->DeviceNum, DISP_HDMI_SOURCE_TYPE_AUDIO); Udit 
	
	if (STDevice_p->DecoderState == AUDDVB_PAUSED || STDevice_p->DecoderState == AUDDVB_PLAYING)
	{
		staud_stop(Audio_p);
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< Unsubscribing Audio events...\n"));
	/*STEVT_UnsubscribeDeviceEvent(EVTHndl[EVT_AUD], Audio_p->AUDDeviceName, STAUD_LOW_DATA_LEVEL_EVT);
	STEVT_UnsubscribeDeviceEvent(EVTHndl[EVT_AUD], Audio_p->AUDDeviceName, STAUD_DATA_ERROR_EVT);
	STEVT_UnsubscribeDeviceEvent(EVTHndl[EVT_AUD], Audio_p->AUDDeviceName, STAUD_CDFIFO_UNDERFLOW_EVT);Uk*/

/*	STEVT_UnsubscribeDeviceEvent(EVTHndl[EVT_AUD], Audio_p->AUDDeviceName, STAUD_FIFO_OVERF_EVT);
	STEVT_UnsubscribeDeviceEvent(EVTHndl[EVT_AUD], Audio_p->AUDDeviceName, STAUD_PREPARED_EVT);
	STEVT_UnsubscribeDeviceEvent(EVTHndl[EVT_AUD], Audio_p->AUDDeviceName, STAUD_SYNC_ERROR_EVT);*/
	/*STEVT_UnsubscribeDeviceEvent(EVTHndl[EVT_AUD], Audio_p->AUDDeviceName, STAUD_NEW_FRAME_EVT);*/
	/*STEVT_UnsubscribeDeviceEvent(EVTHndl[EVT_AUD], Audio_p->AUDDeviceName, STAUD_PTS_EVT);*/
	
	ErrCode=STAUD_Close(Audio_p->AUDHndl);
	
//	TermParams.ForceTerminate = TRUE;
//	ErrCode=STAUD_Term(STDevice_p->AUDDeviceName, &TermParams);
/*	Moved in WA */
	DVBCommand[DeviceNum][0] = 1 ; 
	STOS_SemaphoreSignal(LinuxDVBTaskSem_p); 
	
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "\naudio: %d=<<< staud_audio_term() Failure %d\n", ErrCode);
	}

	DG_PRINTF((KERN_INFO "audio: 0 <<< staud_audio_term()\n"));
	return 0;
	
} /* end AUD_Term */



/* stdoc ******************************************************************

   Name:        aud_set_streamtype 
   
   Purpose:     Set the Stream Type for Source Sti710x Audio Stream
   
   Arguments:   <int type>:  This value describe the type of the audio stream
                                
   ReturnValue: None
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

void staud_set_streamtype(STAUD_Audio_t  *Audio_p , int type)
{
	
	
	DG_PRINTF((KERN_INFO "audio: >>> staud_set_streamtype(...)\n"));
	
	Audio_p->DecoderObject = STAUD_OBJECT_DECODER_COMPRESSED0;
	Audio_p->InputObject = STAUD_OBJECT_INPUT_CD0;
	
	Audio_p->StreamParams.SamplingFrequency    = 44100;
	Audio_p->StreamParams.StreamID             = STAUD_IGNORE_ID;
	
	switch(type) 
	{
		case AUDDVB_CAP_DTS:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_DTS;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: DTS\n"));
			break;
		
		case AUDDVB_CAP_LPCM:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_LPCM;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: LPCM\n"));
			break;
		
		case AUDDVB_CAP_MPEG1:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_MPEG1;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: MPEG1\n"));
			break;
		
		case AUDDVB_CAP_MPEG2:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_MPEG2;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: MPEG2\n"));
			break;
		
		case AUDDVB_CAP_MP3:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_MP3;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: MP3\n"));
			break;
		
		case AUDDVB_CAP_MPEG_AAC:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_MPEG_AAC;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: AAC\n"));
			break;
		
		case AUDDVB_CAP_HE_AAC:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_HE_AAC;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: HE_AAC\n"));
			break;
		
		case AUDDVB_CAP_OGG:
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) NOT SUPPORTED\n"));
			break;
		
		case AUDDVB_CAP_SDDS:
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) NOT SUPPORTED\n"));
			break;
		
		case AUDDVB_CAP_AC3:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_AC3;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: AC3\n"));
			break;
		
		case AUDDVB_PES:
			Audio_p->StreamParams.StreamType = STAUD_STREAM_TYPE_PES;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) TYPE: PES\n"));
			break;
		
		case AUDDVB_ES:
			Audio_p->StreamParams.StreamType = STAUD_STREAM_TYPE_ES;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) TYPE: ES\n"));
			break;
		
		case AUDDVB_CAP_PCM:
			Audio_p->StreamParams.StreamContent = STAUD_STREAM_CONTENT_PCM;
			DG_PRINTF((KERN_INFO "audio: staud_set_streamtype(...) SOURCE: PCM\n"));
			break;
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< staaud_set_streamtype(...)\n"));
	
} /* aud_set_streamtype */





/* stdoc ******************************************************************

   Name:        aud_set_channel []
   
   Purpose:     
   
   Arguments:   <aud_handle_t AudioHandle>
                <int ChannelSelect>
   
   ReturnValue: (int) 0 on success, -1 on failure
   
   Errors:      
   
   Example:     
   
   SeeAlso:     
   
   Note:        
   
   Public:      ???
   
   ************************************************************************ */

int staud_set_channel(STAUD_Audio_t * Audio_p, int ChannelSelect)
{
        STAUD_StereoMode_t StereoMode;
	ST_ErrorCode_t ErrCode;
	
    
	DG_PRINTF((KERN_INFO "audio: >>> staud_set_channell()\n"));

	switch (ChannelSelect)
	{
		case AUDDVB_CHANNEL_SELECT_MODE_STEREO          :
		StereoMode = STAUD_STEREO_MODE_STEREO       ;
		break;

		case  AUDDVB_CHANNEL_SELECT_MODE_PROLOGIC        :
		StereoMode = STAUD_STEREO_MODE_PROLOGIC;
		break;

		case  AUDDVB_CHANNEL_SELECT_MODE_DUAL_LEFT       :
		StereoMode = STAUD_STEREO_MODE_DUAL_LEFT;
		break;

		case  AUDDVB_CHANNEL_SELECT_MODE_DUAL_RIGHT      :
		StereoMode = STAUD_STEREO_MODE_DUAL_RIGHT;
		break;

		case  AUDDVB_CHANNEL_SELECT_MODE_DUAL_MONO       :
		StereoMode = STAUD_STEREO_MODE_DUAL_MONO;
		break;

		case  AUDDVB_CHANNEL_SELECT_MODE_AUTO	      :
		StereoMode = STAUD_STEREO_MODE_AUTO;
		break;

		default:
		printk(KERN_ALERT "audio: staud_set_channel: unknown mode\n");
		printk(KERN_ALERT "audio: <<< -1 = staud_set_channel\n");
		return -1;
	}

	ErrCode = STAUD_DRSetStereoMode (Audio_p->AUDHndl,
					 Audio_p->DecoderObject,
					 StereoMode);

	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "audio: astud_set_channel: STAUD_DRSetStereoMode() returned error %s\n", ErrCode);
		printk(KERN_ALERT  "audio: <<< -1 = staud_set_channel\n");
		return -1;
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< 0 = staud_set_channel\n"));
	
	return 0;
	
} /* aud_set_channel */

/* stdoc ******************************************************************

   Name:        staud_audio_init [Sti710x Audio Decoder Init]
   
   Purpose:     Audio Decoder Initialization for STi710x device
   
   Arguments:   None
   
   ReturnValue: (int) Error Code
   
   Errors:      STAUD_ERROR_DECODER_RUNNING:     Decoding running
                STAUD_ERROR_DECODER_STOPPED:     Decoder stopped
                STAUD_ERROR_DECODER_PREPARING:   Decoder Already Initialized
                STAUD_ERROR_DECODER_PREPARED:    Parameters Error
                STAUD_ERROR_DECODER_PAUSING:     Decoder Pausing
                STAUD_ERROR_DECODER_NOT_PAUSING: Decoder Not Pausing mode
                STAUD_ERROR_INVALID_STREAM_ID:   Set invelid Stream Id.
                STAUD_ERROR_INVALID_STREAM_TYPE: Set Invelid Stream Type
                STAUD_ERROR_INVALID_DEVICE:      Invalid Audio Handle
                STAUD_ERROR_EVT_REGISTER:        Invalid Event handler
                STAUD_ERROR_EVT_UNREGISTER:      Invalid Event handler
                STAUD_ERROR_CLKRV_OPEN:          Clock recovery error
                STAUD_ERROR_CLKRV_CLOSE:         Clock recovery support closed
                STAUD_ERROR_AVSYNC_TASK:         AV Sync Error
                ST_NO_ERROR:                     No Error Condition
   
		Example:     None
   
   SeeAlso:     aud_close
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_audio_init(int AudioNum, aud_shared_buffer_t *audio_buffer, 
							ST_DeviceName_t Clock, ST_DeviceName_t Evt)
{
	ST_ErrorCode_t                 ErrCode;
	STAUD_InitParams_t             InitParams;
	STAUD_OpenParams_t             OpenParams;
	STEVT_DeviceSubscribeParams_t  EventParams;
	STAUD_TermParams_t             TermParams;
	U32                            SyncDelay = 0;	
	char                           devname[16];
	int 					Result;
	STAUD_DigitalOutputConfiguration_t DigitalOutParams = {0};
	AUDDVB_Device_t		* CurrentDriver; 
	STAUD_Audio_t		* CurrentDevice; 
	CurrentDevice = & audio_device[AudioNum].STAUD_Device; 
	CurrentDriver	= &audio_device[AudioNum]; 
	DG_PRINTF((KERN_INFO "audio: >>> staud_audio_init()\n"));
	
		
	sprintf(devname, AUDIO_DEVICE_NAME_PREFIX"%d", AudioNum);
	memset(&InitParams,0,sizeof(STAUD_InitParams_t));
	
	DG_PRINTF((KERN_INFO "audio: Device name is %s\n", devname));
	
	strcpy(CurrentDevice->AUDDeviceName, devname);
	CurrentDevice->Synchronised=FALSE;
	
	CurrentDevice->StreamParams.StreamType = STAUD_STREAM_TYPE_PES;
	CurrentDevice->DecoderState = AUDDVB_STOPPED;
	CurrentDevice->DeviceNum = AudioNum;


	InitParams.DeviceType		= STAUD_DEVICE_STI7100;
	InitParams.Configuration 	= STAUD_CONFIG_DVB_COMPACT;  
	InitParams.CPUPartition_p = (void*)0xFFFFFFFF;

	InitParams.InterruptNumber = 0;
	InitParams.InterruptLevel  = 0;
	strcpy(InitParams.RegistrantDeviceName, "audio");
	InitParams.MaxOpen = 1;

	strcpy(InitParams.EvtHandlerName, Evt);
	
	InitParams.AVMEMPartition = STAVMEM_GetPartitionHandle(0);

#if MSPP_SUPPORT
		InitParams.BufferPartition								= AVMEM_PartitionHandle[1];
		InitParams.AllocateFromEMBX								= FALSE;
#else //MSPP_SUPPORT
		InitParams.BufferPartition								= 0;
		InitParams.AllocateFromEMBX								= TRUE;
#endif //MSPP_SUPPORT


	/* Dacs CONFIG : */
	InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier	= 256;
	InitParams.DACClockToFsRatio = 256 ;
	InitParams.PCMOutParams.InvertWordClock	= FALSE;
	InitParams.PCMOutParams.Format		= STAUD_DAC_DATA_FORMAT_I2S;
	InitParams.PCMOutParams.InvertBitClock	= FALSE;
	InitParams.PCMOutParams.Precision	= STAUD_DAC_DATA_PRECISION_24BITS; /* 18 bit data */
	InitParams.PCMOutParams.Alignment	= STAUD_DAC_DATA_ALIGNMENT_LEFT;
	InitParams.PCMOutParams.MSBFirst	= TRUE; /* PCM data order MSB first*/
#ifndef NO_CLKRV
	strcpy(InitParams.ClockRecoveryName, Clock);
#else
	strcpy(InitParams.ClockRecoveryName, Clock);
#endif
	
	InitParams.PCMMode = PCM_ON;
	InitParams.SPDIFMode = STAUD_DIGITAL_MODE_NONCOMPRESSED;
	InitParams.NumChannels	= 2;
	
	
	/* Specific to STAUDLX */
	InitParams.SPDIFOutParams.AutoLatency		= TRUE;
	InitParams.SPDIFOutParams.AutoCategoryCode	= TRUE;
	InitParams.SPDIFOutParams.CategoryCode	        = 0;
	InitParams.SPDIFOutParams.AutoDTDI		= TRUE;
	InitParams.SPDIFOutParams.CopyPermitted		= STAUD_COPYRIGHT_MODE_NO_COPY;
	InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier= 128;
	InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode		= STAUD_SPDIF_DATA_PRECISION_24BITS;
	InitParams.SPDIFOutParams.Emphasis						= STAUD_SPDIF_EMPHASIS_CD_TYPE;

#ifdef USE_INTERNAL_DAC
	InitParams.DriverIndex				= 0;
#else
	InitParams.DriverIndex				= 0;
#endif
	
	DG_PRINTF((KERN_INFO "audio: Init parameters successfully filled at address 0x%x\n", &InitParams));
	
	ErrCode = STAUD_Init(CurrentDevice->AUDDeviceName, &InitParams);
	if (ErrCode != ST_NO_ERROR)
	{
		
		printk(KERN_ALERT  "audio: staud_audio_init: STAUD_Init() failed with error %d\n", ErrCode);
		printk(KERN_ALERT  "audio: -1 = <<< staud_audio_init()\n");
		return -1;
	}
	
	/* Open device */
	OpenParams.SyncDelay = SyncDelay;
	
	ErrCode = STAUD_Open(CurrentDevice->AUDDeviceName, &OpenParams, &CurrentDevice->AUDHndl);
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "audio: staud_audio_init: STAUD_Open() failed\n");
		TermParams.ForceTerminate = FALSE;
		STAUD_Term(CurrentDevice->AUDDeviceName, &TermParams);		
		printk(KERN_ALERT  "audio: -1 = <<< staud_audio_init()\n");
		return -1;
	}
	CurrentDriver->AUDHndl = CurrentDevice->AUDHndl;
	DG_PRINTF((KERN_INFO "audio: STAUD_Open returned 0x%x\n", ErrCode));
	
	if (0)
	{
		ErrCode = STAUD_IPSetLowDataLevelEvent(CurrentDevice->AUDHndl,
						       STAUD_OBJECT_INPUT_CD0,
						       AUDIO_BIT_BUFFER_LOW_LEVEL * 100 / AUDIO_BIT_BUFFER_SIZE);
	}
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT "audio: staud_audio_init(): STAUD_IPSetLowDataLevelEvent Error=%d\n",ErrCode);
	}
	
	DG_PRINTF((KERN_INFO "audio: After STAUD_IPSetLowDataLevelEvent\n"));
	
	
	
	ErrCode = AUDIO_GetBitBufferParams(CurrentDevice->DeviceNum, 
			/*(void** const)*/ &audio_buffer->buffer,
					   &audio_buffer->bufsize);
	
	DG_PRINTF((KERN_INFO "audio: AUDIO_GetBitBufferParams returned the following params:\n"));
	DG_PRINTF((KERN_INFO "audio: \tDeviceNum = %d\n", CurrentDevice->DeviceNum));
	DG_PRINTF((KERN_INFO "audio: \tbuffer = 0x%x\n", (int)CurrentDriver->audio_write_buffer.buffer));
	DG_PRINTF((KERN_INFO "audio: \tbufsize = %d\n", CurrentDriver->audio_write_buffer.bufsize));
	
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT "audio: AUDIO_GetBitBufferParams() failed\n");
		printk(KERN_ALERT  "audio: <<< %d = staud_audio_init()\n", ErrCode);
		return ErrCode;
	}
	
	DG_PRINTF((KERN_INFO "audio: AUDIO_GetBitBufferParams() Success\n"));
	
	/* Default is injection from Memory */
	CurrentDevice->InputSource = AUDDVB_SOURCE_MEMORY;
	

	/*To check Udit */
	if(CurrentDevice->InputSource == AUDDVB_SOURCE_DEMUX)
	{
		if (Result == 0)
		{
			/*Current Driver is Just Provided as Handle ,,, Nothing to do with Actual Stuff*/
			DG_PRINTF((KERN_INFO "audio: staud_audio_init(): switching to DEMUX interface\n"));
			ErrCode = STAUD_IPSetDataInputInterface(CurrentDevice->AUDHndl,
								STAUD_OBJECT_INPUT_CD0,
								NULL,
								NULL,
								(void * const)CurrentDriver);
		}
		else
		{
			/*demux not ready to send any data - switch to be postponed when video feed is setup */
			DG_PRINTF((KERN_INFO "audio: staud_audio_init(): switching to DUMMY interface\n"));
			ErrCode = STAUD_IPSetDataInputInterface(CurrentDevice->AUDHndl,
								STAUD_OBJECT_INPUT_CD0,
								NULL,
								NULL,
								(void * const)CurrentDriver);
		}
	}
	else 
	{
		DG_PRINTF((KERN_INFO "audio: staud_audio_init(): switching to MEMORY interface\n"));
		ErrCode = STAUD_IPSetDataInputInterface(CurrentDevice->AUDHndl,
								STAUD_OBJECT_INPUT_CD0,
								AUDIO_GetWriteAddress,
								AUDIO_InformReadAddress,
								(void * const)CurrentDriver);
		
	}
	
	/* Subscribing Audio Event */
	EventParams.NotifyCallback     = AUDIO_Callback;
	EventParams.SubscriberData_p   = CurrentDriver;
	
	/*ErrCode = STEVT_SubscribeDeviceEvent(EVTHndl[EVT_AUD], 
					      Audio_p->AUDDeviceName,
					      STAUD_DATA_ERROR_EVT, 
					      &EventParams);
	ErrCode |= STEVT_SubscribeDeviceEvent(EVTHndl[EVT_AUD], 
					      Audio_p->AUDDeviceName,
					      STAUD_CDFIFO_UNDERFLOW_EVT, 
					      &EventParams);
	ErrCode |= STEVT_SubscribeDeviceEvent(EVTHndl[EVT_AUD], 
					     Audio_p->AUDDeviceName,
					     STAUD_LOW_DATA_LEVEL_EVT, 
					     &EventParams); Uk*/
/*	ErrCode |= STEVT_SubscribeDeviceEvent(EVTHndl[EVT_AUD], 
					      Audio_p->AUDDeviceName,
					      STAUD_FIFO_OVERF_EVT, 
					      &EventParams);
	ErrCode |= STEVT_SubscribeDeviceEvent(EVTHndl[EVT_AUD], 
					      Audio_p->AUDDeviceName,
					      STAUD_SYNC_ERROR_EVT, 
					      &EventParams);
	ErrCode |= STEVT_SubscribeDeviceEvent(EVTHndl[EVT_AUD], 
					      Audio_p->AUDDeviceName,
					      STAUD_PREPARED_EVT, 
					      &EventParams);*/
	/*ErrCode |= STEVT_SubscribeDeviceEvent(EVTHndl[EVT_AUD], 
					      Audio_p->AUDDeviceName,
					      STAUD_NEW_FRAME_EVT, 
					      &EventParams);*/
	/*ErrCode |= STEVT_SubscribeDeviceEvent(EVTHndl[EVT_AUD], 
					      Audio_p->AUDDeviceName,
					      STAUD_PTS_EVT, 
					      &EventParams);*/
	
	if (ErrCode != ST_NO_ERROR) 
	{
		printk(KERN_ALERT  "audio: staud_audio_init(): event subscription failed Terminating Audio Error=%d\n",ErrCode);
		staud_audio_term(CurrentDevice->DeviceNum);		
		printk(KERN_ALERT  "audio: -1 = <<< staud_audio_init()\n");
		return -1;
	}
	
	/* Set the audio init default parameters */
	/*Default Params */
	staud_set_streamtype(CurrentDevice, AUDDVB_PES);
	
	staud_set_streamtype(CurrentDevice, AUDDVB_CAP_MPEG2);
	
	ErrCode = staud_set_channel(CurrentDevice, AUDDVB_CHANNEL_SELECT_MODE_AUTO);
	if (ErrCode != ST_NO_ERROR)
	{
		printk(KERN_ALERT  "audio: staud_audio_init: aud_set_channel failed...\n");
	}
	
	/* Initial state sync disabled: */            
	/* AUDIO_DisableSynchronization(Audio_p); */
	


	DigitalOutParams.DigitalMode = STAUD_DIGITAL_MODE_NONCOMPRESSED;
	
	ErrCode = STAUD_SetDigitalOutput(CurrentDevice->AUDHndl, DigitalOutParams);
	if (ErrCode != ST_NO_ERROR) {
		DG_PRINTF((KERN_ERR "audio: aud_init(): STAUD_SetDigitalOutput() failed with error %d\n", ErrCode));
		DG_PRINTF((KERN_ERR "audio: -1 = <<< staud_audio_init()\n"));
		staud_audio_term(CurrentDevice->DeviceNum);
		return -1;
	}
	
	/*ErrCode = STAUD_OPEnableHDMIOutput(CurrentDevice->AUDHndl, STAUD_DIGITAL_OUTPUT_OBJECT);
	
	if (ErrCode != ST_NO_ERROR) {
		DG_PRINTF((KERN_ERR "audio: aud_init(): STAUD_OPEnableHDMIOutput() failed with error %s\n", STAPI_Error(ErrCode)));
		DG_PRINTF((KERN_ERR "audio: -1 = <<< staud_audio_init()\n"));
		staud_audio_term(CurrentDevice->DeviceNum);
		return -1;
	}*/
	

	
	DG_PRINTF((KERN_INFO "audio: 0 = <<< staud_audio_init()\n"));
	
	return 0;
} /* end aud_init */



/* stdoc ******************************************************************

   Name:        aud_start_decoding()
   
   Purpose:     Start Sti710x Audio Decoding 
   
   Arguments:   None
   
   ReturnValue: (int):          Error Code
   
   Errors:      ST_NO_ERROR:    No Errors
                Other values:   Error condition
   
   Example:     None 
   
   SeeAlso:     aud_stop
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

int staud_start_decoding(STAUD_Audio_t *Audio_p)
{
	ST_ErrorCode_t    ErrCode;

	
	DG_PRINTF((KERN_INFO "audio: >>> staud_start_decoding()\n"));
	
/*	aud_mute(AudioHandle);*/
	
	DG_PRINTF((KERN_INFO "audio: staud_start_decoding() Frequency: %d\n", Audio_p->StreamParams.SamplingFrequency));
	DG_PRINTF((KERN_INFO "audio: staud_start_decoding() StreamID: %d\n", Audio_p->StreamParams.StreamID));
	DG_PRINTF((KERN_INFO "audio: staud_start_decoding() StreamType: %d\n", Audio_p->StreamParams.StreamType));
	DG_PRINTF((KERN_INFO "audio: staud_start_decoding() StreamContents: %d\n", Audio_p->StreamParams.StreamContent));
	
	ErrCode = AUDIO_Start (Audio_p);
	if (ErrCode != ST_NO_ERROR) 
	{
		printk(KERN_ALERT  "audio: staud_start_decoding(): AUDIO_Start() failure %x\n", ErrCode);
		printk(KERN_ALERT  "audio: <<< -1 = staud_start_decoding()\n");
		return -1;
	}

	

	return 0;
	
} /* audio_start_decoding */

int  aud_audio_init(int i, ST_DeviceName_t Clock, ST_DeviceName_t Evt )
{
	int            err_code = 0;
	int            return_value = 0;
	AUDDVB_Device_t *dev = &audio_device[i];
	
	DG_PRINTF((KERN_ERR "audio: >>> aud_audio_init()\n"));
	
	dev->audio_write_buffer.head = 0;
	dev->audio_write_buffer.tail = 0;
	dev->audio_write_buffer.size = 0;
	spin_lock_init(dev->audio_write_buffer.lock);
	
	init_waitqueue_head(&dev->shared_buffer_notfull_queue);
	
	 
	
	DG_PRINTF((KERN_INFO "audio: head %d\n", dev->audio_write_buffer.head));
	DG_PRINTF((KERN_INFO "audio: tail %d\n", dev->audio_write_buffer.tail));
	DG_PRINTF((KERN_INFO "audio: size %d\n", dev->audio_write_buffer.size));
	
	sema_init(&dev->audio_mutex, 1);
	
	if (return_value != 0) 
	{
		DG_PRINTF((KERN_ERR "audio: <<< -EIO = aud_audio_init()\n"));
		return -EIO;
	}
	        
	err_code = staud_audio_init(i, &dev->audio_write_buffer, Clock, Evt);


	
	
	DG_PRINTF((KERN_INFO "audio: buffer located at 0x%x\n", (int)dev->audio_write_buffer.buffer));
	DG_PRINTF((KERN_INFO "audio: buffer size = %d\n", dev->audio_write_buffer.bufsize));
	
	if (err_code == 0) 
	{    
		dev->mixer_default.volume_left  = DEFAULT_VOLUME_LEFT;
		dev->mixer_default.volume_right = DEFAULT_VOLUME_RIGHT;
		
		return_value = staud_enablesync(&dev->STAUD_Device);
		
		if (return_value == 0) 
		{
			dev->audio_status.AV_sync_state = 1;
		}
		else 
		{
			dev->audio_status.AV_sync_state = 0;
		}
		
		/* Default Audio Decoder Setting */
		dev->audio_status.play_state     = AUDIO_STOPPED;
		dev->audio_status.channel_select = AUDIO_AUTO;
		dev->audio_status.stream_source  = AUDIO_SOURCE_DEMUX;
		dev->audio_status.bypass_mode    = 1; /* == not bypassing */
		dev->audio_status.mute_state     = 0;
		
		printk(KERN_INFO "audio: aud_audio_init : audio DVB device initialized\n");
	} 
	else 
	{
		printk(KERN_ALERT  "audio: <<< -EIO = aud_audio_init()\n");
		return -EIO;
	}
	
	/*err_code = aud_play(dev);
	
	printing error msg only*/
	if (err_code != 0) 
	{
	        printk(KERN_ALERT  "audio: aud_audio_init(): aud_play() failed\n");
	}
	
	
	DG_PRINTF((KERN_ERR "audio: <<< 0 = aud_audio_init()\n"));
	
	return 0;
}



/* stdoc ******************************************************************

   Name:        aud_audio_term []
   
   Purpose:     
   
   Arguments:   <int i>
   
   ReturnValue: (void __exit )
   
   Errors:      
   
   Example:     
   
   SeeAlso:     
   
   Note:        
   
   Public:      ???
   
   ************************************************************************ */

void  aud_audio_term(int i)
{
	int err_code;
	AUDDVB_Device_t *dev = &audio_device[i];

	DG_PRINTF((KERN_ERR "audio: >>> aud_audio_term()\n"));
	err_code = staud_audio_term(i);
	if (err_code != 0) 
	{
		printk(KERN_ALERT  "audio: aud_audio_term() staud_audio_term failed\n");
	}
	
	dev->audio_status.play_state = AUDIO_STOPPED;	
	dev->Initailized	= FALSE; 
		
	if (err_code != 0) 
	{
		printk(KERN_ALERT  "audio: aud_audio_term(): aud_unregister() failed\n");
	}
	
	DG_PRINTF((KERN_INFO "audio: <<< aud_audio_term()\n"));
	
	printk(KERN_INFO "audio: aud_audio_term audio DVB device deinitialized\n");
}



/* stdoc ******************************************************************

   Name:        aud_term
   
   Purpose:     Deinitializations of the Sti710x Audio Decoder
   
   Arguments:   None
   
   ReturnValue: None
   
   Errors:      None
   
   Example:     None
   
   SeeAlso:     aud_init
   
   Note:        None
   
   Public:      Yes
   
   ************************************************************************ */

void __exit aud_term(void)
{
	
	
	DG_PRINTF((KERN_DEBUG "audio: >>> module_exit_ aud_term()\n"));
	
	TaskTerminateCommand = TRUE; 
	STOS_SemaphoreSignal(LinuxDVBTaskSem_p); 

	STOS_TaskDelayUs(100);
	/*Give Time task to work */

	cdev_del(staudlx_dvb_cdev);
                                              
    /* Unregister the module */
    	unregister_chrdev_region(MKDEV(major, 0), MAX_AUDIO_DEVICES);

	 STOS_TaskDelete(LinuxDVBTask_p,NULL,NULL,NULL);
	 LinuxDVBTask_p = NULL; 
	
	DG_PRINTF((KERN_DEBUG "audio: <<<< module_exit_  (aud_term) \n"));
	

} /* aud_term */

int __init aud_init(void)
{

	int i, err_code=0;
	dev_t base_dev_no;

    	SET_MODULE_OWNER(&file_ops);
	
	DG_PRINTF((KERN_INFO "audio: >>> module_init()\n"));

#ifdef ST_7100	
	audio_max_devices = 1;	
#endif 

	for(i=0;i<audio_max_devices;i++)
	{
		audio_device[i].Initailized = FALSE;
		audio_device[i].users   = 0;
	}
	

#if 0	
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	err_code = devfs_register_chrdev(MAJOR_NUMBER, DEVICE_NAME_PREFIX, &file_ops);
	#else
	err_code = register_chrdev(MAJOR_NUMBER, DEVICE_NAME_PREFIX, &file_ops);
	#endif
	/*
	     * Register the major number. If major = 0 then a major number is auto
	     * allocated. The allocated number is returned.
	     * The major number can be seen in user space in '/proc/devices'
	     */
#else 
	    if (major == 0) {

	        if (alloc_chrdev_region(&base_dev_no, 0, MAX_AUDIO_DEVICES, DEVICE_NAME_PREFIX)) {
	    
	            err_code = -EBUSY;
	            printk(KERN_ALERT "No major numbers for staudlx_dvb by %s (pid %i)\n", current->comm, current->pid);
	           goto clean_up;
	        }

	        major = MAJOR(base_dev_no);
	    }
	    else
	    {
	    	base_dev_no = MKDEV(major, 0);
	        
	        if (register_chrdev_region(base_dev_no, MAX_AUDIO_DEVICES, DEVICE_NAME_PREFIX)) {
	        
	            err_code = -EBUSY;
	            printk(KERN_ALERT "No major numbers for staudlx_dvb by %s (pid %i)\n", current->comm, current->pid);
	           goto clean_up;;
	        }
	    }

	    if (NULL == (staudlx_dvb_cdev = cdev_alloc())) {
	    
	         err_code = -EBUSY;
	         printk(KERN_ALERT "No major numbers for staudlx_dvb by %s (pid %i)\n", current->comm, current->pid);
		  unregister_chrdev_region(MKDEV(major, 0), MAX_AUDIO_DEVICES);
	        goto clean_up;
	    }

	    staudlx_dvb_cdev->owner = THIS_MODULE;
	    staudlx_dvb_cdev->ops   = &file_ops;
	    kobject_set_name(&(staudlx_dvb_cdev->kobj), "DEVICE_NAME_PREFIX%d", 0);


	    /*** Register any register regions ***/                  


	    /*** Do the work ***/

	    /* Appears in /var/log/syslog */
	    printk(KERN_ALERT "Load module staudlx_dvb [%d] by %s (pid %i)\n", major, current->comm, current->pid);


	    /*** Register the device nodes for this module ***/

	    /* Add the char device structure for this module */

	    if (cdev_add(staudlx_dvb_cdev, base_dev_no, MAX_AUDIO_DEVICES)) {    
	        	printk(KERN_ALERT "Failed to add a charactor device for staudlx_dvb by %s (pid %i)\n", current->comm, current->pid);
		kobject_put(&(staudlx_dvb_cdev->kobj));
		 unregister_chrdev_region(MKDEV(major, 0), MAX_AUDIO_DEVICES);
		 err_code= -EBUSY;
	       goto clean_up;
	    }
#endif 		
		
	if(LinuxDVBTask_p==NULL)
	{

		LinuxDVBTaskSem_p = STOS_SemaphoreCreateFifo(NULL,0);	
		if(LinuxDVBTaskSem_p)
		{
			err_code= STOS_TaskCreate(LinuxDVBTask,NULL,NULL,16*1024,NULL,NULL,
                                        &LinuxDVBTask_p,NULL,10, "LinuxDVBTask", 0);
			if(err_code)
			{
				STOS_SemaphoreDelete(NULL,LinuxDVBTaskSem_p); 
				printk(KERN_ALERT "audio: <<< EINTERNAL Module_Init No Memory Task  \n");
				return -EINTERNAL; 
				
			}
		}
		else 
		{
			printk(KERN_ALERT "audio: <<< EINTERNAL Module_Init No Memory Semaphore \n");
			return -EINTERNAL; 
		}
		
		
	}
	
clean_up: 	
	if (err_code < 0) {
		DG_PRINTF((KERN_ERR "audio: module_init():  registration failure %x\n", err_code));
		
				printk(KERN_ALERT "Audio: <<< -EIO = register_chrdev() \n" );
		return -EIO;
		
	}

	printk(KERN_INFO "audio: dvb module init: audio device registration: done!\n");
		
	DG_PRINTF((KERN_INFO "audio: <<< 0 = dvb  aud module init() \n"));
		
	return 0;   
}

ST_ErrorCode_t AUDIO_SetInterface(U32 DeviceNum, GetWriteAddress_t  PTIWritePointerFunction,
	InformReadAddress_t  PTIReadPointerFunction, U32 PTIHandle)
{
	AUDDVB_Device_t  *DVBDevice ;
	if(DeviceNum>MAX_AUDIO_DEVICES)
	{
		return ST_ERROR_BAD_PARAMETER;
	}
	DVBDevice = &audio_device[DeviceNum];
	
	return( STAUD_IPSetDataInputInterface(DVBDevice->AUDHndl,
								STAUD_OBJECT_INPUT_CD0,
								PTIWritePointerFunction,
								PTIReadPointerFunction,
								(void * const)PTIHandle));
	
}

EXPORT_SYMBOL(AUDIO_GetBitBufferParams);
EXPORT_SYMBOL(AUDIO_SetInterface);
module_init(aud_init);
module_exit(aud_term);

