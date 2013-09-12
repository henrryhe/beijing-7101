/*****************************************************************************
 *
 *  Module      : staudlx_ioctl_lib.c
 *  Date        : 11-07-2005
 *  Author      : TAYLORD
 *  Description : Implementation for calling STAPI ioctl functions
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/
//#define  STTBX_Print

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "staudlx.h"
#include "stclkrv.h"

#include "staudlx_ioctl.h"
#include "stos.h"


/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/
typedef struct
{
	STAUD_Handle_t		Handle;
	GetWriteAddress_t	GetWriteAddress_p;
	InformReadAddress_t	InformReadAddress_p;
	void*				BufferHandle;
} AudCallbackMap_t;

typedef struct
{
	size_t size;
	void*  uaddr;
	void*  kaddr;
} AudPESMap_t;

/************ LOCAL TYPES FOR FRAME PROCESSOR ****************************/
typedef struct
{
	void*	uaddr;
	void*	kaddr;
	BOOL	BufferMapped;
	U32		TotalBufferSize;/* Total FP input buffer size used for mmap call */
} AudFPMap_t;

static AudFPMap_t AudFPMap;

#define AUD_CBMAP_ELEMENTS 5
#define AUD_PESMAP_ELEMENTS 5

/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/
#define STAUDLX_DEVICE_NOT_OPEN -1
#define PES_ES_PARSER_TASK_PRIORITY 6
static int	fd=STAUDLX_DEVICE_NOT_OPEN; /* not open */
static int	UseCount=0; /* Number of initialised devices */
static int	FPUseCount=0;
static		pthread_t *callback_thread;
static		BOOL callback_exit;
static		BOOL fpcallback_exit;
static		pthread_t *FPcallback_thread;
static		FrameDeliverFunc_t AppFunc_fp; /* To save application level func ptr */
void		*App_clientInfo; /* To save client file handle */

static char	RevisionString[MAX_REVISION_STRING_LEN];

static		AudCallbackMap_t AudCallbackMap[AUD_CBMAP_ELEMENTS];
static		AudPESMap_t AudPESMap[AUD_PESMAP_ELEMENTS];

/********************** External PCM Input **************/
typedef struct
{
	void*	uaddr;
	void*	kaddr;
	BOOL	BufferMapped;
	U32		TotalBufferSize;/* Total PCM input buffer size.Used for mmap call */
} AudPCMMap_t;


static AudPCMMap_t	AudPCMMap;
ST_ErrorCode_t		STAUD_MapPCMBufferToUserSpace(STAUD_PCMBuffer_t *PCMBuffer_p);
ST_ErrorCode_t		STAUD_UnmapPCMBufferFromUserSpace(void);
/*Function Prototype for Linux Kernel Dumper */
ST_ErrorCode_t		STAUD_DumpMMEData (void);

/********************** External PCM Input End **************/

/********************** FP Prototypes **********************/

ST_ErrorCode_t STAUD_GetBufferParam (
			STAUD_Handle_t Handle,
			STAUD_Object_t InputObject,
			STAUD_GetBufferParam_t * BufferParam);

ST_ErrorCode_t STAUD_MapInputBufferToUserSpace(
			STAUD_GetBufferParam_t *Buffer_p);

ST_ErrorCode_t STAUD_UnmapInputBufferFromUserSpace(void);

ST_ErrorCode_t STAUD_DPSetCallback (
			STAUD_Handle_t Handle,
			STAUD_Object_t DPObject,
			FrameDeliverFunc_t Func_fp,
			void * clientInfo);

/*** METHODS ****************************************************************/

void AUD_CallbackThread(void* v)
{
	STAUD_Ioctl_CallbackData_t CallbackData;
	memset(&CallbackData, 0, sizeof(STAUD_Ioctl_CallbackData_t));

    while(1)
	{
		//pthread_testcancel();

		if (callback_exit)
		{
			callback_exit = FALSE;
			break;
		}
		if (ioctl(fd, STAUD_IOC_AWAIT_CALLBACK, &CallbackData) != 0)
		{
			perror(strerror(errno));
			printf(" STAUD_IOC_AWAIT_CALLBACK():Ioctl error\n");
			sleep(1);
		}
		else
		{
			void *address = NULL;
			AudCallbackMap_t *callback = (AudCallbackMap_t*)CallbackData.Handle;
            int i;

			if (callback == NULL)
			{
				CallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;
				continue;
			}

			/* IOCTL is successful: Call the callback */
			if (CallbackData.CBType == STAUD_CB_WRITE_PTR)
			{

				CallbackData.ErrorCode = callback->GetWriteAddress_p(callback->BufferHandle, &address);
				if (CallbackData.ErrorCode != ST_NO_ERROR)
					continue;

				if (address == NULL)
				{
					CallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;
					continue;
				}

				/* Convert address to kernel space */
				for (i=0; i<AUD_PESMAP_ELEMENTS; i++)
				{
					if (address >= AudPESMap[i].uaddr && address < AudPESMap[i].uaddr + AudPESMap[i].size)
					{
						CallbackData.Address = AudPESMap[i].kaddr + (address - AudPESMap[i].uaddr);
						break;
					}
				}

				if (CallbackData.Address == NULL)
					CallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;
			}
			else /* STAUD_CB_READ_PTR */
			{
				/* Convert address to user space */
				for (i=0; i<AUD_PESMAP_ELEMENTS; i++)
				{
					if (CallbackData.Address >= AudPESMap[i].kaddr &&
						CallbackData.Address < AudPESMap[i].kaddr + AudPESMap[i].size)
					{
						address = AudPESMap[i].uaddr + (CallbackData.Address - AudPESMap[i].kaddr);
						break;
					}
				}
				if (address == NULL)
				{
					CallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;
					continue;
				}
				CallbackData.ErrorCode = callback->InformReadAddress_p(callback->BufferHandle, address);
			}
		}
	}
	//return NULL;
}
/* This function is called from AUD_FPCallbackThread in place
 * of the usual application function for debugging purpose and
 * to check the output of the dump.
 *
 * NOTE: If the output of Frame Processor needs to redirected
 * to some channel other than a file, then this function could be used
 *
 */

ST_ErrorCode_t AUD_Generic_Write(
		U8 *MemoryStart,U32 Size, 
		BufferMetaData_t bufferMetaData,
		void * clientInfo)
{
#if 0
	U32 tempsize;

	printf("invoking Mywrite \n");
	if(!Size)
	{
		printf(" Size is NULL \n");
		return ST_ERROR_BAD_PARAMETER;
	}

	//STTBX_Print(("Data size =%d \n",Size));

	//STTBX_PRINT((" Writing FP Output data now: \n"));
	//printf("MemoryStart=%x Size=%d \n",MemoryStart,Size);
	
	if(Bigmem==NULL)
	{
		STTBX_Print(("Memory NULL \n"));
		perror(strerror(errno));
	}
	else
	{
		memmove((void *)tempptr,(void *)MemoryStart,(size_t)Size);
		tempptr+=Size;
		printf("Memory Write Complete !! \n ");
	}

	filehandle=fopen("/tmp/FPDump.txt","w");
	ReturnValue=fwrite((void *)Bigmem,TotalBigsize,1,filehandle);
	fclose(filehandle);
	free((void *)Bigmem);

	while(tempsize)
	{
		printf("%x ",(U8)(*MemoryStart));
		MemoryStart+=1;
		tempsize--;
	}

	printf("\n All data has been written!! \n");

#endif
	return ST_NO_ERROR;
}

/* AUD_FPCallbackThread : The thread function that is called when the thread
 * is created in STAUD_DPSetCallback function. This function makes the ioctl
 * call to the kernel (STAUD_IOC_AWAIT_FPCALLBACK) and passes kernel data to
 * userspace. The thread is terminated in STAUD_Term.
 */

void AUD_FPCallbackThread(void* v)
{
	STAUD_Ioctl_FPCallbackData_t FPCallbackData;
	memset(&FPCallbackData, 0, sizeof(STAUD_Ioctl_FPCallbackData_t));

	while(1)
    {
        //pthread_testcancel();

		if (fpcallback_exit)
		{ 
			printf(" AUD_FPCallbackThread gracefully closed \n");
			fpcallback_exit = FALSE;
			break;
		}

		if (ioctl(fd, STAUD_IOC_AWAIT_FPCALLBACK, &FPCallbackData) != 0)
		{
			perror(strerror(errno));
			printf(" STAUD_IOC_AWAIT_FPCALLBACK():Ioctl error\n");
		}
		else
		{
    		void *address;	

			FPCallbackData.FrameDeliverFunc_fp = AppFunc_fp;
			/*
			Use the following  function pointer and comment above line if FP data needs to be directed
			to other output channel and not a file
			*/
			//FPCallbackData.FrameDeliverFunc_fp = (FrameDeliverFunc_t)AUD_Generic_Write;
			FPCallbackData.clientInfo = App_clientInfo;
			STTBX_Print(("\n AUD_FPCallbackThread: call application function \n"));
			STTBX_Print(("\n FPCallbackData.MemoryStart=%x FPCallbackData.Size=%d \n",FPCallbackData.MemoryStart,FPCallbackData.Size));


			/* Convert address to user space */
			address=(void *)FPCallbackData.MemoryStart;
			if (address >= AudFPMap.kaddr && address < (AudFPMap.kaddr + AudFPMap.TotalBufferSize))
			{
				address = AudFPMap.uaddr + (address - AudFPMap.kaddr);
			}
			if (address == NULL)
			{
				FPCallbackData.ErrorCode = ST_ERROR_BAD_PARAMETER;
				printf("User space map address is NULL and invalid \n");
				break;
			}
				FPCallbackData.MemoryStart=(U8 *)address;
				FPCallbackData.FrameDeliverFunc_fp(
						FPCallbackData.MemoryStart,
						FPCallbackData.Size,
						FPCallbackData.bufferMetaData,
						(void *)FPCallbackData.clientInfo);

				STTBX_Print((" STAUD_IOC_AWAIT_FPCALLBACK():Ioctl successful \n"));
		}
		STTBX_Print(("AUD_FPCallbackThread: Loop continuosly inside thread \n"));
    }
}

ST_ErrorCode_t STAUD_Init(const ST_DeviceName_t DeviceName, STAUD_InitParams_t * InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if( !UseCount ){ /* First time? */
        char *devpath;

        devpath = getenv("STAUDLX_IOCTL_DEV_PATH");  /* get the path for the device */

        if( devpath ){
            fd = open( devpath, O_RDWR);  /* open it */
            if( fd == STAUDLX_DEVICE_NOT_OPEN ){
                perror(strerror(errno));

                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                 /* Create the callback thread using STOS_TaskCreate instead of pthread_create*/
                if (STOS_TaskCreate(AUD_CallbackThread,NULL,NULL,0,NULL,NULL,&callback_thread,
								NULL,PES_ES_PARSER_TASK_PRIORITY,NULL,0))
                {
                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
            }

        }
        else{
            fprintf(stderr,"The dev path enviroment variable is not defined: STAUDLX_IOCTL_DEV_PATH\n");
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if( ErrorCode == ST_NO_ERROR ){
        STAUD_Ioctl_Init_t UserParams;

        memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
        UserParams.InitParams = *InitParams_p;

        /* IOCTL the command */
        if (ioctl(fd, STAUD_IOC_INIT, &UserParams) != 0)
        {
            perror(strerror(errno));

            /* IOCTL failed */
            ErrorCode = ST_ERROR_BAD_PARAMETER;

            printf(" STAUDLX_IOCTL_Init():Ioctl error\n");
        }
        else
        {
            /* IOCTL is successfull: retrieve Error code */
            ErrorCode = UserParams.ErrorCode;
	    /* Increment Use count */
		UseCount++;

        }
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_Open(const ST_DeviceName_t DeviceName, const STAUD_OpenParams_t * OpenParams_p, STAUD_Handle_t * Handle_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Open_t UserParams;

    if ( OpenParams_p == NULL || Handle_p == NULL  )
    {
       printf(" STAUD_IOCTL_OPEN():Ioctl error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.OpenParams = *OpenParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPEN, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_OPEN():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: */
        /* get the handle. */
        *Handle_p = UserParams.Handle;
        /* and retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_Close(STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Close_t UserParams;


    UserParams.Handle = Handle;

   /* Frame Processor scenario -If we had mapped the buffer to user space, Unmap it */
   if(AudFPMap.BufferMapped)
   {
	printf("Calling STAUD_UnmapInputBufferFromUserSpace from DRStop \n");
	ErrorCode = STAUD_UnmapInputBufferFromUserSpace();
	AudFPMap.BufferMapped = 0;
   }


    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_CLOSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOC_CLOSE():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_Term(const ST_DeviceName_t DeviceName, const STAUD_TermParams_t * TermParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Term_t UserParams;

    if ( TermParams_p == NULL  )
    {
       printf(" STAUD_IOCTL_Term():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    /* If last termination */
    if (UseCount == 1)
    {
        callback_exit = TRUE;
    }

	if(FPUseCount==1)
	{
		fpcallback_exit=TRUE;
	}

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.TermParams = *TermParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_TERM, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_Term():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    if( UseCount ){
        UseCount--;
        if( !UseCount ){ /* Last Termination */
            //pthread_join(*callback_thread, NULL);
            close(fd);
	     fd = STAUDLX_DEVICE_NOT_OPEN;
        }
    }
    

    if(FPUseCount){
 	 FPUseCount--;
	 if( !FPUseCount ){ /* Last Termination of FP thread*/
            //pthread_join(FPcallback_thread, NULL);
	     if(fd!=STAUDLX_DEVICE_NOT_OPEN)
	     {
		 close(fd);
		 fd=STAUDLX_DEVICE_NOT_OPEN;
	     }
        }
    }
   

    return ErrorCode;
}

ST_ErrorCode_t STAUD_Play(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Play_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PLAY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_Play():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }



    return ErrorCode;
}
/*
 * STAUD_GetBufferParam : This function gets the size and starting address parameters for
 * 						the buffer related to the frame processor input.
 *
 */
ST_ErrorCode_t STAUD_GetBufferParam (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_GetBufferParam_t * BufferParam)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetBufferParam_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETBUFFERPARAM, &UserParams) != 0)
    {
        perror(strerror(errno));
        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_GETBUFFERPARAM():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
		if(ErrorCode == ST_NO_ERROR)
		{

		STTBX_Print(("\n UserParams.BufferParam.Length=0x%08x, UserParams.BufferParam.StartBase=0x%08x \n", (U32)UserParams.BufferParam.Length, (U32)UserParams.BufferParam.StartBase));

	    BufferParam->Length = UserParams.BufferParam.Length;
	    BufferParam->StartBase = UserParams.BufferParam.StartBase;
		STTBX_Print(("\n BufferParam->Length=0x%08x,  BufferParam->StartBase=0x%08x \n", (U32)BufferParam->Length, (U32)BufferParam->StartBase));

	    /* Update the Total buffer size for FP map also */
	    AudFPMap.TotalBufferSize = UserParams.BufferParam.Length;
		}
    }
    return ErrorCode;
}

/*
 * STAUD_MapInputBufferToUserSpace : Using the buffer parameters obtained by STAUD_GetBufferParam,
 * 								    the buffer space in kernel is mapped in the user space and
 * 								    the start address of this user space is returned.
 */
ST_ErrorCode_t STAUD_MapInputBufferToUserSpace(STAUD_GetBufferParam_t *Buffer_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    off_t pgoffset = (off_t)(Buffer_p->StartBase) % getpagesize();
    void *address, *kaddr = (void*)Buffer_p->StartBase;

    address = mmap(0, Buffer_p->Length + pgoffset, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)Buffer_p->StartBase - pgoffset);
	STTBX_Print(("pgoffset = %d",(off_t)pgoffset));
    STTBX_Print(("*** Buffer_p->StartBase = %x \n",Buffer_p->StartBase));

    printf("mmap(0, 0x%08x, 0x%02x, %d, %d, 0x%08x) => 0x%08x\n", (U32)( Buffer_p->Length + pgoffset),PROT_READ | PROT_WRITE, MAP_SHARED, fd, (U32)((off_t)Buffer_p->StartBase - pgoffset), (U32)address);

    if (address == MAP_FAILED)
    {
		ErrorCode = ST_ERROR_BAD_PARAMETER;
		printf("STAUD_MapInputBufferToUserSpace FAILED \n");
    }
    else
    {
		Buffer_p->StartBase = address+ pgoffset;

		/* Create mapping lookup */
		AudFPMap.kaddr = kaddr;
		AudFPMap.uaddr = (void*)Buffer_p->StartBase;
		printf("STAUD_MapInputBufferToUserSpace SUCCESS \n");

		//printf("memsetting this space \n");
		//printf("One byte access: %x",*((U8 *)AudFPMap.uaddr));
		//memset(AudFPMap.uaddr,0,AudFPMap.TotalBufferSize);
		//printf("Memset complete !! Memory is accessible \n");
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_UnmapInputBufferFromUserSpace(void)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	off_t pgoffset = (off_t)(AudFPMap.uaddr) % getpagesize();

	if (munmap(AudFPMap.uaddr - pgoffset, AudFPMap.TotalBufferSize + pgoffset) == -1)
	{
	    	ErrorCode = ST_ERROR_BAD_PARAMETER;
		printf("STAUD_UNMapInputBufferFromUserSpace FAILED \n");
	}
	else
	{
		printf("STAUD_UnmapInputBufferFromUserSpace SUCCESS \n");
	}

	AudFPMap.TotalBufferSize = 0;
	AudFPMap.kaddr = NULL;
	AudFPMap.uaddr = NULL;

    return ErrorCode;
}

/*
 * STAUD_DPSetCallback : This is the user level library function which will be called for ST Linux OS.
 */
ST_ErrorCode_t STAUD_DPSetCallback (STAUD_Handle_t Handle, STAUD_Object_t DPObject, FrameDeliverFunc_t Func_fp,void * clientInfo)
{
    ST_ErrorCode_t ErrCode;
    STAUD_GetBufferParam_t BufferParam;
    STAUD_Ioctl_DPSetCallback UserParams;

    /* Note all user params except Func_fp */
    UserParams.Handle= Handle;
    UserParams.inputObject= DPObject;
    UserParams.Func_fp= Func_fp;
    UserParams.clientInfo= clientInfo;

    STTBX_Print((" STAUD_DPSetCallback starts \n"));

    ErrCode = STAUD_GetBufferParam( Handle, DPObject, &BufferParam);

	if (ErrCode !=0)
	{
	    perror(strerror(ErrCode));

	    printf(" STAUD_GetFPBufferSize failed: Mmap will not take place \n");

	    return ErrCode;
	}

	ErrCode = STAUD_MapInputBufferToUserSpace(&BufferParam);
	if(ErrCode == ST_NO_ERROR)
	{

	    STTBX_Print((" STAUD_MapInputBufferToUser Mapping  success \n"));
	    AudFPMap.BufferMapped=1;

	}
	else
	{
	    STTBX_Print((" STAUD_MapInputBufferToUser Mapping  fail \n"));

	    return ErrCode;
	}

	/* save application FP and clientinfo*/
	AppFunc_fp = Func_fp;
	App_clientInfo = clientInfo;

	if(!FPUseCount){
                 /* Create the callback thread using STOS_TaskCreate instead of pthread_create*/
		if (STOS_TaskCreate(AUD_FPCallbackThread,NULL,NULL,0,NULL,NULL,&FPcallback_thread,
			NULL,PES_ES_PARSER_TASK_PRIORITY,NULL,0))
		{
	    	ErrCode = ST_ERROR_NO_MEMORY;
        	}
		else
		{
			FPUseCount++;
		}
	}

	/* IOCTL the command */
	if (ioctl( fd, STAUD_IOC_DPSETCALLBACK, &UserParams)!= 0)

	{
	    perror(strerror(errno));
	    /* IOCTL failed */
	    ErrCode = ST_ERROR_BAD_PARAMETER;
	    printf("STAUD_IOC_DPSETCALLBACK():Ioctl error\n");
	}
	else
	{
	    /* IOCTL is successfull: retrieve Error code */
	    ErrCode = UserParams.ErrorCode;
	    if(ErrCode == ST_NO_ERROR)
	    {
		printf("STAUD_IOC_DPSETCALLBACK():Ioctl success\n");
	    }
	}

	STTBX_Print((" Applcation level STAUD_DPSetCallback returns here \n"));
	return ErrCode;

}
/*
STAUD_DumpMMEData : This function is called from Aud_MMEDump in aud_test.c
The user would fire the command aud_mmedump from test prompt when he wishes
to take a snapshot of the mme buffer.
The read call in this function would result in calling of the staud_ioctl_read function in
staud_ioctl.c which in turn would read the 3 MB kernel buffer and pass the data to
user space which will be stored in ../kernel_dump.txt
*/
ST_ErrorCode_t STAUD_DumpMMEData (void)
{
	int kernfd;
	U32 bytesRead;
	void *buf;

	/* The MME dump will be written to this file*/
	kernfd=open("../kernel_dump.txt",O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU|S_IRWXO);

	if(kernfd != -1)
	{
		STTBX_Print(("File open success! \n"));
	}
	else
	{
		STTBX_Print(("File open failed! \n"));
		perror(strerror(errno));
		return ST_ERROR_INVALID_HANDLE;
	}
	buf=malloc(MME_DATA_DUMP_SIZE); /* read entire block of MME_DATA_DUMP_SIZE */
	if(buf==NULL)
	{
		STTBX_Print((" Memory Allocation Failed \n"));
		perror(strerror(errno));
		return ST_ERROR_NO_MEMORY;
	}
	else
		STTBX_Print((" Memory Allocation Successful \n"));

	bytesRead=read(fd,buf,MME_DATA_DUMP_SIZE);
	STTBX_Print(("Bytes read=%d \n",bytesRead));
	if(bytesRead==-1)
	{
		STTBX_Print(("File READ Failure ! \n"));
	}
	else
	{
		STTBX_Print(("File READ Success! \n"));
		if ((write(kernfd,buf,bytesRead))!=bytesRead)
		{
			STTBX_Print(("Target File Write failure! \n"));
		}
		STTBX_Print(("Target File Write success! \n"));
	}
	free(buf);
	close(kernfd);
	return ST_NO_ERROR;

}

ST_ErrorCode_t STAUD_DRConnectSource(STAUD_Handle_t Handle,
                                         STAUD_Object_t DecoderObject,
                                         STAUD_Object_t InputSource)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRConnectSource_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.InputSource = InputSource;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRCONNECTSOURCE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_DRConnectSource():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PPDisableDownsampling(STAUD_Handle_t Handle,
            STAUD_Object_t PostProcObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPDisableDownsampling_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = PostProcObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPDISABLEDOWNSAMPLING, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_PPDisableDownsampling():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PPEnableDownsampling(STAUD_Handle_t Handle,
            STAUD_Object_t PostProcObject, U32 Value)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPEnableDownsampling_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = PostProcObject;
    UserParams.Value = Value;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPENABLEDOWNSAMPLING, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_PPEnableDownsampling():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_OPDisableSynchronization(STAUD_Handle_t Handle,
            STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPDisableSynchronization_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPDISABLESYNCHRONIZATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_OPDisableSynchronization():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_OPEnableSynchronization(STAUD_Handle_t Handle,
            STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPEnableSynchronization_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPENABLESYNCHRONIZATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_OPEnableSynchronization():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

 ST_ErrorCode_t STAUD_IPGetBroadcastProfile(STAUD_Handle_t Handle,
            STAUD_Object_t InputObject,
            STAUD_BroadcastProfile_t *
            BroadcastProfile_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetBroadcastProfile_t UserParams;

    if ( BroadcastProfile_p == NULL  )
    {
        printf(" STAUD_IOCTL_IPGetBroadcastProfile():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }



    UserParams.Handle = Handle;
    UserParams.InputObject= InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETBROADCASTPROFILE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_IPGetBroadcastProfile():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *BroadcastProfile_p = UserParams.BroadcastProfile;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

 ST_ErrorCode_t STAUD_DRGetCapability(const ST_DeviceName_t DeviceName,
                                         STAUD_Object_t DecoderObject,
                                         STAUD_DRCapability_t *Capability_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetCapability_t UserParams;

    if ( Capability_p == NULL  )
    {
       printf(" STAUD_IOCTL_DRGetCapability():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETCAPABILITY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRGetCapability():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        memcpy(Capability_p,&(UserParams.Capability),sizeof(STAUD_DRCapability_t));
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetDynamicRangeControl(STAUD_Handle_t Handle,
                                              STAUD_Object_t DecoderObject,
                                              STAUD_DynamicRange_t * DynamicRange_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetDynamicRangeControl_t UserParams;

    if ( DynamicRange_p == NULL  )
    {
       printf("STAUD_IOCTL_DRGetDynamicRangeControl():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETDYNAMICRANGECONTROL, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRGetDynamicRangeControl():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        memcpy(DynamicRange_p,&UserParams.DynamicRange,sizeof(STAUD_DynamicRange_t));
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPGetSamplingFrequency(STAUD_Handle_t Handle,
            STAUD_Object_t DecoderObject,
            U32 *SamplingFrequency_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetSamplingFrequency_t UserParams;

    if ( SamplingFrequency_p == NULL  )
    {
       printf("STAUD_IOCTL_IPGetSamplingFrequency():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }



    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETSAMPLINGFREQUENCY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_IPGetSamplingFrequency():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *SamplingFrequency_p=UserParams.SamplingFrequency;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}


ST_ErrorCode_t STAUD_DRGetSpeed(STAUD_Handle_t Handle,
                                STAUD_Object_t DecoderObject, S32 *Speed_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetSpeed_t UserParams;

    if ( Speed_p == NULL  )
    {
       printf("STAUD_IOCTL_DRGetSpeed():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }



    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETSPEED, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRGetSpeed():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *Speed_p=UserParams.Speed;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPGetStreamInfo(STAUD_Handle_t Handle,
                                      STAUD_Object_t DecoderObject,
                                      STAUD_StreamInfo_t *StreamInfo_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetStreamInfo_t UserParams;

    if ( StreamInfo_p == NULL  )
    {
       printf("STAUD_IOCTL_IPGetSreamInfo():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETSTREAMINFO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_IPGetSreamInfo():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *StreamInfo_p=UserParams.StreamInfo;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}
ST_ErrorCode_t STAUD_DRGetSyncOffset(STAUD_Handle_t Handle,
                                         STAUD_Object_t DecoderObject,
                                         S32 *Offset_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetSyncOffset_t UserParams;

    if ( Offset_p == NULL  )
    {
       printf("STAUD_IOCTL_DRGetSyncOffset():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETSYNCOFFSET, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRGetSyncOffset():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *Offset_p=UserParams.Offset;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRPause(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 STAUD_Fade_t *Fade_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRPause_t UserParams;

    if ( Fade_p == NULL  )
    {
       printf("STAUD_IOCTL_DRPause():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRPAUSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRPause():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *Fade_p=UserParams.Fade;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_IPPauseSynchro(STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject, U32 Delay)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPPauseSynchro_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;
    UserParams.Delay = Delay;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPPAUSESYNCHRO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_IPPauseSynchro():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRPrepare(STAUD_Handle_t Handle,
                                   STAUD_Object_t DecoderObject,
                                   STAUD_StreamParams_t *StreamParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRPrepare_t UserParams;

    if ( StreamParams_p == NULL  )
    {
       printf("STAUD_IOCTL_DRPrepare():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRPREPARE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRPrepare():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *StreamParams_p=UserParams.StreamParams;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRResume(STAUD_Handle_t Handle,
                              STAUD_Object_t DecoderObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRResume_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRRESUME, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRRESUME():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t STAUD_DRSetClockRecoverySource (STAUD_Handle_t Handle,
            STAUD_Object_t Object,
            STCLKRV_Handle_t ClkSource)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetClockRecoverySource_t UserParams;

    UserParams.Handle = Handle;
    UserParams.Object = Object;
    UserParams.ClkSource = ClkSource;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETCLOCKRECOVERYSOURCE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRSetClockRecoverySource():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}
#endif

 ST_ErrorCode_t STAUD_DRSetDynamicRangeControl(STAUD_Handle_t Handle,
            STAUD_Object_t DecoderObject,
            STAUD_DynamicRange_t *
            DynamicRange_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetDynamicRangeControl_t UserParams;


    if ( DynamicRange_p == NULL  )
    {
       printf("STAUD_IOCTL_DRSetDynamicRangeControl():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }



    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.DynamicRange = *DynamicRange_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETDYNAMICRANGECONTROL, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRSetDynamicRangeControl():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetSpeed(STAUD_Handle_t Handle,
                                STAUD_Object_t DecoderObject, S32 Speed)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetSpeed_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.Speed = Speed;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETSPEED, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRSetSpeed():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}


ST_ErrorCode_t STAUD_IPSetStreamID(STAUD_Handle_t Handle,
                                   STAUD_Object_t DecoderObject,
                                   U8 StreamID)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPSetStreamID_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.StreamID = StreamID;

    /* IOCTL the command */

    if (ioctl(fd, STAUD_IOC_IPSETSTREAMID, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_IPSetStreamID():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}


ST_ErrorCode_t STAUD_IPSkipSynchro(STAUD_Handle_t Handle,
                                   STAUD_Object_t InputObject, U32 Delay)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPPauseSynchro_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;
    UserParams.Delay = Delay;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPPAUSESYNCHRO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_IPPauseSynchro_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetSyncOffset(STAUD_Handle_t Handle,
                                     STAUD_Object_t DecoderObject,
                                     S32 Offset)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetSyncOffset_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.Offset = Offset;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETSYNCOFFSET, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRSetSyncOffset_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

 ST_ErrorCode_t STAUD_DRStart(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 STAUD_StreamParams_t *StreamParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRStart_t UserParams;

    if ( StreamParams_p == NULL  )
    {
       printf("STAUD_IOCTL_DRStart_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.StreamParams = *StreamParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSTART, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRStart_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *StreamParams_p = UserParams.StreamParams;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRStop(STAUD_Handle_t Handle,
                            STAUD_Object_t DecoderObject,
                            STAUD_Stop_t StopMode, STAUD_Fade_t *Fade_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRStop_t UserParams;

    if ( Fade_p == NULL  )
    {
       printf("STAUD_IOCTL_DRStop_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.StopMode = StopMode;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSTOP, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRStop_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *Fade_p = UserParams.Fade;
        ErrorCode = UserParams.ErrorCode;
    }

	/* PCM Injection scenario -If we had mapped the buffer to user space, Unmap it */
   if(AudPCMMap.BufferMapped)
   {
	printf("Calling STAUD_UnmapPCMBufferFromUserSpace from DRStop \n");
	ErrorCode = STAUD_UnmapPCMBufferFromUserSpace();
	AudPCMMap.BufferMapped = 0;
   }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_GetCapability(const ST_DeviceName_t DeviceName,
                                   STAUD_Capability_t *Capability_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetCapability_t UserParams;

    if ( Capability_p == NULL  )
    {
       printf("STAUD_IOCTL_GetCapability_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    strncpy (UserParams.DeviceName, DeviceName,ST_MAX_DEVICE_NAME_TOCOPY);

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETCAPABILITY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_GetCapability_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *Capability_p = UserParams.Capability;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}


ST_Revision_t STAUD_GetRevision(void)
{

    static char* source = "Aud Drv = [STAUDLX-REL_1.0.6]:Recommended LX = [HEGRnD_CBL_ACC_AMP_BL019_0]";
	memset(RevisionString,0,MAX_REVISION_STRING_LEN);
	if(fd == STAUDLX_DEVICE_NOT_OPEN)
	{
		/*Sepcial case::Get revision is called before initilizing audio, so return static string.
		No information about current LX firmware actully running on LX */
		strncpy(RevisionString,source,strlen(source));
	}
	else
	{

		/* STAudLX is initlaized, so get the string from core driver. This will give current LX firmware information too */
		if (ioctl(fd, STAUD_IOC_GETREVISION, RevisionString) != 0)
		{
		    perror(strerror(errno));
		    /* IOCTL failed */
		    printf(" STAUD_GetRevision():Ioctl error\n");
		}
	}

	return RevisionString;
}


ST_ErrorCode_t STAUD_OPSetDigitalMode(STAUD_Handle_t Handle,
                                      STAUD_Object_t OutputObject,
                                      STAUD_DigitalMode_t OutputMode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPSetDigitalMode_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;
    UserParams.OutputMode = OutputMode;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPSETDIGITALMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_OPSetDigitalMode_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}
ST_ErrorCode_t STAUD_OPGetDigitalMode(STAUD_Handle_t Handle,
                                      STAUD_Object_t OutputObject,
                                      STAUD_DigitalMode_t * OutputMode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPGetDigitalMode_t UserParams;

    if ( OutputMode == NULL  )
    {
       printf("STAUD_IOC_OPGetDigitalMode():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPGETDIGITALMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOC_OPGetDigitalMode():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *OutputMode = UserParams.OutputMode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPGetCapability(const ST_DeviceName_t DeviceName,
                                     STAUD_Object_t InputObject,
                                     STAUD_IPCapability_t *Capability_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetCapability_t UserParams;

    if ( Capability_p == NULL  )
    {
       printf("STAUD_IOCTL_IPGETCAPABILITY_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    strncpy (UserParams.DeviceName,DeviceName,ST_MAX_DEVICE_NAME_TOCOPY);

    UserParams.InputObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETCAPABILITY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_IPGETCAPABILITY_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Capability_p = UserParams.IPCapability;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPGetDataInterfaceParams(STAUD_Handle_t Handle,
            STAUD_Object_t InputObject,
            STAUD_DataInterfaceParams_t *
            DMAParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetDataInterfaceParams_t UserParams;


    if ( DMAParams_p == NULL  )
    {
       printf("STAUD_IOC_IPGETDATAINTERFACEPARAMS_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }



    UserParams.Handle = Handle;
    UserParams.DecoderObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETDATAINTERFACEPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));
        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_IPGETDATAINTERFACEPARAMS_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *DMAParams_p = UserParams.DMAParams;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_IPGetParams(STAUD_Handle_t Handle,
                                 STAUD_Object_t InputObject,
                                 STAUD_InputParams_t *InputParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetParams_t UserParams;

    if ( InputParams_p == NULL  )
    {
       printf("STAUD_IOC_IPGETPARAMS():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));
        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_IPGETPARAMS():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *InputParams_p = UserParams.InputParams;
    }

    return ErrorCode;
}

  ST_ErrorCode_t STAUD_IPGetInputBufferParams(STAUD_Handle_t Handle,
            STAUD_Object_t InputObject,
            STAUD_BufferParams_t* DataParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetInputBufferParams_t UserParams;


    if ( DataParams_p == NULL  )
    {
       printf("STAUD_IOC_IPGETINPUTBUFFERPARAMS():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETINPUTBUFFERPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));
        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_IPGETINPUTBUFFERPARAMS():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *DataParams_p = UserParams.DataParams;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPQueuePCMBuffer(STAUD_Handle_t Handle,
                                      STAUD_Object_t InputObject,
                                      STAUD_PCMBuffer_t *PCMBuffer_p,
                                      U32 NumBuffers, U32 *NumQueued_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    void * address = NULL;
    STAUD_Ioctl_IPQueuePCMBuffer_t UserParams;

    if ( PCMBuffer_p == NULL  || NumQueued_p == NULL )
    {
       printf("STAUD_IOC_IPQUEUEPCMBUFFER():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;
    UserParams.NumBuffers = NumBuffers;
    address = (void*)PCMBuffer_p->StartOffset;
   /* Map user address to kernel address */
   if((address >= AudPCMMap.uaddr) &&
   	(address < (AudPCMMap.uaddr + AudPCMMap.TotalBufferSize)))
   {
	address = AudPCMMap.kaddr + (address - AudPCMMap.uaddr);
   }

   PCMBuffer_p->StartOffset = (U32)address;

    UserParams.PCMBuffer = *PCMBuffer_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPQUEUEPCMBUFFER, &UserParams) != 0)
    {
        perror(strerror(errno));
        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_IPQUEUEPCMBUFFER():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *NumQueued_p = UserParams.NumQueued;
    }
    return ErrorCode;


}

ST_ErrorCode_t STAUD_IPGetPCMBuffer (STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									STAUD_PCMBuffer_t * PCMBuffer_p)

{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetPCMBuffer_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETPCMBUFFER, &UserParams) != 0)
    {
        perror(strerror(errno));
        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_IPGetPCMBUFFER():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
	if(ErrorCode == ST_NO_ERROR)
	{
		/* If we are calling Get PCM buffer for first time, map the kernel space to user space */
		if(!AudPCMMap.BufferMapped)
		{
			ErrorCode = STAUD_MapPCMBufferToUserSpace(&(UserParams.PCMBuffer_p));
			if(ErrorCode  != ST_NO_ERROR)
			{
				printf(" Error in MapBufferPCMToUserSpace = %x \n",ErrorCode);
				return ST_ERROR_BAD_PARAMETER;
			}
			AudPCMMap.BufferMapped = 1;
		}

		/* Map the kernel address to user address space */
		if(((void*)UserParams.PCMBuffer_p.StartOffset >= AudPCMMap.kaddr) &&
			((void*)UserParams.PCMBuffer_p.StartOffset < (AudPCMMap.kaddr + AudPCMMap.TotalBufferSize)))
		{
			UserParams.PCMBuffer_p.StartOffset =(U32) (AudPCMMap.uaddr + ((void*)UserParams.PCMBuffer_p.StartOffset - AudPCMMap.kaddr));
		}

	        *PCMBuffer_p = UserParams.PCMBuffer_p;
	}
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_IPGetPCMBufferSize (STAUD_Handle_t Handle,
									STAUD_Object_t InputObject,
									U32 * BufferSize)

{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetPCMBufferSize_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETPCMBUFFERSIZE, &UserParams) != 0)
    {
        perror(strerror(errno));
        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_IPGETPCMBUFFERSIZE():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
	if(ErrorCode == ST_NO_ERROR)
	{
		*BufferSize = UserParams.BufferSize;

		/* Update the Total buffer size for PCM map also */
		AudPCMMap.TotalBufferSize = UserParams.BufferSize;
	}
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPSetLowDataLevelEvent(STAUD_Handle_t Handle,
                                            STAUD_Object_t InputObject,
                                            U8 Level)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPSetLowDataLevelEvent_t UserParams;


    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;
    UserParams.Level = Level;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPSETLOWDATALEVELEVENT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_IPSETLOWDATALEVELEVENT():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPSetParams(STAUD_Handle_t Handle,
                                 STAUD_Object_t InputObject,
                                 STAUD_InputParams_t *InputParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPSetParams_t UserParams;

    if ( InputParams_p == NULL )
    {
       printf("STAUD_IOC_IPSETPARAMS():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }



    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;
    UserParams.InputParams = *InputParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPSETPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_IPSETPARAMS():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *InputParams_p = UserParams.InputParams;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_MXConnectSource(STAUD_Handle_t Handle,
                                     STAUD_Object_t MixerObject,
                                     STAUD_Object_t *InputSources_p,
                                     STAUD_MixerInputParams_t *
                                     InputParams_p, U32 NumInputs)
{

   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
   STAUD_Ioctl_MXConnectSource_t UserParams;

    if ( InputSources_p == NULL || InputParams_p == NULL)
    {
       printf("STAUD_MXConnectSource():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


   int counter=0;

   if (NumInputs > MAX_NUMBER_OF_STAUD_MIXERINPUTPARAMS)
   {
        printf("MAX_NUMBER_OF_STAUD_MIXERINPUTPARAMS has been exceeded from hard limit of %d:Ioctl error\n",MAX_NUMBER_OF_STAUD_MIXERINPUTPARAMS);
         return (ST_ERROR_BAD_PARAMETER);
   }

    UserParams.Handle = Handle;
    UserParams.MixerObject = MixerObject;
    UserParams.NumInputs = NumInputs;


    for (counter=0;counter<NumInputs;++counter)
    {
      UserParams.InputParams[counter] = *InputParams_p;
      ++InputParams_p;
    }

    for (counter=0;counter<NumInputs;++counter)
    {
      UserParams.InputSources[counter] = *InputSources_p;
      ++InputSources_p;
    }

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MXCONNECTSOURCE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_MXConnectSource():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_MXGetCapability(const ST_DeviceName_t DeviceName,
                                     STAUD_Object_t MixerObject,
                                     STAUD_MXCapability_t *Capability_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_MXGetCapability_t UserParams;

    if ( Capability_p == NULL )
    {
       printf("STAUD_MXGetCapability():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    memcpy(UserParams.DeviceName, DeviceName, sizeof(ST_DeviceName_t));
    UserParams.MixerObject = MixerObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MXGETCAPABILITY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_MXGetCapability():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Capability_p = UserParams.MXCapability;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_MXSetInputParams(STAUD_Handle_t Handle,
                                      STAUD_Object_t MixerObject,
                                      STAUD_Object_t InputSource,
                                      STAUD_MixerInputParams_t *
                                      InputParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_MXSetInputParams_t UserParams;

    if ( InputParams_p == NULL )
    {
       printf("STAUD_MXSetInputParams():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;
    UserParams.MixerObject = MixerObject;
    UserParams.InputSource = InputSource;
    UserParams.MXMixerInputParams = *InputParams_p;


    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MXSETINPUTPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_MXSetInputParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *InputParams_p = UserParams.MXMixerInputParams;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPGetAttenuation(STAUD_Handle_t Handle,
                                      STAUD_Object_t PostProcObject,
                                      STAUD_Attenuation_t *Attenuation_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetAttenuation_t UserParams;

    if ( Attenuation_p == NULL )
    {
       printf("STAUD_PPGetAttenuation():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPGETATTENUATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPGetAttenuation():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    *Attenuation_p=UserParams.Attenuation;
    return ErrorCode;

}

ST_ErrorCode_t STAUD_OPGetCapability(const ST_DeviceName_t DeviceName,
                                     STAUD_Object_t OutputObject,
                                     STAUD_OPCapability_t *Capability_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPGetCapability_t UserParams;

    if ( Capability_p == NULL )
    {
       printf("STAUD_OPGetCapability():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    strncpy (UserParams.DeviceName, DeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
    UserParams.OutputObject = OutputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPGETCAPABILITY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_OPGetCapability():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    *Capability_p=UserParams.OPCapability;
    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPSetSpeakerEnable(STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        STAUD_SpeakerEnable_t *SpeakerEnable_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetSpeakerEnable_t UserParams;

    if ( SpeakerEnable_p == NULL )
    {
       printf("STAUD_PPSetSpeakerEnable():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;
    UserParams.SpeakerEnable = *SpeakerEnable_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPSETSPEAKERENABLE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPSetSpeakerEnable():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *SpeakerEnable_p = UserParams.SpeakerEnable;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PPGetSpeakerEnable(STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        STAUD_SpeakerEnable_t *SpeakerEnable_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetSpeakerEnable_t UserParams;

    if ( SpeakerEnable_p == NULL )
    {
       printf("STAUD_PPGetSpeakerEnable():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPGETSPEAKERENABLE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPGetSpeakerEnable():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *SpeakerEnable_p = UserParams.SpeakerEnable;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPSetStereoModeNow(STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        STAUD_StereoMode_t StereoMode)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetStereoModeNow_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = PostProcObject;
    UserParams.StereoMode = StereoMode;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPSETSTEREOMODENOW, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPSetStereoModeNow():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPSetAttenuation(STAUD_Handle_t Handle,
                                      STAUD_Object_t PostProcObject,
                                      STAUD_Attenuation_t *Attenuation_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetAttenuation_t UserParams;

    if ( Attenuation_p == NULL )
    {
       printf("STAUD_PPSetAttenuation():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;
    UserParams.Attenuation = *Attenuation_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPSETATTENUATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPSetAttenuation():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Attenuation_p = UserParams.Attenuation;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_OPSetParams(STAUD_Handle_t Handle,
                                 STAUD_Object_t OutputObject,
                                 STAUD_OutputParams_t *Params_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPSetParams_t UserParams;

    if ( Params_p == NULL )
    {
       printf("STAUD_OPSetParams():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;
    UserParams.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPSETPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_OPSetParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Params_p = UserParams.Params;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_OPGetParams(STAUD_Handle_t Handle,
                                 STAUD_Object_t OutputObject,
                                 STAUD_OutputParams_t *Params_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPGetParams_t UserParams;

    if ( Params_p == NULL )
    {
       printf("STAUD_OPGetParams():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;
    UserParams.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPGETPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_OPGetParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Params_p = UserParams.Params;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPSetDelay(STAUD_Handle_t Handle,
                                STAUD_Object_t PostProcObject,
                                STAUD_Delay_t *Delay_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetDelay_t UserParams;

    if ( Delay_p == NULL )
    {
       printf("STAUD_PPSetDelay():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;
    UserParams.Delay = *Delay_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPSETDELAY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPSetDelay():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Delay_p = UserParams.Delay;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPGetDelay(STAUD_Handle_t Handle,
                                STAUD_Object_t PostProcObject,
                                STAUD_Delay_t *Delay_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetDelay_t UserParams;

    if ( Delay_p == NULL )
    {
       printf("STAUD_PPGetDelay():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPGETDELAY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("PPGetDelay():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Delay_p = UserParams.Delay;
    }

    return ErrorCode;

}

#if 0
ST_ErrorCode_t STAUD_PPSetEffect(STAUD_Handle_t Handle,
                                 STAUD_Object_t PostProcObject,
                                 STAUD_Effect_t Effect)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetEffect_t UserParams;

    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;
    UserParams.Effect = Effect;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPSETEFFECT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPSetEffect():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

ST_ErrorCode_t STAUD_PPGetEffect(STAUD_Handle_t Handle,
                                 STAUD_Object_t PostProcObject,
                                 STAUD_Effect_t *Effect_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetEffect_t UserParams;

    if ( Effect_p == NULL )
    {
       printf("STAUD_PPGetEffect():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPGETEFFECT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPGetEffect():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Effect_p = UserParams.Effect ;
    }

    return ErrorCode;
}
#endif

ST_ErrorCode_t STAUD_PPSetKaraoke(STAUD_Handle_t Handle,
                                  STAUD_Object_t PostProcObject,
                                  STAUD_Karaoke_t Karaoke)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetKaraoke_t UserParams;

    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;
    UserParams.Karaoke = Karaoke;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPSETKARAOKE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPSetKaraoke():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PPGetKaraoke(STAUD_Handle_t Handle,
                                  STAUD_Object_t PostProcObject,
                                  STAUD_Karaoke_t *Karaoke_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetKaraoke_t UserParams;

    if ( Karaoke_p == NULL )
    {
       printf("STAUD_PPGetKaraoke():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPGETKARAOKE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPGetKaraoke():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Karaoke_p = UserParams.Karaoke;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPSetSpeakerConfig(STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        STAUD_SpeakerConfiguration_t *
                                        SpeakerConfig_p, STAUD_BassMgtType_t BassType)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetSpeakerConfig_t UserParams;

    if ( SpeakerConfig_p == NULL )
    {
       printf("STAUD_PPSetSpeakerConfig():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;
    UserParams.SpeakerConfig = *SpeakerConfig_p;
    UserParams.BassType = BassType;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPSETSPEAKERCONFIG, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPSetSpeakerConfig():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *SpeakerConfig_p = UserParams.SpeakerConfig;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PPGetSpeakerConfig(STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        STAUD_SpeakerConfiguration_t *
                                        SpeakerConfig_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetSpeakerConfig_t UserParams;

    if ( SpeakerConfig_p == NULL )
    {
       printf("STAUD_PPGetSpeakerConfig():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPGETSPEAKERCONFIG, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPGetSpeakerConfig():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *SpeakerConfig_p = UserParams.SpeakerConfig;
    }

    return ErrorCode;

}

ST_ErrorCode_t  STAUD_IPSetDataInputInterface(STAUD_Handle_t Handle,
                                              STAUD_Object_t InputObject,
                                              GetWriteAddress_t   GetWriteAddress_p,
                                              InformReadAddress_t InformReadAddress_p,
                                                      void * const BufferHandle_p)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPSetDataInputInterface_t UserParams;
    AudCallbackMap_t* SubstituteBufferHandle = NULL;

    if ( BufferHandle_p == NULL )
    {
        printf("STAUD_IPSetDataInputInterface():NULL Ptr error\n");
        return ST_ERROR_BAD_PARAMETER;
    }

    if (GetWriteAddress_p == NULL || InformReadAddress_p == NULL)
    {
        /* PTI - both should be NULL */
        GetWriteAddress_p = NULL;
        InformReadAddress_p = NULL;
        UserParams.BufferHandle = BufferHandle_p;
    }
    else
    {
        /* Add the callbacks to the lookup table */
        int i;

        /* Overwrite previous if present */
        for (i=0; i<AUD_CBMAP_ELEMENTS; i++)
        {
            if (Handle == AudCallbackMap[i].Handle)
            {
                AudCallbackMap[i].GetWriteAddress_p = GetWriteAddress_p;
                AudCallbackMap[i].InformReadAddress_p = InformReadAddress_p;
		  AudCallbackMap[i].BufferHandle = BufferHandle_p;
                SubstituteBufferHandle = &AudCallbackMap[i];
                break;
            }
        }
        /* Not found: create new */
        if (i == AUD_CBMAP_ELEMENTS)
        {
            for (i=0; i<AUD_CBMAP_ELEMENTS; i++)
            {
                if (AudCallbackMap[i].Handle == 0)
                {
                    AudCallbackMap[i].Handle = Handle;
                    AudCallbackMap[i].GetWriteAddress_p = GetWriteAddress_p;
                    AudCallbackMap[i].InformReadAddress_p = InformReadAddress_p;
                    AudCallbackMap[i].BufferHandle = BufferHandle_p;
                    SubstituteBufferHandle = &AudCallbackMap[i];
                    break;
                }
            }
        }

        if (SubstituteBufferHandle == NULL)
        {
            printf("STAUD_IPSetDataInputInterface():NULL Ptr error\n");
            return ST_ERROR_NO_MEMORY;
        }

        UserParams.BufferHandle = (void*)SubstituteBufferHandle;
    }

    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;
    UserParams.GetWriteAddress = GetWriteAddress_p;
    UserParams.InformReadAddress = InformReadAddress_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPSETDATAINPUTINTERFACE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IPSetDataInputInterface():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPConnectSource(STAUD_Handle_t Handle,
                                     STAUD_Object_t PostProcObject,
                                     STAUD_Object_t InputSource)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPConnectSource_t UserParams;

    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;
    UserParams.InputSource = InputSource;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPCONNECTSOURCE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPConnectSource():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;


}

ST_ErrorCode_t STAUD_OPMute(STAUD_Handle_t Handle,
                            STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPMute_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPMUTE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_OPMute():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_OPUnMute(STAUD_Handle_t Handle,
                              STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPUnMute_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPUNMUTE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_UnOPMute():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PPGetCapability(const ST_DeviceName_t DeviceName,
                                     STAUD_Object_t PostProcObject,
                                     STAUD_PPCapability_t *Capability_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetCapability_t UserParams;

    if ( Capability_p == NULL )
    {
       printf("STAUD_UnOPMute():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    strncpy (UserParams.DeviceName, DeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
    UserParams.PostProcObject = PostProcObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPGETCAPABILITY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_UnOPMute():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Capability_p=UserParams.PPCapability;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_DisableDeEmphasis(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DisableDeEmphasis_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DISABLEDEEMPHASIS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DisableDeEmphasis():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_DisableSynchronisation(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DisableSynchronisation_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DISABLESYNCHRONISATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DisableSynchronisation():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_EnableDeEmphasis(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_EnableDeEmphasis_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_ENABLEDEEMPHASIS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_EnableDeEmphasis():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_EnableSynchronisation(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_EnableSynchronisation_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_ENABLESYNCHRONISATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_EnableSynchronisation():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_Resume(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Resume_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_RESUME, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Resume():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_SetPrologic(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetPrologic_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETPROLOGIC, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetPrologic():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}
 ST_ErrorCode_t STAUD_GetAttenuation(const STAUD_Handle_t Handle,
                                     STAUD_Attenuation_t *Attenuation)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetAttenuation_t UserParams;

    if ( Attenuation == NULL )
    {
       printf("STAUD_GetAttenuation():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETATTENUATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetAttenuation():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Attenuation = UserParams.Attenuation;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_GetChannelDelay(const STAUD_Handle_t Handle,
                                     STAUD_Delay_t *Delay)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetChannelDelay_t UserParams;

    if ( Delay == NULL )
    {
       printf("STAUD_GetChannelDelay():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETCHANNELDELAY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetChannelDelay():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *Delay = UserParams.Delay;
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_GetDigitalOutput(const STAUD_Handle_t Handle,
                                      STAUD_DigitalOutputConfiguration_t *
                                      Mode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetDigitalOutput_t UserParams;

    if ( Mode == NULL )
    {
       printf("STAUD_GetDigitalOutput():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETDIGITALOUTPUT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetDigitalOutput():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Mode = UserParams.Mode;
    }
    return ErrorCode;


}

ST_ErrorCode_t STAUD_GetDynamicRangeControl(const STAUD_Handle_t Handle,
                                            U8 *Control)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetDynamicRangeControl_t UserParams;

    if ( Control == NULL )
    {
       printf("STAUD_GetDynamicRangeControl():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETDYNAMICRANGECONTROL, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetDynamicRangeControl():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Control= UserParams.Control;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_Start(const STAUD_Handle_t Handle,
                           STAUD_StartParams_t *Params)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Start_t UserParams;

    if ( Params == NULL )
    {
       printf("STAUD_Start():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.Params = *Params;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_START, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Sart():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;





}

ST_ErrorCode_t STAUD_Mute(const STAUD_Handle_t Handle,
                          BOOL AnalogueOutput, BOOL DigitalOutput)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Mute_t UserParams;

    UserParams.Handle = Handle;
    UserParams.AnalogueOutput = AnalogueOutput;
    UserParams.DigitalOutput = DigitalOutput;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MUTE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Mute():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_GetEffect(const STAUD_Handle_t Handle,
                               STAUD_Effect_t *Mode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetEffect_t UserParams;

    if ( Mode == NULL )
    {
       printf("STAUD_GetEffect():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETEFFECT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetEffect():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Mode = UserParams.EffectMode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_GetKaraokeOutput(const STAUD_Handle_t Handle,
                                      STAUD_Karaoke_t *Mode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetKaraokeOutput_t UserParams;

    if ( Mode == NULL )
    {
       printf("STAUD_GetKaraokeOutput():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETKARAOKEOUTPUT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetKaraokeOutput():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Mode=UserParams.KaraokeMode;
    }
    return ErrorCode;


}

ST_ErrorCode_t STAUD_GetPrologic(const STAUD_Handle_t Handle,
                                 BOOL *PrologicState)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetPrologic_t UserParams;

    if ( PrologicState == NULL )
    {
       printf("STAUD_GetPrologic():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETPROLOGIC, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetPrologic():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *PrologicState = UserParams.PrologicState;
    }
    return ErrorCode;


}

ST_ErrorCode_t STAUD_GetSpeaker(const STAUD_Handle_t Handle,
                                 STAUD_Speaker_t *Speaker)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetSpeaker_t UserParams;

    if ( Speaker == NULL )
    {
       printf("STAUD_GetSpeaker():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETSPEAKER, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetSpeaker():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Speaker = UserParams.Speaker;
    }
    return ErrorCode;


}

ST_ErrorCode_t STAUD_GetSpeakerConfiguration(const STAUD_Handle_t Handle,
                                    STAUD_SpeakerConfiguration_t *Speaker)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetSpeakerConfiguration_t UserParams;

    if ( Speaker == NULL )
    {
       printf("STAUD_GetSpeakerConfiguration():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETSPEAKERCONFIGURATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetSpeakerConfiguration():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Speaker = UserParams.Speaker;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_GetStereoOutput(const STAUD_Handle_t Handle,
                                         STAUD_Stereo_t *Mode)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetStereoOutput_t UserParams;

    if ( Mode == NULL )
    {
       printf("STAUD_GetStereoOutput():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETSTEREOOUTPUT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetStereoOutput():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Mode = UserParams.Mode;
    }
    return ErrorCode;


}

ST_ErrorCode_t STAUD_GetSynchroUnit(STAUD_Handle_t Handle,
                                    STAUD_SynchroUnit_t *SynchroUnit_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetSynchroUnit_t UserParams;

    if ( SynchroUnit_p == NULL )
    {
       printf("STAUD_GetSynchroUnit():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETSYNCHROUNIT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetSynchroUnit():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *SynchroUnit_p = UserParams.SynchroUnit;
    }

    return ErrorCode;

}


ST_ErrorCode_t STAUD_Pause(const STAUD_Handle_t Handle,
                               STAUD_Fade_t *Fade)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Pause_t UserParams;

    if ( Fade == NULL )
    {
       printf("STAUD_Pause():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.Fade = *Fade;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PAUSE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Pause():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Fade = UserParams.Fade;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PauseSynchro(const STAUD_Handle_t Handle, U32 Unit)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PauseSynchro_t UserParams;

    UserParams.Handle = Handle;
    UserParams.Unit = Unit;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PAUSESYNCHRO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PauseSynchro():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

 ST_ErrorCode_t STAUD_Prepare(const STAUD_Handle_t Handle,
                                 STAUD_Prepare_t *Params)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Prepare_t UserParams;

    if ( Params == NULL )
    {
       printf("STAUD_Prepare():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;
    UserParams.Params = *Params;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PREPARE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Prepare():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Params = UserParams.Params;
    }

    return ErrorCode;
}
ST_ErrorCode_t STAUD_SetAttenuation(const STAUD_Handle_t Handle,
                                     STAUD_Attenuation_t Attenuation)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetAttenuation_t UserParams;
    UserParams.Attenuation = Attenuation;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETATTENUATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetAttenuation():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_SetChannelDelay(const STAUD_Handle_t Handle,
                                     STAUD_Delay_t Delay)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetChannelDelay_t UserParams;
    UserParams.Delay = Delay;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETCHANNELDELAY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetChannelDelay():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_SetDigitalOutput(const STAUD_Handle_t Handle,
                                      STAUD_DigitalOutputConfiguration_t Mode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetDigitalOutput_t UserParams;

    UserParams.Handle = Handle;
    UserParams.Mode = Mode;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETDIGITALOUTPUT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetDigitalOutput():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;


}

ST_ErrorCode_t STAUD_SetDynamicRangeControl(const STAUD_Handle_t Handle,
                                            U8 Control)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetDynamicRangeControl_t UserParams;

    UserParams.Handle = Handle;
    UserParams.Control = Control;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETDYNAMICRANGECONTROL, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetDynamicRangeControl():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_SetEffect(const STAUD_Handle_t Handle,
                               const STAUD_Effect_t Mode)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetEffect_t UserParams;

    UserParams.Handle = Handle;
    UserParams.EffectMode = Mode;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETEFFECT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetEffect():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}


ST_ErrorCode_t STAUD_SetKaraokeOutput(const STAUD_Handle_t Handle,
                                      STAUD_Karaoke_t Mode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetKaraokeOutput_t UserParams;

    UserParams.Handle = Handle;
    UserParams.KaraokeMode = Mode;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETKARAOKEOUTPUT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetKaraokeOutput():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}



ST_ErrorCode_t STAUD_SetSpeaker(const STAUD_Handle_t Handle,
                                 STAUD_Speaker_t Speaker)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Ioctl_SetSpeaker_t UserParams;
    UserParams.Speaker = Speaker;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETSPEAKER, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetSpeaker():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_SetSpeakerConfiguration(const STAUD_Handle_t Handle,
                                    STAUD_SpeakerConfiguration_t Speaker,STAUD_BassMgtType_t BassType)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetSpeakerConfiguration_t UserParams;

    UserParams.Handle = Handle;
    UserParams.Speaker = Speaker;
    UserParams.BassType = BassType;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETSPEAKERCONFIGURATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetSpeakerConfiguration():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_SetStereoOutput(const STAUD_Handle_t Handle,
                                     STAUD_Stereo_t Mode)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetStereoOutput_t UserParams;
    UserParams.Mode = Mode;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETSTEREOOUTPUT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetStereoOutput():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_Stop(const STAUD_Handle_t Handle,
                          const STAUD_Stop_t StopMode,
                          STAUD_Fade_t *Fade)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_Stop_t UserParams;

    if ( Fade == NULL )
    {
       printf("STAUD_Stop():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.StopMode = StopMode;
    UserParams.Fade = *Fade;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_STOP, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Stop():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Fade = UserParams.Fade;
    }
    return ErrorCode;

}

ST_ErrorCode_t STAUD_GetInputBufferParams(STAUD_Handle_t Handle,
            STAUD_AudioPlayerID_t PlayerID,
            STAUD_BufferParams_t* DataParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetInputBufferParams_t UserParams;


    if ( DataParams_p == NULL )
    {
       printf("STAUD_GetInputBufferParams():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PlayerID = PlayerID;
    UserParams.DataParams = *DataParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETINPUTBUFFERPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetInputBufferParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *DataParams_p = UserParams.DataParams;
    }
    return ErrorCode;

}

ST_ErrorCode_t AUD_IntHandlerInstall(int InterruptNumber, int InterruptLevel)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IntHandlerInstall_t UserParams;


    UserParams.InterruptNumber = InterruptNumber;
    UserParams.InterruptLevel = InterruptLevel;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_INTHANDLERINSTALL, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("AUD_IntHandlerInstall():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_SkipSynchro(STAUD_Handle_t Handle,
                                 STAUD_AudioPlayerID_t PlayerID,
                                 U32 Delay)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SkipSynchro_t UserParams;


    UserParams.Handle = Handle;
    UserParams.PlayerID = PlayerID;
    UserParams.Delay = Delay;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SKIPSYNCHRO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SkipSynchro():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetDeEmphasisFilter(STAUD_Handle_t Handle,
                                           STAUD_Object_t DecoderObject,
                                           BOOL Emphasis)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetDeEmphasisFilter_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.Emphasis = Emphasis;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETDEEMPHASISFILTER, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetDeEmphasisFilter():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;


}

ST_ErrorCode_t STAUD_DRGetDeEmphasisFilter(STAUD_Handle_t Handle,
                                           STAUD_Object_t DecoderObject,
                                           BOOL *Emphasis_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetDeEmphasisFilter_t UserParams;

    if ( Emphasis_p == NULL )
    {
       printf("STAUD_DRGetDeEmphasisFilter():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETDEEMPHASISFILTER, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetDeEmphasisFilter():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Emphasis_p = UserParams.Emphasis;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetEffect(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 STAUD_Effect_t Effect)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetEffect_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.Effect = Effect;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETEFFECT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetEffect():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetEffect(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 STAUD_Effect_t *Effect_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetEffect_t UserParams;

    if ( Effect_p == NULL )
    {
       printf("STAUD_DRGetEffect():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETEFFECT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetEffect():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Effect_p = UserParams.Effect ;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetSRSEffectParams (STAUD_Handle_t Handle,
						STAUD_Object_t DecoderObject, STAUD_EffectSRSParams_t ParamType,S16 *Value_p)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetSrsEffectParam_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.ParamType = ParamType;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETSRSEFFECTPARAM, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetSRSEffectParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
	*Value_p = UserParams.Value_p;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_DRSetPrologic(STAUD_Handle_t Handle,
                                   STAUD_Object_t DecoderObject,
                                   BOOL Prologic)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetPrologic_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.Prologic = Prologic;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETPROLOGIC, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetPrologic():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetPrologic(STAUD_Handle_t Handle,
                                   STAUD_Object_t DecoderObject,
                                   BOOL *Prologic_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetPrologic_t UserParams;

    if ( Prologic_p == NULL )
    {
       printf("STAUD_DRGetPrologic():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETPROLOGIC, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetPrologic():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Prologic_p = UserParams.Prologic ;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_DRSetStereoMode(STAUD_Handle_t Handle,
                                     STAUD_Object_t DecoderObject,
                                     STAUD_StereoMode_t StereoMode)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetStereoMode_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.StereoMode = StereoMode;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETSTEREOMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetStereoMode():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_DRGetStereoMode(STAUD_Handle_t Handle,
                                     STAUD_Object_t DecoderObject,
                                     STAUD_StereoMode_t *StereoMode_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetStereoMode_t UserParams;

    if ( StereoMode_p == NULL )
    {
       printf("STAUD_DRGetStereoMode():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETSTEREOMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetStereoMode():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *StereoMode_p=UserParams.StereoMode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetInStereoMode(STAUD_Handle_t Handle,
                                     STAUD_Object_t DecoderObject,
                                     STAUD_StereoMode_t *StereoMode_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetInStereoMode_t UserParams;

    if ( StereoMode_p == NULL )
    {
       printf("STAUD_DRGetInStereoMode():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETINSTEREOMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetInStereoMode():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *StereoMode_p=UserParams.StereoMode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRMute(STAUD_Handle_t Handle,
                            STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRMute_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRMUTE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRMute():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_DRUnMute(STAUD_Handle_t Handle,
                              STAUD_Object_t OutputObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRUnMute_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRUNMUTE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRUnMute():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_PlayerStart(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PlayerStart_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PLAYERSTART, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_PlayerStart():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PlayerStop(const STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PlayerStop_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PLAYERSTOP, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;

        printf(" STAUD_IOCTL_PlayerStop():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PPDcRemove(STAUD_Handle_t Handle, STAUD_Object_t PPObject, STAUD_DCRemove_t *Params_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPDcRemove_t UserParams;

    if ( Params_p == NULL )
    {
       printf("STAUD_PPDcRemove():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;
    UserParams.PPObject = PPObject;
    UserParams.Params = *Params_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPDCREMOVE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPDcRemove():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Params_p=UserParams.Params;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetCompressionMode (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_CompressionMode_t CompressionMode_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetCompressionMode_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.CompressionMode= CompressionMode_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETCOMPRESSIONMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetCompressionMode():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;


}

ST_ErrorCode_t STAUD_DRSetAudioCodingMode (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_AudioCodingMode_t AudioCodingMode_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetAudioCodingMode_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.AudioCodingMode= AudioCodingMode_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETAUDIOCODINGMODE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetAudioCodingMode():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;


}

ST_ErrorCode_t STAUD_IPSetPCMParams (STAUD_Handle_t Handle,
		STAUD_Object_t InputObject, STAUD_PCMInputParams_t * InputParams_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPSetPcmParams_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject= InputObject;
    UserParams.InputParams = *InputParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPSETPCMPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IPSetPCMParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;


}

ST_ErrorCode_t STAUD_IPGetBitBufferFreeSize (STAUD_Handle_t Handle,
		STAUD_Object_t InputObject, U32  * Size_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetBitBufferFreeSize_t UserParams;

    if ( Size_p == NULL )
    {
       printf("STAUD_IPGetBitBufferFreeSize():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;
    UserParams.InputObject= InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETBITBUFFERFREESIZE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IPGetBitBufferFreeSize():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
		*Size_p = UserParams.Size;
    }

    return ErrorCode;


}

ST_ErrorCode_t STAUD_IPSetPCMReaderParams (STAUD_Handle_t Handle,
		STAUD_Object_t InputObject, STAUD_PCMReaderConf_t   * InputParams_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPSetPCMReaderParams_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject= InputObject;
    UserParams.ReaderParams = *InputParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPSETPCMREADERPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IPSetPCMReaderParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPGetPCMReaderParams (STAUD_Handle_t Handle,
		STAUD_Object_t InputObject, STAUD_PCMReaderConf_t   * InputParams_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetPCMReaderParams_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject= InputObject;
    UserParams.PCMReaderParams = *InputParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETPCMREADERPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IPGetPCMReaderParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetDownMix (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,STAUD_DownmixParams_t * DownMixParam_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetDownMix_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.DownMixParam = *DownMixParam_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETDOWNMIX, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetDownMix():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_DRSetSRSEffectParams (STAUD_Handle_t Handle,
								STAUD_Object_t DecoderObject,
								STAUD_EffectSRSParams_t ParamsType,S16 Value)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetSrsEffect_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.ParamsType = ParamsType;
    UserParams.Value= Value;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETSRSEFFECT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetSRSEffectParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}


ST_ErrorCode_t STAUD_MXUpdatePTSStatus (STAUD_Handle_t Handle,
								STAUD_Object_t MixerObject,
								U32 InputID,
								BOOL PTSStatus)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_IOCTL_MXUpdatePTSStatus_t UserParams;

    UserParams.Handle = Handle;
    UserParams.MixerObject= MixerObject;
    UserParams.InputID= InputID;
    UserParams.PTSStatus= PTSStatus;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MXUPDATEPTSSTATUS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_StaudMXUpdatePTSstatus():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}


ST_ErrorCode_t STAUD_MapBufferToUserSpace(STAUD_BufferParams_t* DataParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    off_t pgoffset = (off_t)(DataParams_p->BufferBaseAddr_p) % getpagesize();
    void *address, *kaddr = DataParams_p->BufferBaseAddr_p;

    address = mmap(0, DataParams_p->BufferSize + pgoffset, PROT_READ | PROT_WRITE, MAP_SHARED,
                   fd, (off_t)DataParams_p->BufferBaseAddr_p - pgoffset);

printf("mmap(0, 0x%08x, 0x%02x, %d, %d, 0x%08x) => 0x%08x\n", (U32)(DataParams_p->BufferSize + pgoffset),
 PROT_READ | PROT_WRITE, MAP_SHARED, fd, (U32)((off_t)DataParams_p->BufferBaseAddr_p - pgoffset), (U32)address);

    if (address == MAP_FAILED)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        int i;
        DataParams_p->BufferBaseAddr_p = address + pgoffset;

        /* Create mapping lookup for callbacks */
        for (i=0; i<AUD_PESMAP_ELEMENTS; i++)
        {
            if (AudPESMap[i].size == 0)
            {
                AudPESMap[i].size = DataParams_p->BufferSize;
                AudPESMap[i].kaddr = kaddr;
                AudPESMap[i].uaddr = DataParams_p->BufferBaseAddr_p;
                break;
            }
        }
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_UnmapBufferFromUserSpace(STAUD_BufferParams_t* DataParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    off_t pgoffset = (off_t)(DataParams_p->BufferBaseAddr_p) % getpagesize();
    int i;

    /* Delete mapping lookup for callbacks */
    for (i=0; i<AUD_PESMAP_ELEMENTS; i++)
    {
        if (AudPESMap[i].size == DataParams_p->BufferSize &&
            AudPESMap[i].uaddr == DataParams_p->BufferBaseAddr_p)
        {
            AudPESMap[i].size = 0;
            AudPESMap[i].kaddr = NULL;
            AudPESMap[i].uaddr = NULL;
		printf("STAUD_UnmapBufferFromUserSpace - Done \n");
            break;
        }
    }

	if(i==AUD_PESMAP_ELEMENTS)
	{
		printf("Error -STAUD_UnmapBufferFromUserSpace \n");
	}

    if (munmap(DataParams_p->BufferBaseAddr_p - pgoffset, DataParams_p->BufferSize + pgoffset) == -1)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_MapPCMBufferToUserSpace(STAUD_PCMBuffer_t *PCMBuffer_p)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	off_t pgoffset = (off_t)(PCMBuffer_p->StartOffset) % getpagesize();
	void *address, *kaddr = (void*)PCMBuffer_p->StartOffset;

	address = mmap(0, AudPCMMap.TotalBufferSize + pgoffset, PROT_READ | PROT_WRITE, MAP_SHARED,
	               fd, (off_t)PCMBuffer_p->StartOffset - pgoffset);

	printf("*** PCMBuffer_p->StartOffset = %x \n",PCMBuffer_p->StartOffset);

	printf("mmap(0, 0x%08x, 0x%02x, %d, %d, 0x%08x) => 0x%08x\n", (U32)(AudPCMMap.TotalBufferSize + pgoffset),
	 PROT_READ | PROT_WRITE, MAP_SHARED, fd, (U32)((off_t)PCMBuffer_p->StartOffset - pgoffset), (U32)address);

	if (address == MAP_FAILED)
	{
		ErrorCode = ST_ERROR_BAD_PARAMETER;
		printf("STAUD_MapPCMBufferToUserSpace FAILED \n");
	}
	else
	{
		PCMBuffer_p->StartOffset = (U32)address + pgoffset;

		/* Create mapping lookup */
		AudPCMMap.kaddr = kaddr;
		AudPCMMap.uaddr = (void*)PCMBuffer_p->StartOffset;
		printf("STAUD_MapPCMBufferToUserSpace SUCCESS \n");
	}

	return ErrorCode;
}


ST_ErrorCode_t STAUD_SetClockRecoverySource  (STAUD_Handle_t Handle,STCLKRV_Handle_t ClkSource)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	STAUD_Ioctl_SetClockRecoverySource_t UserParams;
	UserParams.Handle = Handle;
	UserParams.ClkSource= ClkSource;
	/* IOCTL the command */
	if (ioctl(fd, STAUD_IOC_SETCLOCKRECOVERYSOURCE, &UserParams) != 0)
		{
		perror(strerror(errno));
		/* IOCTL failed */
		ErrorCode = ST_ERROR_BAD_PARAMETER;
		printf("STAUD_SetClockRecoverySource():Ioctl error\n");
		}
	else
		{
		/* IOCTL is successfull: retrieve Error code */
		ErrorCode = UserParams.ErrorCode;
		}
	return ErrorCode;
}


ST_ErrorCode_t STAUD_OPEnableHDMIOutput (STAUD_Handle_t Handle,STAUD_Object_t OutputObject)

{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	STAUD_Ioctl_OPEnableHDMIOutput_t UserParams;
	UserParams.Handle = Handle;
	UserParams.OutputObject = OutputObject;
	/* IOCTL the command */
	if (ioctl(fd, STAUD_IOC_OPENABLEHDMIOUTPUT, &UserParams) != 0)
		{
		perror(strerror(errno));
		/* IOCTL failed */
		ErrorCode = ST_ERROR_BAD_PARAMETER;
		printf("STAUD_OPEnableHDMIOutput():Ioctl error\n");
		}
	else
		{
		/* IOCTL is successfull: retrieve Error code */
		ErrorCode = UserParams.ErrorCode;
		}
	return ErrorCode;
}

ST_ErrorCode_t STAUD_OPDisableHDMIOutput (STAUD_Handle_t Handle,STAUD_Object_t OutputObject)

{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	STAUD_Ioctl_OPDisableHDMIOutput_t UserParams;
	UserParams.Handle = Handle;
	UserParams.OutputObject = OutputObject;
	/* IOCTL the command */
	if (ioctl(fd, STAUD_IOC_OPDISABLEHDMIOUTPUT, &UserParams) != 0)
		{
		perror(strerror(errno));
		/* IOCTL failed */
		ErrorCode = ST_ERROR_BAD_PARAMETER;
		printf("STAUD_OPDisableHDMIOutput():Ioctl error\n");
		}
	else
		{
		/* IOCTL is successfull: retrieve Error code */
		ErrorCode = UserParams.ErrorCode;
		}
	return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetBeepToneFrequency (STAUD_Handle_t Handle,
														STAUD_Object_t DecoderObject,
														U32 * BeepToneFreq_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetBeepToneFreq_t UserParams;

    if ( BeepToneFreq_p == NULL  )
    {
       printf("STAUD_DRGetBeepToneFrequency():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETBEEPTONEFREQ, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_DRGetBeepToneFrequency():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
	*BeepToneFreq_p = UserParams.Freq;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetBeepToneFrequency (STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,U32 BeepToneFrequency)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetBeepToneFreq_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.BeepToneFrequency = BeepToneFrequency;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETBEEPTONEFREQ, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetBeepToneFrequency():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_MXSetMixLevel (STAUD_Handle_t Handle,STAUD_Object_t MixerObject,
										U32 InputID, U16 MixLevel)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_MXSetMixerLevel_t UserParams;

    UserParams.Handle = Handle;
    UserParams.MixerObject = MixerObject;
    UserParams.InputID = InputID;
    UserParams.MixLevel = MixLevel;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MXSETMIXERLEVEL, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_MXSetMixLevel():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPSetBroadcastProfile (STAUD_Handle_t Handle, STAUD_Object_t InputObject,
												STAUD_BroadcastProfile_t BroadcastProfile)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPSetBroadcastProfile_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject= InputObject;
    UserParams.BroadcastProfile= BroadcastProfile;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPSETBROADCASTPROFILE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IPSetBroadcastProfile():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}


ST_ErrorCode_t STAUD_DRSetDolbyDigitalEx(STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject, BOOL DolbyDigitalEx)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetDolbyDigitalEx_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.DolbyDigitalEx =DolbyDigitalEx;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETDOLBYDIGITALEX, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetDolbyDigitalEx():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}




ST_ErrorCode_t STAUD_DRGetDolbyDigitalEx(STAUD_Handle_t Handle,
	STAUD_Object_t DecoderObject,BOOL * DolbyDigitalEx_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetDolbyDigitalEx_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;


    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETDOLBYDIGITALEX, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetDolbyDigitalEx():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
 	*DolbyDigitalEx_p = UserParams.DolbyDigitalEx_p;
    }

    return ErrorCode;
}



ST_ErrorCode_t STAUD_PPGetEqualizationParams(STAUD_Handle_t Handle,
							   STAUD_Object_t PostProcObject,
							   STAUD_Equalization_t *Equalization_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetEqualizationParams_t UserParams;

    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;


    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPGETEQUALIZATIONPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPGetEqualizationParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
	 *Equalization_p=UserParams.Equalization_p;
    }

    return ErrorCode;
}



ST_ErrorCode_t STAUD_PPSetEqualizationParams(STAUD_Handle_t Handle,
							   STAUD_Object_t PostProcObject,
							   STAUD_Equalization_t *Equalization_p)


{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetEqualizationParams_t UserParams;

    UserParams.Handle = Handle;
    UserParams.PostProcObject = PostProcObject;
    UserParams.Equalization_p =*Equalization_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_PPSETEQUALIZATIONPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_PPSetEqualizationParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}




ST_ErrorCode_t STAUD_DRSetOmniParams (STAUD_Handle_t Handle,
						  STAUD_Object_t DecoderObject,
						  STAUD_Omni_t Omni)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetOmniParams_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.Omni=Omni;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETOMNIPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetOmniParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}



ST_ErrorCode_t STAUD_DRGetOmniParams (STAUD_Handle_t Handle,
						  STAUD_Object_t DecoderObject,
						  STAUD_Omni_t * Omni_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetOmniParams_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject= DecoderObject;


    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETOMNIPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetOmniParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
	 *Omni_p=UserParams.Omni_p;
    }

    return ErrorCode;
}



ST_ErrorCode_t STAUD_ModuleStart (STAUD_Handle_t Handle,
STAUD_Object_t ModuleObject,
STAUD_StreamParams_t * StreamParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Ioctl_ModuleStart_t UserParams;

    UserParams.Handle = Handle;
    UserParams.ModuleObject= ModuleObject;
    UserParams.StreamParams_p=*StreamParams_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MODULESTART,&UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_ModuleStart():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}




ST_ErrorCode_t STAUD_GenericStart (STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Ioctl_GenericStart_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GENERICSTART, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GenericStart():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}




ST_ErrorCode_t STAUD_UnmapPCMBufferFromUserSpace(void)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	off_t pgoffset = (off_t)(AudPCMMap.uaddr) % getpagesize();

	if (munmap(AudPCMMap.uaddr - pgoffset, AudPCMMap.TotalBufferSize + pgoffset) == -1)
	{
	    	ErrorCode = ST_ERROR_BAD_PARAMETER;
		printf("STAUD_UNMapPCMBufferFromUserSpace FAILED \n");
	}
	else
	{
		printf("STAUD_UnmapPCMBufferFromUserSpace SUCCESS \n");
	}

	//AudPCMMap.TotalBufferSize = 0;
	AudPCMMap.kaddr = NULL;
	AudPCMMap.uaddr = NULL;

    return ErrorCode;
}


ST_ErrorCode_t STAUD_ModuleStop (STAUD_Handle_t Handle,
STAUD_Object_t ModuleObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Ioctl_ModuleStop_t UserParams;

    UserParams.Handle = Handle;
    UserParams.ModuleObject= ModuleObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MODULESTOP,&UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_ModuleStop():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}




ST_ErrorCode_t STAUD_GenericStop (STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Ioctl_GenericStop_t UserParams;

    UserParams.Handle = Handle;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GENERICSTOP, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GenericStop():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}


ST_ErrorCode_t STAUD_MXUpdateMaster(STAUD_Handle_t Handle,
STAUD_Object_t MixerObject,
U32 InputID, BOOL FreeRun)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Ioctl_MXUpdateMaster_t UserParams;

    UserParams.Handle = Handle;
    UserParams.MixerObject= MixerObject;
    UserParams.InputID= InputID;
	 UserParams.FreeRun = FreeRun;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_MXUPDATEMASTER, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_MXUpdateMaster():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_OPSetLatency (STAUD_Handle_t Handle,
					STAUD_Object_t OutputObject, U32 Latency)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPSetLatency_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;
    UserParams.Latency = Latency;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPSETLATENCY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_OPSetLatency():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetPrologicAdvance (STAUD_Handle_t Handle,
					STAUD_Object_t DecoderObject, STAUD_PLIIParams_t PLIIParams)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetAdvancePLII_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.PLIIParams = PLIIParams;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETADVPLII, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetPrologicAdvance():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

 ST_ErrorCode_t STAUD_DRGetPrologicAdvance (STAUD_Handle_t Handle,
				STAUD_Object_t DecoderObject, STAUD_PLIIParams_t *PLIIParams)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetAdvancePLII_t UserParams;

    if ( PLIIParams == NULL )
    {
       printf("STAUD_DRGetPrologicAdvance():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.Handle = Handle;
    UserParams.DecoderObject= DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETADVPLII, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetPrologicAdvance():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    *PLIIParams = UserParams.PLIIParams;

    return ErrorCode;

}

 ST_ErrorCode_t STAUD_IPGetSynchroUnit(STAUD_Handle_t Handle,
                                    STAUD_Object_t InputObject, STAUD_SynchroUnit_t *SynchroUnit_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPGetSynchroUnit_t UserParams;

    if ( SynchroUnit_p == NULL )
    {
       printf("STAUD_GetSynchroUnit():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


	 UserParams.Handle = Handle;
	 UserParams.inputObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPGETSYNCHROUNIT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IPGetSynchroUnit():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *SynchroUnit_p = UserParams.SynchroUnit;
    }

    return ErrorCode;

}

ST_ErrorCode_t STAUD_OPSetEncDigitalOutput(const STAUD_Handle_t Handle,
				STAUD_Object_t OutputObject,
				BOOL EnableDisableEncOutput)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_OPEncSetDigital_t UserParams;

    UserParams.Handle = Handle;
    UserParams.OutputObject = OutputObject;
    UserParams.EnableDisableEncOutput= EnableDisableEncOutput;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_OPENCSETDIGITAL, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_OPSetEncDigitalOutput():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetCircleSurroundParams(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 STAUD_CircleSurrondII_t Csii)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetCircleSrround_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.Csii = Csii;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETCIRCLE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetCircleSurroundParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetCircleSurroundParams(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 STAUD_CircleSurrondII_t *Csii)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetCircleSrround_t UserParams;

    if ( Csii == NULL )
    {
       printf("STAUD_DRGetCircleSurroundParams():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETCIRCLE, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRGetCircleSurroundParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Csii = UserParams.Csii ;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_IPHandleEOF(STAUD_Handle_t Handle,
                                 STAUD_Object_t InputObject)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_IPHandleEOF_t UserParams;

    UserParams.Handle = Handle;
    UserParams.InputObject = InputObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_IPHANDLEEOF, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IPHandleEOF():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRSetDDPOP(STAUD_Handle_t Handle,
                                 STAUD_Object_t DecoderObject,
                                 U32 DDPOPSetting)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetDDPOP_t UserParams;

    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.DDPOPSetting = DDPOPSetting;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETDDPOP, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DRSetDDPOP():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_ConnectSrcDst (STAUD_Handle_t Handle,
													STAUD_Object_t SrcObject,U32 SrcId,
													STAUD_Object_t DstObject,U32 DstId)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_ConnectSrcDst_t UserParams;

    UserParams.Handle = Handle;
    UserParams.SrcObject = SrcObject;
    UserParams.SrcId = SrcId;
	 UserParams.DstObject = DstObject;
    UserParams.DstId = DstId;


    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_CONNECTSRCDST, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_ConnectSrcDst():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DisconnectInput(STAUD_Handle_t Handle,
                                      STAUD_Object_t Object,U32 InputId)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DisconnectInput_t UserParams;

    UserParams.Handle = Handle;
    UserParams.Object = Object;
    UserParams.InputId = InputId;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DISCONNECTINPUT, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_DisconnectInput():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_ENCGetCapability(const ST_DeviceName_t DeviceName,
                                        STAUD_Object_t EncoderObject,
                                        STAUD_ENCCapability_t *Capability_p)
{
	ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    STAUD_Ioctl_ENCGetCapability_t UserParams;

	if ( Capability_p == NULL  )
    {
       printf("STAUD_IOCTL_ENCGetCapability_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.EncoderObject=EncoderObject;
    strncpy (UserParams.DeviceName, DeviceName,ST_MAX_DEVICE_NAME_TOCOPY);
    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_ENCGETCAPABILITY, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_ENCGetCapability_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *Capability_p = UserParams.Capability;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}

/*****************************************************************************
STAUD_ENCSetOutputParams()

Description:
Gets output parameters of audio encoder.



Parameters   :
Handle				Handle to audio Object.
EncoderObject		References the particular encoder object.
EncoderOPParams		Encoder output params to be set to a particular audio encoder.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_ENCSetOutputParams(STAUD_Handle_t Handle,
                                STAUD_Object_t EncoderObject,
                                STAUD_ENCOutputParams_s EncoderOPParams)
{
	ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
	STAUD_Ioctl_ENCSetOutputParams_t UserParams;

    UserParams.EncoderObject=EncoderObject;
    UserParams.Handle=Handle;
    UserParams.EncoderOPParams=EncoderOPParams;
    UserParams.ErrorCode = ST_NO_ERROR;
    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_ENCSETOUTPUTPARAMS,UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_Ioctl_ENCSetOutputParams_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
     /*   EncoderOPParams = UserParams.EncoderOPParams;*/
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}
/*****************************************************************************
STAUD_ENCGetOutputParams()

Description:
Gets output parameters of audio encoder.



Parameters   :
Handle				Handle to audio Object.
EncoderObject		References the particular encoder object.
EncoderOPParams_p	Pointer to the encoder output params to be returned.

Return             Value.

ST_NO_ERROR                         No error.

ST_ERROR_INVALID_HANDLE             The handle is not valid.

STAUD_ERROR_INVALID_DEVICE          The audio decoder is unrecognised or not avail-able
with the current configuration.
See Also:
*****************************************************************************/
ST_ErrorCode_t STAUD_ENCGetOutputParams(STAUD_Handle_t Handle,
                                STAUD_Object_t EncoderObject,
                                STAUD_ENCOutputParams_s *EncoderOPParams_p)
{
	ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
	STAUD_Ioctl_ENCGetOutputParams_t UserParams;

	if ( EncoderOPParams_p == NULL  )
    {
       printf("STAUD_Ioctl_ENCGetOutputParams_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }

    UserParams.EncoderObject=EncoderObject;
    UserParams.Handle=Handle;
    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_ENCGETOUTPUTPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_Ioctl_ENCGetOutputParams_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *EncoderOPParams_p = UserParams.EncoderOPParams;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}


ST_ErrorCode_t STAUD_GetModuleAttenuation(STAUD_Handle_t Handle,
                                      STAUD_Object_t ModuleObject,
                                      STAUD_Attenuation_t *Attenuation_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_GetModuleAttenuation_t UserParams;

    if ( Attenuation_p == NULL )
    {
       printf("STAUD_GetModuleAttenuation():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.Object = ModuleObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_GETMODULEATTENUATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_GetModuleAttenuation():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }

    *Attenuation_p=UserParams.Attenuation;
    return ErrorCode;

}


ST_ErrorCode_t STAUD_SetModuleAttenuation(STAUD_Handle_t Handle,
                                      STAUD_Object_t ModuleObject,
                                      STAUD_Attenuation_t *Attenuation_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetModuleAttenuation_t UserParams;

    if ( Attenuation_p == NULL )
    {
       printf("STAUD_SetModuleAttenuation():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.Object = ModuleObject;
    UserParams.Attenuation = *Attenuation_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETMODULEATTENUATION, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetModuleAttenuation():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
        *Attenuation_p = UserParams.Attenuation;
    }

    return ErrorCode;
}

 ST_ErrorCode_t STAUD_DRSetDTSNeoParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t DecoderObject,
                                          STAUD_DTSNeo_t *DTSNeo_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetDTSNeo_t UserParams;

    if ( DTSNeo_p == NULL )
    {
       printf("STAUD_Ioctl_DRSetDTSNeo_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
    UserParams.DTSNeo = *DTSNeo_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETDTSNEO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Ioctl_DRSetDTSNeo_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;

    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetDTSNeoParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t DecoderObject,
                                          STAUD_DTSNeo_t *DTSNeo_p)
 {
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetDTSNeo_t UserParams;

    if ( DTSNeo_p == NULL )
    {
       printf("STAUD_Ioctl_DRGetDTSNeo_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETDTSNEO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Ioctl_DRGetDTSNeo_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
	*DTSNeo_p=UserParams.DTSNeo;
    }


    return ErrorCode;

}

/*****************************************************************************
STAUD_DRSetInitParams()

-Description:
Set decoder specific parameters

-Parameters:
Handle              Handle to audio driver.

DecoderObject       References a specific audio decoder.

InitParams Values of init params to be set.

-Return Value:
ST_NO_ERROR                 No error.

ST_ERROR_INVALID_HANDLE     The handle is not valid.

STAUD_ERROR_INVALID_DEVICE  The audio decoder is unrecognised or not avail-able
with the current configuration.

See Also:
*****************************************************************************/

ST_ErrorCode_t STAUD_DRSetInitParams (STAUD_Handle_t Handle,
STAUD_Object_t DecoderObject, STAUD_DRInitParams_t * InitParams)
{

	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRSetInitParams_t UserParams;

    if ( InitParams == NULL )
    {
       printf("STAUD_SetDRInitParams_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;
	UserParams.DRInitParams = * InitParams;
    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETINITPARAMS, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_SetDRInitParams():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;

    }


    return ErrorCode;


}





 ST_ErrorCode_t STAUD_PPSetBTSCParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t PostProObject,
                                          STAUD_BTSC_t *BTSC_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPSetBTSC_t UserParams;

    if ( BTSC_p == NULL )
    {
       printf("STAUD_Ioctl_PPSetBTSC_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PPObject = PostProObject;
    UserParams.BTSC= *BTSC_p;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRSETBTSC, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Ioctl_PPSetBTSC_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;

    }

    return ErrorCode;
}

ST_ErrorCode_t STAUD_PPGetBTSCParams(STAUD_Handle_t Handle,
                                          STAUD_Object_t  PostProObject,
                                          STAUD_BTSC_t *BTSC_p)
 {
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_PPGetBTSC_t UserParams;

    if ( BTSC_p == NULL )
    {
       printf("STAUD_Ioctl_PPGetBTSC_t():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.PPObject= PostProObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETBTSC, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_Ioctl_PPGetBTSC_t():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
	*BTSC_p=UserParams.BTSC;
    }


    return ErrorCode;

}
ST_ErrorCode_t STAUD_SetScenario(STAUD_Handle_t Handle, STAUD_Scenario_t Scenario)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_SetScenario_t UserParams;

    UserParams.Handle = Handle;
    UserParams.Scenario = Scenario;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_SETSCENARIO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf("STAUD_IOC_SETSCENARIO:Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        ErrorCode = UserParams.ErrorCode;
    }
    return ErrorCode;
}

ST_ErrorCode_t STAUD_DRGetStreamInfo(STAUD_Handle_t Handle,
                                      STAUD_Object_t DecoderObject,
                                      STAUD_StreamInfo_t *StreamInfo_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Ioctl_DRGetStreamInfo_t UserParams;

    if ( StreamInfo_p == NULL  )
    {
       printf("STAUD_IOCTL_DRGetSreamInfo():NULL Ptr error\n");
       return ST_ERROR_BAD_PARAMETER;
    }


    UserParams.Handle = Handle;
    UserParams.DecoderObject = DecoderObject;

    /* IOCTL the command */
    if (ioctl(fd, STAUD_IOC_DRGETSTREAMINFO, &UserParams) != 0)
    {
        perror(strerror(errno));

        /* IOCTL failed */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        printf(" STAUD_IOCTL_DRGetSreamInfo():Ioctl error\n");
    }
    else
    {
        /* IOCTL is successfull: retrieve Error code */
        *StreamInfo_p=UserParams.StreamInfo;
        ErrorCode = UserParams.ErrorCode;
    }

    return ErrorCode;
}
