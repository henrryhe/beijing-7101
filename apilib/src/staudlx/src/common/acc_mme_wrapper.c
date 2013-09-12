
/***************************************************************

This file is the wrapper over MME calls _MME_WRAPPER_
_ACC_WRAPP_MME_  can be set to 1 to enable logging else 0 by default
This is the log of the commands sent to Lx for debugging

***************************************************************/

#ifndef _MME_WRAPPER_
    #define _MME_WRAPPER_

    #define FORCEFULLTERM_LINUX 0
    //#define  STTBX_PRINT
    #include "stos.h"
    #include "acc_mme.h"

    #if _ACC_WRAPP_MME_
        #define RAM2_BASE_ADDRESS		0xB0000000 /* LMI_VID */
        #ifndef ST_OSLINUX
            #include <stdio.h>
            #include <string.h>
            #include <os21.h>
            #include <os21/mutex.h>

            FILE * usb_fout;

            #ifdef _USB_WRITE_
                #include "stsys.h"

                #ifdef _USB_BUF_
                    int  usb_bs = (_USB_BUF_ * 1024);
                    char usb_buffer[((_USB_BUF_ >= 512)? _USB_BUF_ : 512) * 1024];
                    int usb_i  = 0;

                    #ifdef _USB_NONSTOP_  // open the log file only once and close it on user request only
                        #define DUMP(n, f)  if ((usb_i + (n)) >= usb_bs) { SYS_FWrite(usb_buffer, usb_i, 1, f); usb_i = 0;}
                        #define FWRITE(p,s,n,f) DUMP(s*n, f); memcpy(&usb_buffer[usb_i], p, s *n); usb_i += s*n //  SYS_FWrite
                        #define FOPEN(n,p) usb_fout //SYS_FOpen(n,p);
                        #define FFOPEN(n,p) SYS_FOpen(n,p);
                        #define FCLOSE(f)  f = NULL //SYS_FClose(f) //SYS_FWrite(usb_buffer, usb_i, 1, f);
                        #define FPUTC(c,f) DUMP(1, f); usb_buffer[usb_i] = c; usb_i++ //   SYS_FWrite(&c, 1, 1, f)
                        char  CMD_LOG_FILE[256] = "/USBHDISK00/usb.log";
                    #else  //!_USB_NONSTOP_ : reopen the out file at each log
                        #define DUMP(n, f)  if ((usb_i + (n)) >= usb_bs) { SYS_FWrite(usb_buffer, usb_i, 1, f); usb_i = 0;}
                        #define FWRITE(p,s,n,f) DUMP(s*n, f); memcpy(&usb_buffer[usb_i], p, s *n); usb_i += s*n //  SYS_FWrite
                        #define FOPEN(n,p) SYS_FOpen(n,p);
                        #define FFOPEN FOPEN
                        #define FCLOSE(f)  SYS_FClose(f) //SYS_FWrite(usb_buffer, usb_i, 1, f);
                        #define FPUTC(c,f) DUMP(1, f); usb_buffer[usb_i] = c; usb_i++ //   SYS_FWrite(&c, 1, 1, f)
                        char    CMD_LOG_FILE[256] = "/USBHDISK00/test.log";

                    #endif  //_USB_NONSTOP_
                #else   //!_USB_BUF_

                    #define FWRITE SYS_FWrite
                    #define FOPEN  SYS_FOpen
                    #define FFOPEN FOPEN
                    #define FFOPEN FOPEN
                    #define FCLOSE SYS_FClose
                    #define FPUTC(c,f)  SYS_FWrite(&c, 1, 1, f)
                    char    CMD_LOG_FILE[256] = "/USBHDISK00/test.log";

                #endif // _USB_BUF_

            #else  // !_USB_WRITE_

            #define FWRITE fwrite
            #define FOPEN  fopen
            #define FFOPEN FOPEN
            #define FCLOSE fclose
            #define FPUTC  fputc
            #define CMD_LOG_FILE "test.log"

        #endif   // _USB_WRITE_

        MME_ERROR   acc_MME_Init(void);
        void        acc_mme_logreset(void);
        void        acc_mme_logstart(void);
        void        acc_mme_logstop(void);
        void        acc_mme_loginit(void);
        void        acc_mme_logDeinit(void);


    #else //#if ST_OSLINUX
        /*********************************** LINUX *********************************/
        #include "stavmem.h"
        #include "staudlx.h" /* This file is included to define MME_DATA_DUMP_SIZE*/

        #define OVERFLOW_LIMIT      (MME_DATA_DUMP_SIZE - (15*1024))
        #define FILE            U32 /* Fake FILE structure in linux kernel */
        STAVMEM_BlockHandle_t       MME_AVMemHandle_p;
        U32                 *AVMemBufStartAddr=NULL;
        STAVMEM_PartitionHandle_t   Wrapper_AVMEMPartition;
    #endif // ST_OSLINUX

    unsigned char *curptr = NULL;
    /*function prototype*/
    FILE *      cmd_log_lock(void);
    void        cmd_log_release(FILE * fin);

    MME_ERROR   acc_MME_InitTransformer(const char *, MME_TransformerInitParams_t *, MME_TransformerHandle_t *);
    MME_ERROR   acc_MME_GetTransformerCapability(const char *, MME_TransformerCapability_t *);
    MME_ERROR   acc_MME_SendCommand( MME_TransformerHandle_t , MME_Command_t *);
    MME_ERROR   acc_MME_AbortCommand(MME_TransformerHandle_t , MME_CommandId_t);
    MME_ERROR   acc_MME_TermTransformer(MME_TransformerHandle_t);

    enum eBoolean
    {
        ACC_FALSE,
        ACC_TRUE,
    };

    enum eBoolean  log_cmd = ACC_TRUE;
    STOS_Mutex_t  *cmd_mutex;
    enum eCmdType
    {
        CMD_GETCAPABILITY, CMD_INIT, CMD_SEND, CMD_ABORT, CMD_TERM, CMD_LAST
    };

    static unsigned char * cmd_str[CMD_LAST] =
    {
        "CMD_GETC", "CMD_INIT", "CMD_SEND", "CMD_ABORT", "CMD_TERM"
    };

    #define NB_MAX_TRANSFORMERS 8

    int handles[NB_MAX_TRANSFORMERS + 1];

    enum eWarning
    {
        ACC_WARNING_NO_SCATTERPAGE_IN_BUFFER,

        // do not edit
        ACC_LAST_WARNING
    };

    char warning[ACC_LAST_WARNING][64]=
    {
        #define E(x) #x
        E(ACC_WARNING_NO_SCATTERPAGE_IN_BUFFER),
        #undef E
    };

    #ifdef ST_OSLINUX
        #define CheckBufWrap()                                                        \
        {                                                                             \
            if (curptr >= ((unsigned char *)AVMemBufStartAddr + OVERFLOW_LIMIT))          \
            {                                                                           \
                /* Start again from the buffer top.*/                         \
                /* We will lose old data but no option */                             \
                STTBX_Print((" Buffer Wrap \n"));                     \
                curptr = (unsigned char*)AVMemBufStartAddr;               \
            }                                                                           \
        }
        #else
            #define CheckBufWrap()      do { /* nothing to do */  } while (0)
            static void acc_warning(enum eWarning wn)
            {
                task_status_t status;
                task_status(0, &  status, task_status_flags_stack_used);
                printf("[%d bytes of stack used] %s\n", status.task_stack_used, warning[wn]);
            }
    #endif

    static void handles_clear(void)
    {
        int i;
        for (i = 0; i < NB_MAX_TRANSFORMERS; i++)
        {
        handles[i] = 0;
        }
    }

    static int handles_search(int hdl)
    {
        int i;
        for (i = 0; i < NB_MAX_TRANSFORMERS; i++)
        {
            if (handles[i] == hdl) return (i);
        }

        return i;
    }

    static int handles_new(void)
    {
        int i;
        for (i = 0; i < NB_MAX_TRANSFORMERS; i++)
        {
            if (handles[i] == 0) return (i);
        }
        return i;
    }

    FILE * cmd_log_lock(void)
    {
        FILE * fin=NULL;

        STOS_MutexLock(cmd_mutex);
        #ifndef ST_OSLINUX
            fin=FOPEN(CMD_LOG_FILE, "ab");
        #endif

        return(fin);
    }

    void cmd_log_release(FILE * fin)
    {
        #ifndef ST_OSLINUX
            FCLOSE(fin);
        #endif

        STOS_MutexRelease(cmd_mutex);
    }


    #ifdef ST_OSLINUX
        ST_ErrorCode_t AllocateMemoryForDump(STAVMEM_PartitionHandle_t AVMEMPartition)
        {
            STAVMEM_AllocBlockParams_t          AllocParams;
            ST_ErrorCode_t Error=ST_NO_ERROR;

            Wrapper_AVMEMPartition = AVMEMPartition;

            AllocParams.PartitionHandle = AVMEMPartition;
            AllocParams.Alignment = 32;
            AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AllocParams.ForbiddenRangeArray_p = NULL;
            AllocParams.ForbiddenBorderArray_p = NULL;
            AllocParams.NumberOfForbiddenRanges = 0;
            AllocParams.NumberOfForbiddenBorders = 0;
            AllocParams.Size = MME_DATA_DUMP_SIZE;

            Error = STAVMEM_AllocBlock(&AllocParams, &MME_AVMemHandle_p);
            if(Error != ST_NO_ERROR)
            {
                return Error;
            }

            Error = STAVMEM_GetBlockAddress(MME_AVMemHandle_p, (void *)&AVMemBufStartAddr);
            if(Error != ST_NO_ERROR)
                return Error;

            STTBX_Print(("AVMemBufStartAddr = %x ", AVMemBufStartAddr));
            /* Map AVmem address to physical address */
            AVMemBufStartAddr = (U32*)(((U32)AVMemBufStartAddr) | 0xA0000000);

            curptr = (unsigned char*)AVMemBufStartAddr;
            STTBX_Print(("curptr = %x\n", curptr));

            return Error;
        }

        ST_ErrorCode_t DeAllocateMemoryForDump(void)
        {
            STAVMEM_FreeBlockParams_t FreeBlockParams;
            ST_ErrorCode_t Error=ST_NO_ERROR;

            FreeBlockParams.PartitionHandle = Wrapper_AVMEMPartition;
            Error |= STAVMEM_FreeBlock(&FreeBlockParams, &MME_AVMemHandle_p);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print(("STAVMEM_FreeBlock Error = %x , %s\n", Error,__FILE__));
            }

            return Error;
        }


    #else

        void GetDumpAddress(void)
        {
            curptr = (char*)RAM2_BASE_ADDRESS;
        }

        static void cmd_printf(FILE * f, char * cmd)
        {
            int   l  = strlen(cmd);
            char *cp = cmd;
            char  c;

            while (l)
            {
                c = *cp;
                FPUTC(c,f);
                cp++; l--;
            }

            c = 0;
            FPUTC(c,f);
        }

        void acc_mme_logstart(void)
        {
            printf("REOPEN File :: %s\n", CMD_LOG_FILE);
            usb_fout = FFOPEN(CMD_LOG_FILE, "ab");
            log_cmd = ACC_TRUE;
        }

        void acc_mme_logstop(void)
        {
            log_cmd =  ACC_FALSE;
            FCLOSE(usb_fout);
        }

    #endif

    void acc_mme_loginit(void)
    {
        cmd_mutex = STOS_MutexCreateFifo();
        handles_clear();
    }

    void acc_mme_logDeinit(void)
    {
        STOS_MutexDelete(cmd_mutex);
        handles_    clear();
    }

    #ifndef ST_OSLINUX
        MME_ERROR acc_MME_Init(void)
        {
            MME_ERROR mme_status;
            GetDumpAddress();
            acc_mme_loginit();
            mme_status =  MME_Init();
            return mme_status;
        }

        void acc_mme_logreset(void)
        {
            FILE * fcmd;
            printf("CREATE File :: %s\n", CMD_LOG_FILE);
            fcmd = FFOPEN(CMD_LOG_FILE, "wb");
            FCLOSE(fcmd);
        }

    #else

        ST_ErrorCode_t Init_MME_Dump(STAVMEM_PartitionHandle_t AVMEMPartition)
        {
            ST_ErrorCode_t      error = ST_NO_ERROR;
            acc_mme_loginit();
            error = AllocateMemoryForDump(AVMEMPartition);
            if(error != ST_NO_ERROR)
            {
                STTBX_Print((" Error in AllocateMemoryForDump %x \n",error));
            }
            log_cmd = ACC_TRUE;
            return error;
        }

        ST_ErrorCode_t DeInit_MME_Dump(void)
        {
            ST_ErrorCode_t      error = ST_NO_ERROR;

            error = DeAllocateMemoryForDump();
            if(error != ST_NO_ERROR)
            {
                STTBX_Print((" Error in AllocateMemoryForDump %x \n",error));
            }
            acc_mme_logDeinit();
            return error;
        }

        void acc_mme_logreset(void)
        {
            U32 DataSize;
            STTBX_Print(("acc_mme_wrapper.c:Resetting the circular buffer \n"));
            curptr = (char*)AVMemBufStartAddr;
            DataSize=(U32)((curptr)-(unsigned char *)AVMemBufStartAddr);
            memset((void*)AVMemBufStartAddr,0,DataSize);
        }

    #endif

    #ifdef ST_OSLINUX
        EXPORT_SYMBOL(AVMemBufStartAddr);
        EXPORT_SYMBOL(curptr);
    #endif

    MME_ERROR acc_MME_InitTransformer(  const char *Name,
                                        MME_TransformerInitParams_t * Params_p,
                                        MME_TransformerHandle_t     * Handle_p)
    {
        MME_ERROR  mme_status;
        FILE     * fcmd = cmd_log_lock();
        #ifndef ST_OSLINUX
            if (log_cmd == ACC_TRUE)
            {
                cmd_printf(fcmd,cmd_str[CMD_INIT]);
                cmd_printf(fcmd,(char *)Name);
                FWRITE(Params_p, sizeof(MME_TransformerInitParams_t), 1, fcmd);
                FWRITE(Params_p->TransformerInitParams_p, Params_p->TransformerInitParamsSize, 1, fcmd);
            }

        #else
            CheckBufWrap();

            if (log_cmd == ACC_TRUE)
            {
                memcpy((void*)curptr,(void*)cmd_str[CMD_INIT],strlen(cmd_str[CMD_INIT]));
                curptr += strlen(cmd_str[CMD_INIT]);

                *curptr = 0x00;
                curptr+=1;

                memcpy((void*)curptr,(void*)Name,strlen(Name));
                curptr += strlen(Name);

                *curptr = 0x00;
                curptr+=1;

                FWRITE(Params_p->TransformerInitParams_p, Params_p->TransformerInitParamsSize, 1, fcmd);

                memcpy((void*)curptr,(void*)Params_p,sizeof(MME_TransformerInitParams_t));
                curptr += sizeof(MME_TransformerInitParams_t);

                memcpy((void*)curptr,(void*)Params_p->TransformerInitParams_p,Params_p->TransformerInitParamsSize);//copy cmd string of size cmd_init to lmi vid current pointer
                curptr += Params_p->TransformerInitParamsSize;
            }

        #endif
        cmd_log_release(fcmd);

        mme_status =  MME_InitTransformer(Name, Params_p, Handle_p);

        if (mme_status == MME_SUCCESS)
        {
            handles[handles_new()] = (int) *Handle_p;
        }
        return mme_status;
    }


    MME_ERROR acc_MME_GetTransformerCapability(const char * TransformerName, MME_TransformerCapability_t *TransformerInfo_p)
    {

        if (log_cmd == ACC_TRUE)
        {
            FILE * fcmd=NULL;

            #ifdef ST_OSLINUX
                CheckBufWrap();
            #endif

            fcmd = (FILE *)cmd_log_lock();

            #ifndef ST_OSLINUX
                cmd_printf(fcmd,cmd_str[CMD_GETCAPABILITY]);
                cmd_printf(fcmd,(char *)TransformerName);
            #else
                memcpy((void*)curptr,(void*)cmd_str[CMD_GETCAPABILITY],strlen(cmd_str[CMD_GETCAPABILITY]));
                curptr += strlen(cmd_str[CMD_GETCAPABILITY]);

                *curptr = 0x00;
                curptr+=1;

                memcpy((void*)curptr,(void*)TransformerName,strlen(TransformerName));
                curptr += strlen(TransformerName);

                *curptr = 0x00;
                curptr+=1;
            #endif

            cmd_log_release(fcmd);
        }

        return MME_GetTransformerCapability(TransformerName, TransformerInfo_p);
    }

    MME_ERROR acc_MME_SendCommand(MME_TransformerHandle_t Handle, MME_Command_t * CmdInfo_p)
    {
        MME_ERROR   mme_status;
        FILE      * fcmd = NULL;
        int         i, j, nbuf;

        if (log_cmd == ACC_TRUE)
        {

        }

        if (log_cmd == ACC_TRUE)
        {
            // log the command;
            int hdl_idx;

            #ifdef ST_OSLINUX
                CheckBufWrap();
            #endif

            hdl_idx =  handles_search((int)Handle);

            fcmd    = cmd_log_lock();

            #ifndef ST_OSLINUX
                cmd_printf(fcmd,cmd_str[CMD_SEND]);
                FWRITE(&hdl_idx, sizeof(int), 1, fcmd);
                FWRITE(CmdInfo_p, sizeof(MME_Command_t), 1, fcmd);
                FWRITE(CmdInfo_p->Param_p, CmdInfo_p->ParamSize, 1, fcmd);
            #else
                memcpy((void*)curptr,(void*)cmd_str[CMD_SEND],strlen(cmd_str[CMD_SEND]));
                curptr += strlen(cmd_str[CMD_SEND]);

                *curptr = 0x00;
                curptr  +=1;

                memcpy((void*)curptr,(void*)&hdl_idx,sizeof(int));
                curptr  += sizeof(int);

                memcpy((void*)curptr,(void*)CmdInfo_p, sizeof(MME_Command_t));
                curptr  += sizeof(MME_Command_t);

                memcpy((void*)curptr,(void*)CmdInfo_p->Param_p,CmdInfo_p->ParamSize);
                curptr  += CmdInfo_p->ParamSize;

            #endif

            nbuf =  CmdInfo_p->NumberInputBuffers + CmdInfo_p->NumberOutputBuffers;
            for (i = 0; i < nbuf; i++)
            {
                MME_DataBuffer_t  * db = CmdInfo_p->DataBuffers_p[i];
                MME_ScatterPage_t * sc;

                #ifndef ST_OSLINUX
                    FWRITE(db, sizeof(MME_DataBuffer_t), 1, fcmd);
                #else
                    memcpy((void*)curptr,(void*)db, sizeof(MME_DataBuffer_t));
                    curptr +=  sizeof(MME_DataBuffer_t);
                #endif

                if (db->NumberOfScatterPages == 0)
                {
                    #ifndef ST_OSLINUX
                        // check whether a buffer is sent without any pages  !! should never happen.
                        acc_warning(ACC_WARNING_NO_SCATTERPAGE_IN_BUFFER);
                    #endif
                }

                for (j = 0; j < (int)db->NumberOfScatterPages; j++)
                {
                    sc = &db->ScatterPages_p[j];

                    #ifndef ST_OSLINUX
                        FWRITE(sc, sizeof(MME_ScatterPage_t), 1, fcmd);
                    #else
                        memcpy((void*)curptr,(void*)sc, sizeof(MME_ScatterPage_t));
                        curptr +=  sizeof(MME_ScatterPage_t);
                    #endif

                    if (sc->Size != 0)
                    {
                        if (i<(int)CmdInfo_p->NumberInputBuffers)
                        {
                            #ifndef ST_OSLINUX
                                FWRITE(sc->Page_p, sizeof(unsigned char), sc->Size,fcmd);
                            #else
                                memcpy((void*)curptr,(void*)sc->Page_p,  sc->Size * sizeof(unsigned char));
                                curptr +=  (sc->Size * sizeof(unsigned char));
                            #endif
                        }
                    }
                }
            }
        }

        // send the command to get back the ID generated by Multicom
        mme_status =  MME_SendCommand(Handle, CmdInfo_p);

        if (log_cmd == ACC_TRUE)
        {
            // replace the ID in the local copy
            #ifndef ST_OSLINUX
                FWRITE(&CmdInfo_p->CmdStatus.CmdId, sizeof(unsigned int), 1 ,fcmd);
            #else
                    memcpy((void*)curptr,(void*)&CmdInfo_p->CmdStatus.CmdId, sizeof(unsigned int));
                curptr +=  sizeof(unsigned int);
            #endif

            cmd_log_release(fcmd);
        }

        return mme_status;
    }

    MME_ERROR acc_MME_AbortCommand(MME_TransformerHandle_t Handle, MME_CommandId_t CmdId)
    {
        if (log_cmd == ACC_TRUE)
        {
            FILE *fcmd;
            int hdl_idx;

            #ifdef ST_OSLINUX
                CheckBufWrap();
            #endif

            fcmd    = (FILE *)cmd_log_lock();
            hdl_idx =  handles_search((int)Handle);

            #ifndef ST_OSLINUX
                cmd_printf(fcmd,cmd_str[CMD_ABORT]);
                FWRITE(&hdl_idx, sizeof(int), 1, fcmd);
                FWRITE(&CmdId, sizeof(int), 1, fcmd);
            #else
                memcpy((void*)curptr,(void*)cmd_str[CMD_ABORT],strlen(cmd_str[CMD_ABORT]));
                curptr += strlen(cmd_str[CMD_ABORT]);
                *curptr = 0x00;
                curptr+=1;
                memcpy((void*)curptr,(void*)&hdl_idx, sizeof(int));
                curptr +=  sizeof(int);
                memcpy((void*)curptr,(void*)&CmdId, sizeof(int));
                curptr +=  sizeof(int);
            #endif

            cmd_log_release(fcmd);
        }
        return MME_AbortCommand(Handle, CmdId);
    }


    /* MME_TermTransformer()
    * Terminate a transformer instance
    */
    MME_ERROR acc_MME_TermTransformer(MME_TransformerHandle_t handle)
    {
        MME_ERROR   mme_status;
        int         hdl_idx =  handles_search((int)handle);

        if (log_cmd == ACC_TRUE)
        {
            FILE * fcmd;

            #ifdef ST_OSLINUX
                CheckBufWrap();
            #endif

            fcmd = (FILE *)cmd_log_lock();

            #ifndef ST_OSLINUX
                cmd_printf(fcmd,cmd_str[CMD_TERM]);
                FWRITE(&hdl_idx, sizeof(int), 1, fcmd);
            #else
                memcpy((void*)curptr,(void*)cmd_str[CMD_TERM],strlen(cmd_str[CMD_TERM]));
                curptr += strlen(cmd_str[CMD_TERM]);
                *curptr = 0x00;
                curptr+=1;

                memcpy((void*)curptr,(void*)&hdl_idx, sizeof(int));
                curptr +=  sizeof(int);
            #endif

            cmd_log_release(fcmd);
        }

        mme_status =  MME_TermTransformer(handle);

        if (mme_status == MME_SUCCESS)
        {
            handles[hdl_idx] = 0;
        }
        #ifdef ST_OSLINUX
            #if FORCEFULLTERM_LINUX
                while (mme_status != MME_SUCCESS)
                {
                    /* This condition will arise when there is a MME_TermTransformer failure.
                    For Log purpose we are not returning from this function in case of failure.
                    We shall loop in a while loop. As soon as the user notices a kernel message
                    on console informing him about MME_TermTransformer failure, take a
                    mme dump from the test harness (aud_mmedump)
                    */
                    printk(KERN_ALERT "MME_TermTransformer FAILURE: PLS Take dump\n");
                    STOS_TaskDelayUs((signed long long)1000000*1000);
                }
            #endif
        #endif
        return mme_status;
    }

    #endif //_ACC_WRAPP_MME_
#endif //_MME_WRAPPER_




