/*
 *  ALSA Driver on the top of STAUDLX
 *  Copyright (c)   (c) 2006 STMicroelectronics Limited
 *
 *  *  Authors:  Udit Kumar <udit-dlh.kumar@st.com>
 *
 *   This program is interface between STAUDLX and ALSA application. 
 * For ALSA application, this will serve as ALSA driver but at the lower layer, it will map ALSA lib calls to STAULDX calls
 */

#include <sound/driver.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <linux/config.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/proc_fs.h>


#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#define SNDRV_GET_ID
#include <sound/initval.h>

#include <sound/asoundef.h>



#include <asm/dma.h>
#include "st_pcm.h"

static int index[SNDRV_CARDS] = {SNDRV_DEFAULT_IDX1,SNDRV_DEFAULT_IDX1,SNDRV_DEFAULT_IDX1,SNDRV_DEFAULT_IDX1};
        	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = {SNDRV_DEFAULT_STR1,SNDRV_DEFAULT_STR1,SNDRV_DEFAULT_STR1,SNDRV_DEFAULT_STR1};	/* ID for this card */





#if defined (CONFIG_CPU_SUBTYPE_STB7100)

#define SND_DRV_CARDS  4

static stm_snd_output_device_t  card_list[SND_DRV_CARDS]= {
        /*major                      minor             input type          output type          */
        {PCM0_DEVICE,               MAIN_DEVICE, STM_DATA_TYPE_LPCM,     STM_DATA_TYPE_LPCM},
        {PCM1_DEVICE,               MAIN_DEVICE, STM_DATA_TYPE_LPCM,     STM_DATA_TYPE_LPCM},
        {SPDIF_DEVICE,              MAIN_DEVICE, STM_DATA_TYPE_IEC60958, STM_DATA_TYPE_IEC60958},
        {CAPTURE_DEVICE, 	MAIN_DEVICE, STM_DATA_TYPE_LPCM,     STM_DATA_TYPE_LPCM}
};

#include "stb7100_snd.h"
#include "stm7100_pcm.c"
#include "stb7100_spdif.c"
#define DEVICE_NAME "STb7100"


#else 

#error "BAD cpu arhitecture defined - PCM player is not supported"

#endif

MODULE_AUTHOR("Udit Kumar <udit-dlh.kumar@st.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DEVICE_NAME " ALSA driver");
MODULE_SUPPORTED_DEVICE("{{STM," DEVICE_NAME "}}");

static int snd_pcm_playback_hwfree(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	DEBUG_PRINT(("ALSA Core : >>>snd_pcm_playback_hwfree \n  "));
        chip->card_data->in_use = 0;
	
	if(runtime->dma_area == NULL)
	{
		DEBUG_PRINT(("ALSA Core : <<< snd_pcm_playback_hwfree null Ptr \n  "));
		return 0;
	}

	runtime->dma_area    = 0;
	runtime->dma_addr    = 0;
	runtime->dma_bytes   = 0;
		DEBUG_PRINT(("ALSA Core : <<< snd_pcm_playback_hwfree Bug Phy Area \n  "));
	return 0;
}

static snd_pcm_uframes_t snd_pcm_playback_pointer(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
//	DEBUG_PRINT((" ALSA Core  : pointer CB \n"));
	
	return chip->playback_ops->playback_pointer(substream);
}


static int snd_pcm_playback_prepare(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	/*unsigned long flags=0;*/
	/* Chip isn't running at this point so we don't have to disable interrupts*/
//	spin_lock_irqsave(&chip->lock,flags);
	DEBUG_PRINT(("ALSA Core : >>> snd_pcm_playback_prepare  \n  "));
#if 0
	/*
	 * On the STb7100 we can only use either the PCM0 device or the protocol
	 * converter as they physically use the same hardware. As we have no
	 * device specific hook for prepare, we do this here for the moment.
	 */
	if((card_list[PCM0_DEVICE].in_use               && (chip->card_data->major == PROTOCOL_CONVERTER_DEVICE)) ||
	   (card_list[PROTOCOL_CONVERTER_DEVICE].in_use && (chip->card_data->major == PCM0_DEVICE)))
	{
		int converter_enable = (chip->card_data->major==PROTOCOL_CONVERTER_DEVICE ? 1:0);
		printk("%s: device (%d,%d) is in use by (%d,%d)\n",
                	__FUNCTION__,
                	chip->card_data->major,
                	chip->card_data->minor,
                	(converter_enable ? 	PCM0_DEVICE:
                				PROTOCOL_CONVERTER_DEVICE),
                	(converter_enable ? 	card_list[PCM0_DEVICE].minor:
                				card_list[PROTOCOL_CONVERTER_DEVICE].minor));

        	return -EBUSY;
        }
#endif

	chip->card_data->in_use = 1;
	//spin_unlock_irqrestore(&chip->lock,flags);

	if(chip->playback_ops->program_hw(substream) < 0)
		return -EIO;

	DEBUG_PRINT(("ALSA Core : <<< snd_pcm_playback_prepare  \n  "));
	return 0;
}


static int snd_pcm_dev_free(snd_device_t *dev)
{
	pcm_hw_t *snd_card = dev->device_data;

	DEBUG_PRINT(("ALSA Core >>>snd_pcm_dev_free(dev = 0x%08lx)\n",dev));
	DEBUG_PRINT((">>> snd_card = 0x%08lx\n",snd_card));


	if(snd_card->playback_ops->free_device)
		return snd_card->playback_ops->free_device(snd_card);
	DEBUG_PRINT(("ALSA Core <<< snd_pcm_dev_free(dev = 0x%08lx)\n",dev));
	return 0;
}


static int snd_playback_trigger(snd_pcm_substream_t * substream, int cmd)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	DEBUG_PRINT(("ALSA Core : <<< snd_playback_trigger  \n  "));
	switch(cmd)
	{
		case SNDRV_PCM_TRIGGER_START:
			DEBUG_PRINT(("ALSA Core :  snd_playback_trigger Start \n  "));
			
			chip->State = STAUD_ALSA_START;
			chip->NumberOfBytesCopied = 0;
			chip->NumberUsed =0;
			chip->playback_ops->start_playback(substream);
			DEBUG_PRINT(("  Debug snd_playback_trigger_started \n"));
			
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			DEBUG_PRINT(("ALSA Core :  snd_playback_trigger Stop \n  "));
			//if (chip->State == STAUD_ALSA_STOP)
				//return 0; //  Debug above two lines 
			
			 chip->State = STAUD_ALSA_STOP;
			 chip->playback_ops->stop_playback(substream);
			 chip->card_data->in_use = 0;
			 chip->NumberOfBytesCopied = 0;
			 chip->NumberUsed =0;
			 chip->PrevReadPointer=chip->BaseAddress;		
			 chip->ReadPointer = chip->BaseAddress;
			 DEBUG_PRINT(("  Debug snd_playback_trigger_stopped \n"));
			chip->Copy = FALSE; 
			 // Reset the Write Pointer  ---  
			chip->WritePointer = (U8 *)substream->runtime->dma_addr;
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			DEBUG_PRINT(("ALSA Core :  snd_playback_trigger Pause \n  "));
			chip->playback_ops->pause_playback(substream);
			DEBUG_PRINT(("  Debug snd_playback_trigger_pause \n"));
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			DEBUG_PRINT(("ALSA Core :  snd_playback_trigger resume \n  "));
			chip->playback_ops->unpause_playback(substream);
			DEBUG_PRINT(("  Debug snd_playback_trigger_resume \n"));
			break;
		default:
			return -EINVAL;
	}
	snd_pcm_trigger_done(substream,substream);	
	DEBUG_PRINT(("ALSA Core :>>> snd_playback_trigger  \n  "));
	return 0;	
}


static int snd_pcm_playback_close(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	U32 Error=0;

	DEBUG_PRINT(("ALSA Core : >>> snd_pcm_playback_close(substream = 0x%08lx)\n",substream));
	DEBUG_PRINT((">>> chip = 0x%08lx\n",chip));

	/*
	 * If the PCM clocks are programmed then ensure the playback is
	 * stopped. If not do nothing otherwise we can end up with the
	 * DAC in a bad state.
	 */
	 if(chip->State == STAUD_ALSA_START)
	 {
		if(chip->are_clocks_active)
			chip->playback_ops->stop_playback(substream);  
	 }
	chip->are_clocks_active = 0;
	Error = STAUD_Close(chip->AudioHandle);
	if(Error!=0)
	{
		printk(KERN_ALERT "STAUD_Close Failed Error code = %d and Device id is = %d \n",Error,chip->card_data->major);
		
		return -1;
	}


	chip->current_substream = 0;

	switch(chip->card_data->major)
	{
		case PCM0_DEVICE:
			DEBUG_PRINT(( "Closing PCMP0 \n "));
			ALSACommand[0][0] = 1;
			break;

		case PCM1_DEVICE:
			DEBUG_PRINT((  "Closing PCMP1 \n "));
			ALSACommand[1][0] = 1;
			break;

		case SPDIF_DEVICE:
			DEBUG_PRINT((  "Closing SPDIF \n "));
			ALSACommand[2][0] = 1;
			break;

	}
	STOS_SemaphoreSignal(alsaPlayerTaskSem_p);
	DEBUG_PRINT(("ALSA Core : <<< snd_pcm_playback_close(substream = 0x%08lx)\n",substream));
	return 0;
}

static unsigned int  STAPIReadPointerFunction(void * const handle, void* const address)
{
	if(handle)
	{
		pcm_hw_t *chip = (pcm_hw_t *)handle;
		U32 SizeUsed =0;
		//DEBUG_PRINT((" Update Read Pointer to =%x \n", (U32)(address)));
		//printk(KERN_ALERT " Update Read Pointer to =%x \n", (U32)(address));
//		spin_lock(&chip->lock);
		chip->ReadPointer = address; 
		if(chip->PrevReadPointer > chip->ReadPointer)
		{
			SizeUsed = 	chip->SizeOfBuffer -(chip->PrevReadPointer -chip->ReadPointer);
		}
		else 
			SizeUsed = chip->PrevReadPointer -chip->ReadPointer; 

		chip->NumberUsed +=SizeUsed;
		
		if (chip->State == STAUD_ALSA_START)
			snd_pcm_period_elapsed(chip->current_substream);
	
//		spin_unlock(&chip->lock);
	}
	else 
	{
		return -1;
	}

	
	return 0;
}

static unsigned int  STAPIWritePointerFunction(void * const handle, void** const address_p)
{
	if(handle)
	{
		pcm_hw_t *chip = (pcm_hw_t *)handle;
		//DEBUG_PRINT(("Give Write Pointer to =%x \n", (U32)(chip->WritePointer)));
//		spin_lock(&chip->lock);
	/*We are not having Copy callback 
		APLAY mono stream requirement */
		if(!(chip->Copy)) 
		{
			if(chip->ReadPointer == chip->BaseAddress)
			{
				*address_p  = (((void *)chip->ReadPointer  -1)); 
			}
			else 
			{
				*address_p  = (void *)chip->TopAddress; 
			}
			return 0; 
		}
		*address_p = (void *)chip->WritePointer; 
		if((chip->NumberOfBytesCopied != chip->NumberUsed )&& (chip->WritePointer == chip->ReadPointer))
		{
			*address_p -=1;
			if((U8 *)*address_p < chip->BaseAddress)
				*address_p = chip->TopAddress;
		}
//		spin_unlock(&chip->lock);
	}
	else 
	{
		return -1;
	}
	
	return 0;
}

static int snd_pcm_playback_hwparams(snd_pcm_substream_t * substream,
					 snd_pcm_hw_params_t * hw_params)
{
	int   err  = 0;
	U32 size=0;
	
	U32 Error=0;
	STAUD_BufferParams_t BufferParams;
	snd_pcm_runtime_t *runtime = substream->runtime;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	DEBUG_PRINT(("ALSA Core :>>> snd_pcm_playback_hwparams \n  "));

	size = params_buffer_bytes(hw_params);
	DEBUG_PRINT(("ALSA Core : snd_pcm_playback_hwparams Size =%d \n  ",size));

	if(chip->card_data->major == PCM0_DEVICE)
	{
		Error = STAUD_IPGetInputBufferParams(chip->AudioHandle,STAUD_OBJECT_INPUT_CD0,&BufferParams);
	}
	else if (chip->card_data->major == PCM1_DEVICE)
	{
		Error = STAUD_IPGetInputBufferParams(chip->AudioHandle,STAUD_OBJECT_INPUT_CD1,&BufferParams);
	}
	else if (chip->card_data->major == SPDIF_DEVICE)
	{
		Error = STAUD_IPGetInputBufferParams(chip->AudioHandle,STAUD_OBJECT_INPUT_CD2,&BufferParams);
	}
	if(Error!=0)
	{
		printk( KERN_ALERT "STAUD_IPGetInputBufferParams Error = %x and Device ID is %d \n",Error, chip->card_data->major);
	}
	else 
	{
		DEBUG_PRINT(("ALSA Core : Buffer Start Address =%x and Size is %d \n", 
			BufferParams .BufferBaseAddr_p,BufferParams.BufferSize));

	
	}
	
	
	{
		U32 Address;
		Address = (U32)phys_to_virt((unsigned long)BufferParams .BufferBaseAddr_p);
		Address = Address| 0xA0000000;  //Virtual Pointer// Get the ST40 Virtual Address
		runtime->dma_area    =  (U8*)Address; 
		runtime->dma_addr    = (dma_addr_t)runtime->dma_area;	//Physical Pointer
		runtime->dma_bytes   = (size_t)BufferParams.BufferSize;
	}
			DEBUG_PRINT(("ALSA Core : snd_pcm_playback_hwparams BIG PHY AREA \n"));
			DEBUG_PRINT(("ALSA Core : snd_pcm_playback_hwparams Area=%x, addr=%xSize =%d \n  ",runtime->dma_area  ,runtime->dma_addr,
				size));


	/*Set Audio Driver Interface */
	chip->WritePointer = runtime->dma_area;    
	chip->ReadPointer =  runtime->dma_area ;  
	chip->BaseAddress  =runtime->dma_area;
	chip->TopAddress = chip->BaseAddress + runtime->dma_bytes;
	chip->PrevReadPointer = chip->ReadPointer; 
	chip->SizeOfBuffer     =  BufferParams.BufferSize;
	DEBUG_PRINT((" Debug : DMA Pointer =%x \n", runtime->dma_addr ));
	if(chip->card_data->major == PCM0_DEVICE)
	{
		Error=STAUD_IPSetDataInputInterface(chip->AudioHandle,STAUD_OBJECT_INPUT_CD0,STAPIWritePointerFunction, STAPIReadPointerFunction, (void *)chip);
	}
	else if (chip->card_data->major == PCM1_DEVICE)
	{
		Error=STAUD_IPSetDataInputInterface(chip->AudioHandle,STAUD_OBJECT_INPUT_CD1,STAPIWritePointerFunction, STAPIReadPointerFunction, (void *)chip);
	}
	else if (chip->card_data->major == SPDIF_DEVICE)
	{
		Error=STAUD_IPSetDataInputInterface(chip->AudioHandle,STAUD_OBJECT_INPUT_CD2,STAPIWritePointerFunction, STAPIReadPointerFunction, (void *)chip);
	}
	if(Error !=0)
	{
		printk(KERN_ALERT "STAUD_IPSetDataInputInterface Error = %d and Device ID is =%d \n", Error, chip->card_data->major);
	}
	

	DEBUG_PRINT(("ALSA Core : >>> snd_pcm_playback_hwparams dma_area = 0x%08lx err = %d\n",substream->runtime->dma_area, err));

	err = 1; /* Changed buffer */
	return err;
}


/*
 * This is a constraint rule which limits the period size to the capabilities
 * of the ST PCM Players. These only have a 19bit count register which
 * counts individual samples, i.e. for a 10-channel player it will count 10
 * for each alsa 10 channel frame. This means we also need to ensure that
 * the number of samples is an exact multiple of the number of channels.
 */
 #if 0
static int snd_pcm_period_size_rule(snd_pcm_hw_params_t *params,
				     snd_pcm_hw_rule_t   *rule)
{
	snd_interval_t *periodsize;
	snd_interval_t *channels;
	snd_interval_t  newperiodsize;

	int refine = 0;
	DEBUG_PRINT(("ALSA Core : >>> snd_pcm_period_size_rule \n"));
	periodsize    = hw_param_interval(params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
	newperiodsize = *periodsize;
	channels      = hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS);
	DEBUG_PRINT(("ALSA Core : snd_pcm_period_size_rule Period size min=%d PS max =%d, channel min=%d 
		channel max =%d PCMP_MAX_SAMPLES =%d \n",periodsize->min,newperiodsize.max,channels->max, channels->min,PCMP_MAX_SAMPLES));

	if((periodsize->max * channels->min) > PCMP_MAX_SAMPLES) {
		newperiodsize.max = PCMP_MAX_SAMPLES / channels->min;
		refine = 1;
	}

	if((periodsize->min * channels->min) > PCMP_MAX_SAMPLES) {
		newperiodsize.min = PCMP_MAX_SAMPLES / channels->min;
		refine = 1;
	}

	if(refine) {
		DEBUG_PRINT(("snd_pcm_period_size_rule: refining (%d,%d) to (%d,%d)\n",periodsize->min,periodsize->max,newperiodsize.min,newperiodsize.max));
		return snd_interval_refine(periodsize, &newperiodsize);
	}
	DEBUG_PRINT(("ALSA Core : <<<  snd_pcm_period_size_rule \n"));
	return 0;
}
#endif 

static int snd_pcm_playback_open(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	int err = 0;

	DEBUG_PRINT(("ALSA Core :>>> snd_pcm_playback_open(substream = 0x%08lx)\n",substream));
	DEBUG_PRINT((">>> chip = 0x%08lx\n",chip));
		
	snd_pcm_set_sync(substream);
#if  0
//  No Rule 
	snd_pcm_hw_rule_add(substream->runtime,
			    0, SNDRV_PCM_HW_PARAM_CHANNELS,
			    snd_pcm_period_size_rule,
			    0, SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
			    -1);

	snd_pcm_hw_rule_add(substream->runtime,
			    0, SNDRV_PCM_HW_PARAM_PERIOD_TIME,
			    snd_pcm_period_size_rule,
			    0, SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
			    -1);

	snd_pcm_hw_rule_add(substream->runtime,
			    0, SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
			    snd_pcm_period_size_rule,
			    0, SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
			    -1);
#endif 
	//spin_lock(&chip->lock);

	chip->current_substream = substream;
       runtime->hw = chip->hw;

	 
// To check in case of decoder is not working than in rawmode of spdif 
// we will support only 32 mode of data else we can support any 


        if(chip->playback_ops->open_device)
		err = chip->playback_ops->open_device(substream);

//	spin_unlock(&chip->lock);
	DEBUG_PRINT(("ALSA Core :<<<  snd_pcm_playback_open(substream = 0x%08lx)\n",substream));

	return err;
}

/*
 * nopage callback for mmapping a RAM page
 */
 
static struct page *snd_pcm_mmap_data_nopage(struct vm_area_struct *area, unsigned long address, int *type)
{
        snd_pcm_substream_t *substream = (snd_pcm_substream_t *)area->vm_private_data;
        snd_pcm_runtime_t *runtime;
        unsigned long offset;
        struct page * page;
        void *vaddr;
        size_t dma_bytes;
     
        if (substream == NULL)
                return NOPAGE_OOM;
        runtime = substream->runtime;
        offset = area->vm_pgoff << PAGE_SHIFT;
        offset += address - area->vm_start;
        snd_assert((offset % PAGE_SIZE) == 0, return NOPAGE_OOM);
        dma_bytes = PAGE_ALIGN(runtime->dma_bytes);
        if (offset > dma_bytes - PAGE_SIZE)
                return NOPAGE_SIGBUS;

        if (substream->ops->page) {
                page = substream->ops->page(substream, offset);
                if (! page)
                        return NOPAGE_OOM;
        } else {
                vaddr = runtime->dma_area + offset;
                page = virt_to_page(vaddr);
        }
        if (!PageReserved(page))
                get_page(page);
        if (type)
                *type = VM_FAULT_MINOR;
        return page;
}


static struct vm_operations_struct snd_pcm_vm_ops_data =
{
        .open =         snd_pcm_mmap_data_open,
        .close =        snd_pcm_mmap_data_close,
        .nopage =       snd_pcm_mmap_data_nopage,
};

/*
 * mmap the DMA buffer on RAM
 */
 
static int snd_pcm_mmap(snd_pcm_substream_t *substream, struct vm_area_struct *area)
{
        area->vm_ops = &snd_pcm_vm_ops_data;
        area->vm_private_data = substream;
        area->vm_flags |= VM_RESERVED;

        area->vm_page_prot = pgprot_noncached(area->vm_page_prot);

        atomic_inc(&substream->runtime->mmap_count);
        return 0;
}


static int snd_pcm_silence(snd_pcm_substream_t *substream, int channel, 
                            snd_pcm_uframes_t    pos,       snd_pcm_uframes_t count)
{
        snd_pcm_runtime_t *runtime = substream->runtime;
		unsigned char * TempWriteLevel;
		pcm_hw_t *chip = snd_pcm_substream_chip(substream);
        char *hwbuf;
	int   totalbytes;
    
        if(channel != -1)
                return -EINVAL;
         
        hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);

	  totalbytes = frames_to_bytes(runtime, count);
	chip->NumberOfBytesCopied += totalbytes;
        snd_pcm_format_set_silence(runtime->format, hwbuf, totalbytes);
                 
        /*Update Write Pointer STAPI WORK */
		DEBUG_PRINT(("Silence \n"));
		DEBUG_PRINT(("Current Write Ptr=%x, Size =%d \n", chip->WritePointer, (count *runtime->channels)));
		TempWriteLevel = chip->WritePointer + (count *runtime->channels);
		if((U32)TempWriteLevel>(runtime->dma_addr +runtime->dma_bytes))
		{
			U32 MaxAllowedWritePointer = (U32)runtime->dma_addr +(U32)runtime->dma_bytes;
			U32 Diff;
			U32 RemainingBytes;
			Diff = (U32)MaxAllowedWritePointer - (U32)chip->WritePointer; 
			RemainingBytes = count *runtime->channels - Diff;
			chip->WritePointer = chip->WritePointer + RemainingBytes;

		}
		else 
		{
			chip->WritePointer = TempWriteLevel; 
		}
		DEBUG_PRINT(("Updated Write Ptr=%x \n", chip->WritePointer));
        return 0;
        
}



static int snd_pcm_copy(snd_pcm_substream_t	*substream,
			 int			 channel, 
			 snd_pcm_uframes_t	 pos,
			 void __user		*buf,
			 snd_pcm_uframes_t	 count)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	unsigned char * TempWriteLevel;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	U8		  *hwbuf;
	int                totalbytes;
	 
	if(channel != -1)
		return -EINVAL;

	/*Assuming Middle Layer will respect the Boundary of Buffer */         
	hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);
	chip->Copy = TRUE; 
	totalbytes = frames_to_bytes(runtime, count);
	chip->NumberOfBytesCopied +=totalbytes;

	//DEBUG_PRINT((" Copy CallBack Src add =%x and Bytes = %d \n", (U32)hwbuf, totalbytes));
	/*Actaul Data Copied To look at this again==  Debug 2 Lines */

	if((hwbuf+totalbytes)>(chip->TopAddress))
	{
		U32  MaxAllowedWritePointer = (U32)runtime->dma_addr +(U32)runtime->dma_bytes;
		U32 Diff;
		U32 RemainingBytes;
		Diff = (U32)MaxAllowedWritePointer - (U32)hwbuf; 
		if(copy_from_user(hwbuf, buf, Diff))
		{
			printk(KERN_ALERT "Copy Failed \n");
			return -EFAULT;
		}	
		hwbuf =chip->BaseAddress;
		buf +=Diff;
		RemainingBytes = totalbytes - Diff;	
		if(copy_from_user(hwbuf, buf, RemainingBytes))
		{
			printk(KERN_ALERT "Copy Failed \n");
			return -EFAULT;
		}	
	

	}
	else 
	{
		if(copy_from_user(hwbuf, buf, totalbytes))
		{
			printk(KERN_ALERT "Copy Failed \n");
			return -EFAULT;
		}
	}

 
	chip->WritePointer = hwbuf;  //  Debug May be needed to Add this 
	/*Do not actual copy as we have dma interface enabled */
	//DEBUG_PRINT(("Copy Current Write Ptr=%x, Size =%d, pos=%x \n", chip->WritePointer, (2*count *runtime->channels),pos));
	TempWriteLevel = chip->WritePointer + totalbytes;

	if((U32)TempWriteLevel>(runtime->dma_addr +runtime->dma_bytes))
	{
		U32  MaxAllowedWritePointer = (U32)runtime->dma_addr +(U32)runtime->dma_bytes;
		U32 Diff;
		U32 RemainingBytes;
		Diff = (U32)MaxAllowedWritePointer - (U32)chip->WritePointer; 
		RemainingBytes = totalbytes - Diff;
		chip->WritePointer = (U8*)runtime->dma_addr + RemainingBytes;

	}
	else 
	{
		chip->WritePointer = TempWriteLevel;
	}
	
//	DEBUG_PRINT(("Updated Write Ptr=%x \n", chip->WritePointer));
	return 0; 
	
}


static void snd_card_pcm_free(snd_pcm_t *pcm)
{
	DEBUG_PRINT(("snd_card_pcm_free(pcm = 0x%08lx)\n",pcm));	
	snd_pcm_lib_preallocate_free_for_all(pcm);
}


static snd_pcm_ops_t  snd_card_playback_ops_pcm = {
	.open      =            snd_pcm_playback_open,
        .close     =            snd_pcm_playback_close,
     	.mmap      =            snd_pcm_mmap,
        .silence   =            snd_pcm_silence,
	.copy      =            snd_pcm_copy,
        .ioctl     =            snd_pcm_lib_ioctl,
        .hw_params =            snd_pcm_playback_hwparams,
        .hw_free   =            snd_pcm_playback_hwfree,
        .prepare   =            snd_pcm_playback_prepare,
        .trigger   =            snd_playback_trigger,
        .pointer   =            snd_pcm_playback_pointer,
};


static int snd_pcm_capture_hwfree(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	DEBUG_CAPTURE_CALLS((" Debug : snd_pcm_capture_hwfree \n "));
        chip->card_data->in_use = 0;
	
	if(runtime->dma_area == NULL)
		return 0;

	runtime->dma_area    = 0;
	runtime->dma_addr    = 0;
	runtime->dma_bytes   = 0;
	return 0;
}

static snd_pcm_uframes_t snd_pcm_capture_pointer(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	//DEBUG_PRINT((" Debug :snd_pcm_playback_pointer \n"));
	
	return chip->playback_ops->playback_pointer(substream);
}


static int snd_pcm_capture_prepare(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	
	/* Chip isn't running at this point so we don't have to disable interrupts*/
//	spin_lock_irqsave(&chip->lock,flags);

	DEBUG_CAPTURE_CALLS(("snd_pcm_capture_prepare \n"));
	chip->card_data->in_use = 1;
	//spin_unlock_irqrestore(&chip->lock,flags);

	if(chip->playback_ops->program_hw(substream) < 0)
		return -EIO;

	return 0;
}

static int snd_capture_trigger(snd_pcm_substream_t * substream, int cmd)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	DEBUG_CAPTURE_CALLS(("  Debug snd_playback_trigger \n"));
	switch(cmd)
	{
		case SNDRV_PCM_TRIGGER_START:
			DEBUG_CAPTURE_CALLS(("  Debug snd_capture_trigger_start \n"));
			
			chip->State = STAUD_ALSA_START;
			chip->NumberOfBytesCopied = 0;
			chip->NumberUsed =0;
			chip->playback_ops->start_playback(substream);
			DEBUG_CAPTURE_CALLS(("  Debug snd_capture_trigger_started \n"));
			
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			DEBUG_CAPTURE_CALLS(("  Debug snd_capture_triggerr_stop \n"));
			//if (chip->State == STAUD_ALSA_STOP)
				//return 0; //  Debug above two lines 
			
			 chip->State = STAUD_ALSA_STOP;
			
			 chip->playback_ops->stop_playback(substream);
			 chip->card_data->in_use = 0;
			 chip->NumberOfBytesCopied = 0;
			 chip->NumberUsed =0;
			 chip->PrevReadPointer=chip->BaseAddress;		
			 chip->ReadPointer = chip->BaseAddress;
			 DEBUG_CAPTURE_CALLS(("  Debug snd_capture_trigger_stopped \n"));

			 // Reset the Write Pointer  ---  
			chip->WritePointer = (U8* )substream->runtime->dma_addr;
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			chip->playback_ops->pause_playback(substream);
			DEBUG_CAPTURE_CALLS(("  Debug snd_capture_trigger_pause \n"));
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			chip->playback_ops->unpause_playback(substream);
			DEBUG_CAPTURE_CALLS(("  Debug snd_capture_trigger_resume \n"));
			break;
		default:
			return -EINVAL;
	}
	snd_pcm_trigger_done(substream,substream);	
	return 0;	
}


static int snd_pcm_capture_close(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	U32 Error=0;

	DEBUG_CAPTURE_CALLS(("snd_pcm_capture_close(substream = 0x%08lx)\n",substream));
	DEBUG_CAPTURE_CALLS((">>> chip = 0x%08lx\n",chip));

	/*
	 * If the PCM clocks are programmed then ensure the playback is
	 * stopped. If not do nothing otherwise we can end up with the
	 * DAC in a bad state.
	 */
	 if(chip->State == STAUD_ALSA_START)
	 {
		if(chip->are_clocks_active)
			chip->playback_ops->stop_playback(substream);  
	 }
	chip->are_clocks_active = 0;
	Error = STAUD_Close(chip->AudioHandle);
	if(Error!=0)
	{
		printk(KERN_ALERT "STAUD_Close Failed Error code = %d and Device id is = %d \n",Error,chip->card_data->major);
		
		return -1;
	}


	chip->current_substream = 0;

	if(chip->card_data->major==CAPTURE_DEVICE)
	{
			ALSACommand[3][0] = 1;

	}
	STOS_SemaphoreSignal(alsaPlayerTaskSem_p);
	return 0;
}

ST_ErrorCode_t alsacapture(	U8 * MemoryStart, U32 Size, U32 PTS, U32 PTS33,void * clientInfo)
{
	pcm_hw_t * chip = (pcm_hw_t*) clientInfo; 
	
	/*if(((chip->NumberOfBytesCopied - chip->NumberUsed)+Size) >= (chip->TopAddress - chip->BaseAddress))
	{
		
		printk(KERN_ALERT "Over run by Driver \n");
		snd_pcm_period_elapsed(chip->current_substream);
		return; 
	}
	*/
	chip->WritePointer =(U8*)((U32) MemoryStart + Size);  
	chip->NumberOfBytesCopied +=Size;
	
	DEBUG_PRINT(("STAUD Reader Callback Memory Start =%x, Size =%d \n ",MemoryStart, Size));	
	snd_pcm_period_elapsed(chip->current_substream);
	return ST_NO_ERROR;
}

static int snd_pcm_capture_hwparams(snd_pcm_substream_t * substream,
					 snd_pcm_hw_params_t * hw_params)
{
	int   err  = 0;


	U32 Error=0;
	STAUD_BufferParams_t BufferParams;
	snd_pcm_runtime_t *runtime = substream->runtime;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);

	DEBUG_CAPTURE_CALLS(("snd_pcm_capture_hwparams \n"));
	Error = STAUD_IPGetInputBufferParams(chip->AudioHandle,STAUD_OBJECT_INPUT_PCMREADER0,&BufferParams);

	
	{
		U32 Address;
		Address = (U32)phys_to_virt((unsigned long)BufferParams .BufferBaseAddr_p);
		Address = Address| 0xA0000000;  //Virtual Pointer// Get the ST40 Virtual Address
		runtime->dma_area    =  (U8*)Address; 
		runtime->dma_addr    = (dma_addr_t)runtime->dma_area;	//Physical Pointer
		runtime->dma_bytes   = (size_t)BufferParams.BufferSize;
	}


	/*Set Audio Driver Interface */
	chip->WritePointer = runtime->dma_area;    
	chip->ReadPointer =  runtime->dma_area ;  
	chip->BaseAddress  =runtime->dma_area;
	chip->TopAddress = chip->BaseAddress + runtime->dma_bytes;
	chip->PrevReadPointer = chip->ReadPointer; 
	chip->SizeOfBuffer     =  BufferParams.BufferSize;
	DEBUG_CAPTURE_CALLS((" Debug : DMA Pointer =%x \n", runtime->dma_addr ));

	Error = STAUD_DPSetCallback(chip->AudioHandle, STAUD_OBJECT_FRAME_PROCESSOR,alsacapture,chip);
	if(Error !=0)
	{
		printk(KERN_ALERT "STAUD_DPSetCallback Error = %d and Device ID is =%d \n", Error, chip->card_data->major);
	}
	


	err = 1; /* Changed buffer */
	return err;
}





static int snd_pcm_capture_open(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	int err = 0;

	DEBUG_CAPTURE_CALLS(("snd_pcm_capture_open(substream = 0x%08lx)\n",substream));
	DEBUG_CAPTURE_CALLS((">>> chip = 0x%08lx\n",chip));

	snd_pcm_set_sync(substream);

	chip->current_substream = substream;
       runtime->hw = chip->hw;
	
	
        if(chip->playback_ops->open_device)
		err = chip->playback_ops->open_device(substream);

//	spin_unlock(&chip->lock);

	return err;
}


static int snd_capture_copy(snd_pcm_substream_t	*substream,
			 int			 channel, 
			 snd_pcm_uframes_t	 pos,
			 void __user		*dst,
			 snd_pcm_uframes_t	 count)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	unsigned char * TempWriteLevel;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	char		  *hwbuf;
	int                totalbytes ;
	 
	if(channel != -1)
		return -EINVAL;

	printk(KERN_ALERT "Wrong copy callback called from ALSA lib  \n");	
	
	if(chip->State == STAUD_ALSA_STOP)
	{
		printk("Wrong copy callback called from ALSA lib After stop \n");
		return 0;
	}

	DEBUG_PRINT(("snd_capture_copy() callback \n "));
	
	/*Buffer Pointer From Where we will start Copying*/
	hwbuf = chip->ReadPointer; 

	
	if(chip->NumberOfBytesCopied != chip->NumberUsed)
	{
		totalbytes = chip->NumberOfBytesCopied - chip->NumberUsed;
	
	}
	if(frames_to_bytes(runtime, count)<=totalbytes)
	{
		totalbytes = frames_to_bytes(runtime, count); 
	}
	totalbytes = frames_to_bytes(runtime, count); 
	DEBUG_PRINT(("Copy params pos =%d, count =%d \n", pos, count));
	if(copy_to_user(dst, hwbuf, totalbytes))
	{
		printk(KERN_ALERT "Copy Failed \n");
		return -EFAULT;
	}
	
	
	TempWriteLevel = chip->ReadPointer + totalbytes;

	if((U32)TempWriteLevel>(runtime->dma_addr +runtime->dma_bytes))
	{
		U32  MaxAllowedWritePointer = (U32)runtime->dma_addr +(U32)runtime->dma_bytes;
		U32 Diff;
		U32 RemainingBytes;
		Diff = (U32)MaxAllowedWritePointer - (U32)chip->ReadPointer; 
		RemainingBytes = totalbytes - Diff;
		chip->ReadPointer = (U8 *)runtime->dma_addr + RemainingBytes;

	}
	else 
	{
		chip->ReadPointer = TempWriteLevel;
	}
	chip->NumberUsed +=totalbytes; 
	DEBUG_PRINT(("Updated ReadPointer =%x \n", chip->ReadPointer));
	return 0; 
	
}





static snd_pcm_ops_t  snd_card_capture_ops_pcm = {
	.open      =            snd_pcm_capture_open,
        .close     =            snd_pcm_capture_close,
	//.copy      =            snd_capture_copy,
        .ioctl     =            snd_pcm_lib_ioctl,
        .hw_params =            snd_pcm_capture_hwparams,
        .hw_free   =            snd_pcm_capture_hwfree,
        .prepare   =            snd_pcm_capture_prepare,
        .trigger   =            snd_capture_trigger,
        .pointer   =            snd_pcm_capture_pointer,
};

static int __devinit snd_card_pcm_allocate(pcm_hw_t *snd_card, int device,char* name)
{
	int err;
	snd_pcm_t *pcm;

	err = snd_pcm_new(snd_card->card,name,device, 1, 0, &pcm);
	if (err < 0)
		return err;

	
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_card_playback_ops_pcm);


	pcm->private_data = snd_card;
	pcm->private_free = snd_card_pcm_free;
	pcm->info_flags   = 0;
	snd_card->pcm = pcm;
	strcpy(pcm->name, name);

	snd_pcm_lib_preallocate_pages_for_all(pcm,
					SNDRV_DMA_TYPE_CONTINUOUS,
					snd_dma_continuous_data(GFP_KERNEL),
					PCM_PREALLOC_SIZE,
					PCM_PREALLOC_MAX);
	return 0;
}


static int __devinit snd_card_capture_allocate(pcm_hw_t *snd_card, int device,char* name)
{
	int err;
	snd_pcm_t *pcm;

	err = snd_pcm_new(snd_card->card,name,device, 0, 1, &pcm);
	if (err < 0)
		return err;

	
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_card_capture_ops_pcm);


	pcm->private_data = snd_card;
	pcm->private_free = snd_card_pcm_free;
	pcm->info_flags   = 0;
	snd_card->pcm = pcm;
	strcpy(pcm->name, name);

	snd_pcm_lib_preallocate_pages_for_all(pcm,
					SNDRV_DMA_TYPE_CONTINUOUS,
					snd_dma_continuous_data(GFP_KERNEL),
					PCM_PREALLOC_SIZE,
					PCM_PREALLOC_MAX);
	return 0;
}
static int __init alsa_card_init(void)
{
	int i=0;
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : >>> alsa_card_init() \n "));
//for(i=0;i<SND_DRV_CARDS;i++){
	printk(KERN_INFO "  Loading ST ALSA Module \n "); 
	for(i=0;i<4;i++)
	{
		if (snd_pcm_card_probe(i) < 0)
		{
			printk(KERN_ALERT "STAUD ALSA : <<< ENODEV alsa_card_init() \n ");
			printk(KERN_ALERT "STm PCM Player not found or device busy\n");
			return -ENODEV;
		}
	}
	printk(KERN_INFO " ST ALSA Module Loaded \n "); 
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : <<< 0 alsa_card_init() \n "));
	return 0;
}


static void __exit alsa_card_exit(void)
{
	int i=0;
	printk("Removing ST ALSA Module \n");
//	for(i=0;i<SND_DRV_CARDS;i++){
	
	for(i=0;i<4;i++)
	{
		if(card_list[i].device)
		{
			DEBUG_PRINT((" Freeing card#%d \n",i));
			snd_pcm_card_unregister(i);
			snd_card_free(card_list[i].device);
		}
	}
	
		printk("Removed ST ALSA Module \n");
}


module_init(alsa_card_init)
module_exit(alsa_card_exit)
