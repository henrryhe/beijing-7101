#ifndef __H_LINUX_COMMON
#define __H_LINUX_COMMON

#include "stddefs.h"
#include "stcommon.h"

/*
  List of Major numbers used to access STAPI drivers.
  By default Major is zero so that Linux kernel will
  assign a value dynamically when the module is loaded.
  This prevents from conflicts with existing drivers.
  However this can be customized in a final STB product
  where precise values need to be defined a priori in
  order to avoid of creating special device nodes.
*/
/* front end */
#define STTUNER_IOCTL_MAJOR      0
#define STINJ_IOCTL_MAJOR        0
#define STIPINJ_IOCTL_MAJOR	     0
#define STMERGE_IOCTL_MAJOR      0
#define STPTI4_IOCTL_MAJOR       0
#define STDEMUX_IOCTL_MAJOR      0
/* Audio/Video */
#define STVID_IOCTL_MAJOR        0
#define STVIDTEST_IOCTL_MAJOR    0
#define STAUDLX_IOCTL_MAJOR      0
/* Video output stage */
#define STVTG_IOCTL_MAJOR        0
#define STVOUT_IOCTL_MAJOR       0
#define STHDMI_IOCTL_MAJOR       0
/* Graphic */
#define STLAYER_IOCTL_MAJOR      0
#define STBLIT_IOCTL_MAJOR       0
#define STVMIX_IOCTL_MAJOR       0
#define STDENC_IOCTL_MAJOR       0
#define STGXOBJ_IOCTL_MAJOR      0 
#define STGFB_IOCTL_MAJOR        0 
/* infrastructure */
#define STEVT_IOCTL_MAJOR        0
#define STCLKRV_IOCTL_MAJOR      0
#define STAPIGAT_IOCTL_MAJOR     0
#define TESTTOOL_IOCTL_MAJOR     0
/* miscellanea */
#define STSMART_IOCTL_MAJOR      0
#define STVBI_IOCTL_MAJOR        0
#define STCC_IOCTL_MAJOR         0
#define STTTX_IOCTL_MAJOR        0
#define STBLAST_IOCTL_MAJOR      0
#define STSUBT_IOCTL_MAJOR       0
#define STPCCRD_IOCTL_MAJOR      0
/* security    */
#define STCRYPT_IOCTL_MAJOR      0
#define GSECHAL_IOCTL_MAJOR      0
#define STTKDMA_IOCTL_MAJOR      0
#define NDSDTVCA_IOCTL_MAJOR     0
/* tree */
#define STTREE1_IOCTL_MAJOR      0
#define STTREE2_IOCTL_MAJOR      0

/*
  The following list of major is for core module and
  it is temporary provided to avoid "driver build or
  integration issues". Core majors MUST not be used,
  relevant special device files MUST not be created,
  SOON THESE WILL BE REMOVED!
*/
#define STTUNER_CORE_MAJOR      0
#define STIPINJ_CORE_MAJOR      0
#define STINJ_CORE_MAJOR        0
#define STMERGE_CORE_MAJOR      0
#define STPTI4_CORE_MAJOR       0
#define STDEMUX_CORE_MAJOR      0
#define STVID_CORE_MAJOR        0
#define STVIDTEST_CORE_MAJOR    0
#define STAUDLX_CORE_MAJOR      0
#define STVTG_CORE_MAJOR        0
#define STVOUT_CORE_MAJOR       0
#define STHDMI_CORE_MAJOR       0
#define STLAYER_CORE_MAJOR      0
#define STBLIT_CORE_MAJOR       0
#define STVMIX_CORE_MAJOR       0
#define STDENC_CORE_MAJOR       0
#define STGXOBJ_CORE_MAJOR      0 
#define STGFB_CORE_MAJOR        0 
#define STEVT_CORE_MAJOR        0
#define STCLKRV_CORE_MAJOR      0
#define STAPIGAT_CORE_MAJOR     0
#define TESTTOOL_CORE_MAJOR     0
#define STSMART_CORE_MAJOR      0
#define STVBI_CORE_MAJOR        0
#define STCC_CORE_MAJOR         0
#define STI2C_CORE_MAJOR        0
#define STPIO_CORE_MAJOR        0
#define STTTX_CORE_MAJOR        0
#define STBLAST_CORE_MAJOR      0
#define STFDMA_CORE_MAJOR       0
#define STCRYPT_CORE_MAJOR      0
#define GSECHAL_CORE_MAJOR      0
#define STTKDMA_CORE_MAJOR      0
#define STSUBT_CORE_MAJOR       0
#define STPCCRD_CORE_MAJOR      0
#define STTREE1_CORE_MAJOR      0
#define STTREE2_CORE_MAJOR      0

/* THREAD REALTIME SWITCHES */

#define STVID_DISP_THREAD_IN_REALTIME_PRIORITY          TRUE
#define STVID_TRICKMODE_THREAD_IN_REALTIME_PRIORITY     TRUE
#define STVID_SPEED_THREAD_IN_REALTIME_PRIORITY         TRUE
#define STVID_DECODE_THREAD_IN_REALTIME_PRIORITY        TRUE
#define STVID_INJECTES_THREAD_IN_REALTIME_PRIORITY      TRUE
#define STVID_INJECTSTUB_THREAD_IN_REALTIME_PRIORITY    TRUE
#define STVID_ERROR_THREAD_IN_REALTIME_PRIORITY         TRUE
#define STVID_INJECTION_THREAD_IN_REALTIME_PRIORITY     TRUE

#define STVID_PRODUCER_THREAD_IN_REALTIME_PRIORITY      TRUE
#define STVID_DECODEH264_THREAD_IN_REALTIME_PRIORITY    TRUE
#define STVID_PARSER_THREAD_IN_REALTIME_PRIORITY        TRUE
#define STVID_PREPROC_THREAD_IN_REALTIME_PRIORITY       TRUE

#define STCC_THREAD_IN_REALTIME_PRIORITY                TRUE

#define STLAYER_VIDEO_THREAD_IN_REALTIME_PRIORITY       TRUE
#define STLAYER_GFX_THREAD_IN_REALTIME_PRIORITY         FALSE
#define STVOUT_HDMI_THREAD_IN_REALTIME_PRIORITY         TRUE
#define STVOUT_INFOFRAME_THREAD_IN_REALTIME_PRIORITY    TRUE
#define STBLIT_THREAD_IN_REALTIME_PRIORITY              TRUE

/* Task priorities 99 Highest - 0 Lowest */
#define STFDMA_CALLBACK_TASK_PRIORITY           99  /* OS20 - Highest */
#define STCLKRV_THREAD_PRIORITY                 99  /* Highest */

#define STINTMR_THREAD_PRIORITY					30  /* OS20 - undefined */

#define STVID_THREAD_DISPLAY_PRIORITY         	12	/* OS20 - 12 */
#define STVID_THREAD_TRICKMODE_PRIORITY       	11	/* OS20 - 11 */
#define STVID_THREAD_SPEED_PRIORITY             STVID_THREAD_TRICKMODE_PRIORITY
#define STVID_THREAD_DECODE_PRIORITY 			10	/* OS20 - 10 */
#define STVID_THREAD_INJECT_ES_PRIORITY       	9	/* OS21 - 9 */
#define STVID_THREAD_INJECTSTUB_PRIORITY        9	/* OS21 - 9 */
#define STVID_THREAD_ERROR_RECOVERY_PRIORITY  	9	/* OS20 - 9 */

#define STVID_INJECTION_THREAD_PRIORITY			5   /* OS20 - MIN */

#define STVID_THREAD_PRODUCER_PRIORITY          STVID_THREAD_DECODE_PRIORITY   /* OS21 - Decode */
#define STVID_THREAD_DECODERH264_PRIORITY       STVID_THREAD_DECODE_PRIORITY   /* OS21 - Decode */
#define STVID_THREAD_PARSER_PRIORITY            STVID_THREAD_DECODE_PRIORITY   /* OS21 - Decode */
#define STVID_THREAD_PREPROC_PRIORITY           25   /* OS21 - 255 */

#define STVOUT_AUTOMATIC_INFOFRAME_THREAD_PRIORITY   30 /* OS21 - 255 */

#define STLAYER_VIDEO_THREAD_PRIORITY           7   /* OS21 - 7 */
#define STLAYER_GFX_THREAD_PRIORITY             10  /* To be checked ... */

#define STBLIT_MASTER_THREAD_PRIORITY           5   /* OS21 - 5  */
#define STBLIT_SLAVE_THREAD_PRIORITY            5   /* OS21 - 5  */
#define STBLIT_INTERRUPT_THREAD_PRIORITY        10  /* OS21 - 10 */



/* Thread names */
#define KERNEL_THREAD_DECODE_NAME       "kvid_dec.d"
#define KERNEL_THREAD_TRICKMODE_NAME    "kvid_trick.d"
#define KERNEL_THREAD_SPEED_NAME        "kvid_speed.d"
#define KERNEL_THREAD_DISPLAY_NAME      "kvid_disp.d"
#define KERNEL_THREAD_ERROR_NAME        "kvid_err.d"
#define KERNEL_THREAD_INJECTION_NAME    "kvid_inj.d"
#define KERNEL_THREAD_PRODUCER_NAME     "kvid_prod.d"
#define KERNEL_THREAD_PARSER_NAME       "kvid_pars.d"
#define KERNEL_THREAD_PREPROC_NAME      "kvid_preproc.d"

#define KERNEL_THREAD_INJECT_ES_NAME    "kvid_injes.d"

#define KERNEL_THREAD_LAYERVID_NAME         "klay_vid.d"
#define KERNEL_THREAD_LAYERGFX_NAME         "klay_gfx.d"

#define KERNEL_THREAD_HARDWAREMANAGER_NAME  "HardwareManagerTask"
#define KERNEL_THREAD_PROCESSCAPTION_NAME   "KprocessCaptionTask"
#define KERNEL_THREAD_STVOUT_HDMI_NAME      "Khdmi_task"
#define KERNEL_THREAD_STVOUT_INFOFRAME_TASK "kvout_InfoFrameAutoTask"


#define KERNEL_THREAD_BLITMASTER_NAME       "kblit_master.d"
#define KERNEL_THREAD_BLITSLAVE_NAME        "kblit_slave.d"
#define KERNEL_THREAD_BLITINTERRUPT_NAME	"kblit_interrupt.d"



/* ************** */
/* For GetRevision*/

/* The IOCTL call returns the revision string of the ioctl module and core module in this structure */
#define STCOMMON_REV_STRING_LEN  (256)
/* The IOCTL call returns the revision string of the ioctl module and core module in this structure */
typedef struct
{
	char IoctlRevString[STCOMMON_REV_STRING_LEN];/* Strings must not be longer that STCOMMON_REV_STRING_LEN bytes including NULL*/
	char CoreRevString[STCOMMON_REV_STRING_LEN];/* Strings must not be longer that STCOMMON_REV_STRING_LEN bytes including NULL*/
} ST_RevisionStrings_t;

#if defined( MODULE ) /* Kernel space */
/************************************************************************************************************************/
/* Prototype of shared IOCTL case function called from the GETREVISION_IOCTL_IMPLEMENTATION macro defined below        */
/* This function should never be directly called from the ioctl.                                                        */
/************************************************************************************************************************/
int ST_GetRevIoctlFunction(ST_RevisionStrings_t *arg, const ST_Revision_t Revision, ST_Revision_t (*func)(void));

/************************************************************************************************************************/
/* This macro implements the ioctl case statement within stxxx_ioctl_ioctl.c                                            */
/* It must be inserted within the ioctl switch statement replacing any existing GetRevision case.                       */
/* Arguments:                                                                                                           */
/* n : Name of STAPI driver. This is the driver function prefix. eg. STPTI for the PTI.                                 */
/* r : IOCTL request. This is the definition of the GetRevision ioctl request defined in stxxx_ioctl.h                  */
/* v : IOCTL revision definition. The Driver revision definition as defined in stxxx.h. eg. STXXX_REVISION              */
/*      All drivers require STXXX_REVISION to be defined in stxxx.h.                                                    */
/*      eg. #define STPTI_REVISION      "STPTI_DVB-REL_6.6.3A0         "                                                */
/*      The string should be no longer than 256 chars                                                                    */
/*                                                                                                                      */
/* Example:                                                                                                             */
/*              ...                                                                                                     */
/*              break;                                                                                                  */
/*                                                                                                                      */
/*        GETREVISION_IOCTL_IMPLEMENTATION(STPTI,STPTI_IOC_GETREVISION,STPTI_REVISION)                                 */
/*                                                                                                                      */
/*        case STPTI_IOC_SUBSCRIBEEVENT:                                                                                */
/*              ...                                                                                                     */
/*                                                                                                                      */
/* The example shows the macro inserted between the break of a previous case statement and the case of the next.        */ 
/************************************************************************************************************************/
#define GETREVISION_IOCTL_IMPLEMENTATION(n,r,v) \
        case (r):\
            return ST_GetRevIoctlFunction((ST_RevisionStrings_t*)arg,(v),n##_GetRevision);\
            break;

#else   /* User space */

/************************************************************************************************************************/
/* This is the prototype of the shared ioctl userspace library function for making the ioctl call to get the revision.  */
/* The function is called from the GETREVISION_LIB_IMPLEMENTATION macro defined below                                  */
/* This function should never be directly called from the ioctl.                                                        */
/************************************************************************************************************************/
ST_Revision_t ST_GetDriverRevision(const char *drivername, int request,const char *path,const char *revision);

/************************************************************************************************************************/
/* This macro implements the ioctl userspace library function within stxxx_ioctl_lib.c                                  */
/* It expands to provide a getrevision function prefixed with the STAPI driver name                                     */
/* eg. STXXX_GetRevision()                                                                                              */
/* The macro must be inserted into the stxxx_ioctl_lib.c file replacing the existing STXXX_GetRevision() function.      */
/* Arguments:                                                                                                           */
/* n : Name of STAPI driver. This is the driver function prefix. eg. STPTI for the PTI.                                 */
/* r : IOCTL request. This is the definition of the GetRevision ioctl request defined in stxxx_ioctl.h                  */
/* p : Device path. The /dev path environment name string.                                                              */
/* v : Lib revision definition. The Driver revision definition as defined in stxxx.h. eg. STXXX_REVISION                */
/*     All drivers require STXXX_REVISION to be defined in stxxx.h.                                                     */
/*     eg. #define STPTI_REVISION      "STPTI_DVB-REL_6.6.3A0         "                                                 */
/*     The string should be no longer than 256 chars                                                                    */
/*                                                                                                                      */
/* Example:                                                                                                             */
/*                                                                                                                      */
/* GETREVISION_LIB_IMPLEMENTATION(STPTI,STPTI_IOC_GETREVISION,"STPTI4_IOCTL_DEV_PATH",STPTI_REVISION);                  */
/*                                                                                                                      */
/*                                                                                                                      */
/************************************************************************************************************************/
#define GETREVISION_LIB_IMPLEMENTATION(n,r,p,v) \
ST_Revision_t n##_GetRevision() \
{\
     return ST_GetDriverRevision(#n,(r),(p),(v));       \
}
#endif


/*********************/



#if !defined(MODULE)
extern int      LinuxPageSize;

/* DencVtgDvoVos Shared mapping area */
extern int      DencVtgDvoVosMappingNumber;
extern void    *DencVtgDvoVosVirtualAddress;
extern void    *AvmemVirtualBaseAddress;

/* Video mapping shared area */
extern int      VideoMappingNumber;
extern void    *VideoVirtualAddress;
extern void    *VideoCDVirtualAddress;

/* Layer mapping address. Needed by the video to access registers... To be cleanned */
extern int      CompositorMappingNumber;
extern void    *CompositorVirtualAddress;
extern void    *LayerVirtualAddress;

/* Disp0 Shared mapping area */
extern int      Disp0MappingNumber;
extern void    *Disp0VirtualAddress;

/* Disp1 Shared mapping area */
extern int      Disp1MappingNumber;
extern void    *Disp1VirtualAddress;

/* Blitter Shared mapping area */
extern int      BlitterMappingNumber;
extern void    *BlitterVirtualAddress;

/* *************** */

#endif /*!defined(MODULE)*/


#endif  /* __H_LINUX_COMMON */

/* End of File */
