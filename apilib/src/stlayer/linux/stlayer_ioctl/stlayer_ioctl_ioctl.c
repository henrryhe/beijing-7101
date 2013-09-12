/*****************************************************************************
 *
 *  Module      : stlayer_ioctl
 *  Date        : 13-11-2005
 *  Author      : AYARITAR
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>            /* File operations (fops) defines */
#include <linux/ioport.h>        /* Memory/device locking macros   */

#include <linux/errno.h>         /* Defines standard error codes */

#include <asm/uaccess.h>         /* User space access routines */

#include <linux/sched.h>         /* User privilages (capabilities) */

#include "stlayer_ioctl_types.h"    /* Module specific types */

#include "stlayer_ioctl.h"          /* Defines ioctl numbers */


#include "stlayer.h"	 /* A STAPI ioctl driver. */

#include "stlayer_ioctl_fops.h"

/*** PROTOTYPES **************************************************************/

/*** METHODS *****************************************************************/

ST_ErrorCode_t STLAYER_AllocData( STLAYER_Handle_t  LayerHandle, STLAYER_AllocDataParams_t *Params_p, void **Address_p )
{
    ST_ErrorCode_t          ErrorCode;
/*    STAVMEM_AllocParams_t   AllocParams;*/

    if ( (U32*)LayerHandle == NULL )
    {
        STTBX_Print(( "STLAYER_AllocData(): Bad Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if ( Params_p == NULL )
    {
        STTBX_Print(( "STLAYER_AllocData(): Bad alloc data params\n"));
        return ST_ERROR_BAD_PARAMETER;
   }


/*
    AllocParams.Size = Params_p->Size ;
    AllocParams.Alignment = Params_p->Alignment ;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP ;
    AllocParams.ForbiddenRangeArray_p = NULL ;
    AllocParams.NumberOfForbiddenRanges = 0 ;
*/
    ErrorCode = STAVMEM_AllocBuffer( LAYER_PARTITION_AVMEM, Params_p->Size, Params_p->Alignment, Address_p );

    return ErrorCode;
} /* End of STLAYER_AllocData() */


/*******************************************************************************
Name        : STLAYER_AllocDataSecure
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t STLAYER_AllocDataSecure( STLAYER_Handle_t  LayerHandle, STLAYER_AllocDataParams_t *Params_p, void **Address_p )
{
    ST_ErrorCode_t          ErrorCode;

    if ( (U32*)LayerHandle == NULL )
    {
        STTBX_Print(( "STLAYER_AllocDataSecure(): Bad Handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if ( Params_p == NULL )
    {
        STTBX_Print(( "STLAYER_AllocDataSecure(): Bad alloc data params\n"));
        return ST_ERROR_BAD_PARAMETER;
   }

    ErrorCode = STAVMEM_AllocBuffer( LAYER_SECURED_PARTITION_AVMEM, Params_p->Size, Params_p->Alignment, Address_p );

    return ErrorCode;
} /* End of STLAYER_AllocSecuredData() */

ST_ErrorCode_t STLAYER_FreeData( STLAYER_Handle_t  LayerHandle, void *Address_p )
{
        return STAVMEM_FreeBuffer( LAYER_PARTITION_AVMEM, Address_p );
} /* End of STLAYER_FreeData() */

/*=============================================================================

   stlayer_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stlayer_ioctl.h'.

  ===========================================================================*/
int stlayer_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

    STTBX_Print((KERN_ALERT "STLAYER: ioctl command %d [0x%.8x]\n", _IOC_NR(cmd), cmd));

    /*** Check the access permittions ***/

    if ((_IOC_DIR(cmd) & _IOC_READ) && !(filp->f_mode & FMODE_READ)) {

        /* No Read permittions */
        err = -ENOTTY;
        goto fail;
    }

    if ((_IOC_DIR(cmd) & _IOC_WRITE) && !(filp->f_mode & FMODE_WRITE)) {

        /* No Write permittions */
        err = -ENOTTY;
        goto fail;
    }

    /*** Execute the command ***/

    switch (cmd)
    {

        case STLAYER_IOC_INIT:
            {
                STLAYER_Ioctl_Init_t UserParams;

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_Init_t *)arg, sizeof(STLAYER_Ioctl_Init_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_Init(UserParams.DeviceName, &( UserParams.InitParams ));

                if((err = copy_to_user((STLAYER_Ioctl_Init_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_Init_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STLAYER_IOC_TERM:
            {
                STLAYER_Ioctl_Term_t UserParams;

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_Term_t *)arg, sizeof(STLAYER_Ioctl_Term_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_Term(UserParams.DeviceName, &( UserParams.TermParams ));

                if((err = copy_to_user((STLAYER_Ioctl_Term_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_Term_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STLAYER_IOC_OPEN:
            {
                STLAYER_Ioctl_Open_t UserParams;

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_Open_t *)arg, sizeof(STLAYER_Ioctl_Open_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_Open(UserParams.DeviceName, &( UserParams.OpenParams ), &(UserParams.Handle));

                if((err = copy_to_user((STLAYER_Ioctl_Open_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_Open_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STLAYER_IOC_CLOSE:
            {
                STLAYER_Ioctl_Close_t UserParams;

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_Close_t *)arg, sizeof(STLAYER_Ioctl_Close_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_Close(UserParams.Handle);

                if((err = copy_to_user((STLAYER_Ioctl_Close_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_Close_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STLAYER_IOC_GETCAPABILITY:
            {
                STLAYER_Ioctl_GetCapability_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETCAPABILITY"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetCapability_t *)arg, sizeof(STLAYER_Ioctl_GetCapability_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetCapability(UserParams.DeviceName,&(UserParams.Capability));

                if((err = copy_to_user((STLAYER_Ioctl_GetCapability_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetCapability_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETINITALLOCPARAMS:
            {
                STLAYER_Ioctl_GetInitAllocParams_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETINITALLOCPARAMS"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetInitAllocParams_t *)arg, sizeof(STLAYER_Ioctl_GetInitAllocParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetInitAllocParams(UserParams.LayerType,UserParams.ViewPortsNumber,&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_GetInitAllocParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetInitAllocParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }



            break;

        case STLAYER_IOC_GETLAYERPARAMS:
            {
                STLAYER_Ioctl_GetLayerParams_t UserParams;
                 STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETLAYERPARAMS"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetLayerParams_t *)arg, sizeof(STLAYER_Ioctl_GetLayerParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetLayerParams(UserParams.Handle,&(UserParams.LayerParams));

                if((err = copy_to_user((STLAYER_Ioctl_GetLayerParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetLayerParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETLAYERPARAMS:
            {
                STLAYER_Ioctl_SetLayerParams_t UserParams;
                 STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETLAYERPARAMS"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetLayerParams_t *)arg, sizeof(STLAYER_Ioctl_SetLayerParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetLayerParams(UserParams.Handle,&(UserParams.LayerParams));

                if((err = copy_to_user((STLAYER_Ioctl_SetLayerParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetLayerParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_OPENVIEWPORT:
            {
                STLAYER_Ioctl_OpenViewPort_t UserParams;
                 STTBX_Print((KERN_DEBUG "STLAYER_IOC_OPENVIEWPORT\n"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_OpenViewPort_t *)arg, sizeof(STLAYER_Ioctl_OpenViewPort_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_OpenViewPort(UserParams.LayerHandle,&(UserParams.Params),&(UserParams.VPHandle));

                if((err = copy_to_user((STLAYER_Ioctl_OpenViewPort_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_OpenViewPort_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_CLOSEVIEWPORT:
            {
                STLAYER_Ioctl_CloseViewPort_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_CLOSEVIEWPORT"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_CloseViewPort_t *)arg, sizeof(STLAYER_Ioctl_CloseViewPort_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_CloseViewPort(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_CloseViewPort_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_CloseViewPort_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_ENABLEVIEWPORT:

            {
                STLAYER_Ioctl_EnableViewPort_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_ENABLEVIEWPORT"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_EnableViewPort_t *)arg, sizeof(STLAYER_Ioctl_EnableViewPort_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_EnableViewPort(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_EnableViewPort_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_EnableViewPort_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_DISABLEVIEWPORT:
           {
                STLAYER_Ioctl_DisableViewPort_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_DISABLEVIEWPORT"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_DisableViewPort_t *)arg, sizeof(STLAYER_Ioctl_DisableViewPort_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_DisableViewPort(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_DisableViewPort_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_DisableViewPort_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_ADJUSTVIEWPORTPARAMS:
          {
                STLAYER_Ioctl_AdjustViewPortParams_t UserParams;
               STTBX_Print((KERN_DEBUG "STLAYER_IOC_ADJUSTVIEWPORTPARAMS"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_AdjustViewPortParams_t *)arg, sizeof(STLAYER_Ioctl_AdjustViewPortParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_AdjustViewPortParams(UserParams.LayerHandle,&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_AdjustViewPortParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_AdjustViewPortParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

         break;

        case STLAYER_IOC_SETVIEWPORTPARAMS:
        {
                STLAYER_Ioctl_SetViewPortParams_t UserParams;
                 STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTPARAMS"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortParams_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortParams(UserParams.VPHandle,&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETVIEWPORTPARAMS:
            {
                STLAYER_Ioctl_GetViewPortParams_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTPARAMS"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortParams_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortParams(UserParams.VPHandle,&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETVIEWPORTSOURCE:
            {
                STLAYER_Ioctl_SetViewPortSource_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTSOURCE"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortSource_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortSource_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortSource(UserParams.VPHandle,&(UserParams.VPSource));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortSource_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortSource_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;


        case STLAYER_IOC_SETFLICKERFILTERMODE:
            {

                STLAYER_Ioctl_SetFlickerFilterMode_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETFLICKERFILTERMODE 1"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetFlickerFilterMode_t *)arg, sizeof(STLAYER_Ioctl_SetFlickerFilterMode_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortFlickerFilterMode(UserParams.VPHandle,UserParams.FFMode);

                if((err = copy_to_user((STLAYER_Ioctl_SetFlickerFilterMode_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetFlickerFilterMode_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETFLICKERFILTERMODE:
            {

                STLAYER_Ioctl_SetFlickerFilterMode_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETFLICKERFILTERMODE"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetFlickerFilterMode_t *)arg, sizeof(STLAYER_Ioctl_GetFlickerFilterMode_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortFlickerFilterMode(UserParams.VPHandle,&(UserParams.FFMode));

                if((err = copy_to_user((STLAYER_Ioctl_GetFlickerFilterMode_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetFlickerFilterMode_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;




        case STLAYER_IOC_GETVIEWPORTSOURCE:
            {
                STLAYER_Ioctl_GetViewPortSource_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTSOURCE"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortSource_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortSource_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortSource(UserParams.VPHandle,&(UserParams.VPSource));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortSource_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortSource_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETVIEWPORTIORECTANGLE:
          {
                STLAYER_Ioctl_SetViewPortIORectangle_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTIORECTANGLE"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortIORectangle_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortIORectangle_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortIORectangle(UserParams.VPHandle,&(UserParams.InputRectangle),&(UserParams.OutputRectangle));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortIORectangle_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortIORectangle_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_ADJUSTIORECTANGLE:
            {
                STLAYER_Ioctl_AdjustIORectangle_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_ADJUSTIORECTANGLE"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_AdjustIORectangle_t *)arg, sizeof(STLAYER_Ioctl_AdjustIORectangle_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_AdjustIORectangle(UserParams.VPHandle,&(UserParams.InputRectangle),&(UserParams.OutputRectangle));

                if((err = copy_to_user((STLAYER_Ioctl_AdjustIORectangle_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_AdjustIORectangle_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }


            break;


        case STLAYER_IOC_GETVIEWPORTIORECTANGLE:
            {
                STLAYER_Ioctl_GetViewPortIORectangle_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTIORECTANGLE"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortIORectangle_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortIORectangle_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortIORectangle(UserParams.VPHandle,&(UserParams.InputRectangle),&(UserParams.OutputRectangle));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortIORectangle_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortIORectangle_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETVIEWPORTPOSITION:
          {
                STLAYER_Ioctl_SetViewPortPosition_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTPOSITION"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortPosition_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortPosition_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortPosition(UserParams.VPHandle,UserParams.XPosition,UserParams.YPosition);

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortPosition_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortPosition_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;


        case STLAYER_IOC_GETVIEWPORTPOSITION:
           {
                STLAYER_Ioctl_GetViewPortPosition_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTPOSITION"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortPosition_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortPosition_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortPosition(UserParams.VPHandle,&(UserParams.XPosition),&(UserParams.YPosition));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortPosition_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortPosition_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }



            break;

        case STLAYER_IOC_SETVIEWPORTPSI:
            {
                STLAYER_Ioctl_SetViewPortPSI_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTPSI"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortPSI_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortPSI_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortPSI(UserParams.VPHandle,&(UserParams.VPPSI));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortPSI_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortPSI_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETVIEWPORTPSI:

         {
                STLAYER_Ioctl_GetViewPortPSI_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTPSI"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortPSI_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortPSI_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortPSI(UserParams.VPHandle,&(UserParams.VPPSI));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortPSI_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortPSI_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETVIEWPORTSPECIALMODE:

         {
                STLAYER_Ioctl_SetViewPortSpecialMode_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTSPECIALMODE"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortSpecialMode_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortSpecialMode_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortSpecialMode(UserParams.VPHandle,UserParams.OuputMode,&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortSpecialMode_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortSpecialMode_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
        }

            break;

        case STLAYER_IOC_GETVIEWPORTSPECIALMODE:
          {
                STLAYER_Ioctl_GetViewPortSpecialMode_t UserParams;
                 STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTSPECIALMODE"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortSpecialMode_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortSpecialMode_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortSpecialMode(UserParams.VPHandle,&(UserParams.OuputMode),&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortSpecialMode_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortSpecialMode_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }


            break;

        case STLAYER_IOC_DISABLECOLORKEY:
          {
                STLAYER_Ioctl_DisableColorKey_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_DISABLECOLORKEY"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_DisableColorKey_t *)arg, sizeof(STLAYER_Ioctl_DisableColorKey_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_DisableColorKey(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_DisableColorKey_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_DisableColorKey_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_ENABLECOLORKEY:
          {
                STLAYER_Ioctl_EnableColorKey_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_ENABLECOLORKEY"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_EnableColorKey_t *)arg, sizeof(STLAYER_Ioctl_EnableColorKey_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_EnableColorKey(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_EnableColorKey_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_EnableColorKey_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;
        case STLAYER_IOC_ENABLEVIEWPORTCOLORFILL:
          {
                STLAYER_Ioctl_EnableViewportColorFill_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_DISABLECOLORFILL"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_EnableViewportColorFill_t *)arg, sizeof(STLAYER_Ioctl_EnableViewportColorFill_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_EnableViewportColorFill(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_EnableViewportColorFill_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_EnableViewportColorFill_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;
        case STLAYER_IOC_DISABLEVIEWPORTCOLORFILL:
          {
                STLAYER_Ioctl_DisableViewportColorFill_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_ENABLECOLORFILL"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_DisableViewportColorFill_t *)arg, sizeof(STLAYER_Ioctl_DisableViewportColorFill_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_DisableViewportColorFill(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_DisableViewportColorFill_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_DisableViewportColorFill_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;



        case STLAYER_IOC_SETVIEWPORTCOLORKEY:
          {
                STLAYER_Ioctl_SetViewPortColorKey_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTCOLORKEY"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortColorKey_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortColorKey_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortColorKey(UserParams.VPHandle,&(UserParams.ColorKey));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortColorKey_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortColorKey_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETVIEWPORTCOLORKEY:
          {
                STLAYER_Ioctl_GetViewPortColorKey_t UserParams;

               STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTCOLORKEY"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortColorKey_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortColorKey_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortColorKey(UserParams.VPHandle,&(UserParams.ColorKey));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortColorKey_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortColorKey_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETVIEWPORTCOLORFILL:
          {
                STLAYER_Ioctl_SetViewportColorFill_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTCOLORFILL"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewportColorFill_t *)arg, sizeof(STLAYER_Ioctl_SetViewportColorFill_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewportColorFill(UserParams.VPHandle,&(UserParams.ColorFill));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewportColorFill_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewportColorFill_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETVIEWPORTCOLORFILL:
          {
                STLAYER_Ioctl_GetViewportColorFill_t UserParams;

               STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTCOLORFILL"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewportColorFill_t *)arg, sizeof(STLAYER_Ioctl_GetViewportColorFill_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewportColorFill(UserParams.VPHandle,&(UserParams.ColorFill));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewportColorFill_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewportColorFill_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;


        case STLAYER_IOC_DISABLEBORDERALPHA:
           {
                STLAYER_Ioctl_DisableBorderAlpha_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_DISABLEBORDERALPHA"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_DisableBorderAlpha_t *)arg, sizeof(STLAYER_Ioctl_DisableBorderAlpha_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_DisableBorderAlpha(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_DisableBorderAlpha_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_DisableBorderAlpha_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

         break;

        case STLAYER_IOC_ENABLEBORDERALPHA:
            {
                STLAYER_Ioctl_EnableBorderAlpha_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_ENABLEBORDERALPHA"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_EnableBorderAlpha_t *)arg, sizeof(STLAYER_Ioctl_EnableBorderAlpha_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_EnableBorderAlpha(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_EnableBorderAlpha_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_EnableBorderAlpha_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETVIEWPORTALPHA:
          {
                STLAYER_Ioctl_SetViewPortAlpha_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTALPHA"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortAlpha_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortAlpha_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortAlpha(UserParams.VPHandle,&(UserParams.Alpha));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortAlpha_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortAlpha_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETVIEWPORTALPHA:
          {
                STLAYER_Ioctl_GetViewPortAlpha_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTALPHA"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortAlpha_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortAlpha_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortAlpha(UserParams.VPHandle,&(UserParams.Alpha));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortAlpha_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortAlpha_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETVIEWPORTGAIN:
          {
                STLAYER_Ioctl_SetViewPortGain_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTGAIN"));
                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortGain_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortGain_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortGain(UserParams.VPHandle,&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortGain_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortGain_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETVIEWPORTGAIN:
         {
                STLAYER_Ioctl_GetViewPortGain_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTGAIN"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortGain_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortGain_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortGain(UserParams.VPHandle,&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortGain_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortGain_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_SETVIEWPORTRECORDABLE:
          {
                STLAYER_Ioctl_SetViewPortRecordable_t UserParams;
                 STTBX_Print((KERN_DEBUG "STLAYER_IOC_SETVIEWPORTRECORDABLE"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_SetViewPortRecordable_t *)arg, sizeof(STLAYER_Ioctl_SetViewPortRecordable_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_SetViewPortRecordable(UserParams.VPHandle,UserParams.Recordable);

                if((err = copy_to_user((STLAYER_Ioctl_SetViewPortRecordable_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_SetViewPortRecordable_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETVIEWPORTRECORDABLE:
         {
                STLAYER_Ioctl_GetViewPortRecordable_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIEWPORTRECORDABLE"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetViewPortRecordable_t *)arg, sizeof(STLAYER_Ioctl_GetViewPortRecordable_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetViewPortRecordable(UserParams.VPHandle,&(UserParams.Recordable));

                if((err = copy_to_user((STLAYER_Ioctl_GetViewPortRecordable_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetViewPortRecordable_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
       }

            break;

        case STLAYER_IOC_GETBITMAPALLOCPARAMS:
            {
                STLAYER_Ioctl_GetBitmapAllocParams_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETBITMAPALLOCPARAMS"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetBitmapAllocParams_t *)arg, sizeof(STLAYER_Ioctl_GetBitmapAllocParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetBitmapAllocParams((UserParams.LayerHandle),&(UserParams.Bitmap),&(UserParams.Params1),&(UserParams.Params1));

                if((err = copy_to_user((STLAYER_Ioctl_GetBitmapAllocParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetBitmapAllocParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETBITMAPHEADERSIZE:
            {
                STLAYER_Ioctl_GetBitmapHeaderSize_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETBITMAPHEADERSIZE"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetBitmapHeaderSize_t *)arg, sizeof(STLAYER_Ioctl_GetBitmapHeaderSize_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetBitmapHeaderSize((UserParams.LayerHandle),&(UserParams.Bitmap),&(UserParams.HeaderSize));

                if((err = copy_to_user((STLAYER_Ioctl_GetBitmapHeaderSize_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetBitmapHeaderSize_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_GETPALETTEALLOCPARAMS:
            {
                STLAYER_Ioctl_GetPaletteAllocParams_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETPALETTEALLOCPARAMS"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetPaletteAllocParams_t *)arg, sizeof(STLAYER_Ioctl_GetPaletteAllocParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetPaletteAllocParams((UserParams.LayerHandle),&(UserParams.Palette),&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_GetPaletteAllocParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetPaletteAllocParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }


          break;



        case STLAYER_IOC_GETVTGNAME:

          {
                STLAYER_Ioctl_GetVTGName_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVTGNAME"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetVTGName_t *)arg, sizeof(STLAYER_Ioctl_GetVTGName_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetVTGName(UserParams.LayerHandle,&(UserParams.VTGName));

                if((err = copy_to_user((STLAYER_Ioctl_GetVTGName_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetVTGName_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;


        case STLAYER_IOC_GETVTGPARAMS :
            {
                STLAYER_Ioctl_GetVTGParams_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVTGPARAMS"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetVTGParams_t *)arg, sizeof(STLAYER_Ioctl_GetVTGParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetVTGParams(UserParams.LayerHandle,&(UserParams.VTGParams));

                if((err = copy_to_user((STLAYER_Ioctl_GetVTGParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetVTGParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_UPDATEFROMMIXER :
           {
                STLAYER_Ioctl_UpdateFromMixer_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_UPDATEFROMMIXER"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_UpdateFromMixer_t *)arg, sizeof(STLAYER_Ioctl_UpdateFromMixer_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_UpdateFromMixer(UserParams.LayerHandle,&(UserParams.OutputParams));

                if((err = copy_to_user((STLAYER_Ioctl_UpdateFromMixer_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_UpdateFromMixer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_DISABLEVIEWPORTFILTER :
            {
                STLAYER_Ioctl_DisableViewPortFilter_t UserParams;
                STTBX_Print((KERN_DEBUG "STLAYER_IOC_DISABLEVIEWPORTFILTER"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_DisableViewPortFilter_t *)arg, sizeof(STLAYER_Ioctl_DisableViewPortFilter_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_DisableViewPortFilter(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_DisableViewPortFilter_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_DisableViewPortFilter_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_ENABLEVIEWPORTFILTER :
          {
                STLAYER_Ioctl_EnableViewPortFilter_t UserParams;
                 STTBX_Print((KERN_DEBUG "STLAYER_IOC_ENABLEVIEWPORTFILTER"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_EnableViewPortFilter_t *)arg, sizeof(STLAYER_Ioctl_EnableViewPortFilter_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_EnableViewPortFilter(UserParams.VPHandle,UserParams.FilterHandle );

                if((err = copy_to_user((STLAYER_Ioctl_EnableViewPortFilter_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_EnableViewPortFilter_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_ATTACHALPHAVIEWPORT :

            {
                STLAYER_Ioctl_AttachAlphaViewPort_t UserParams;
                STTBX_Print(("STLAYER_IOC_ATTACHALPHAVIEWPORT\n"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_AttachAlphaViewPort_t *)arg, sizeof(STLAYER_Ioctl_AttachAlphaViewPort_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_AttachAlphaViewPort(UserParams.VPHandle,UserParams.MaskedLayer);

                if((err = copy_to_user((STLAYER_Ioctl_AttachAlphaViewPort_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_AttachAlphaViewPort_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STLAYER_IOC_ALLOCDATA:
            {
                STLAYER_Ioctl_AllocData_t UserParams;
                STTBX_Print(("STLAYER_IOC_ALLOCDATA\n"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_AllocData_t *)arg, sizeof(STLAYER_Ioctl_AllocData_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_AllocData(UserParams.LayerHandle,&(UserParams.AllocDataParams),&(UserParams.Address_p));

                if((err = copy_to_user((STLAYER_Ioctl_AllocData_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_AllocData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }


            break;


        case STLAYER_IOC_ALLOCDATASECURE:
            {
                STLAYER_Ioctl_AllocData_t UserParams;
                STTBX_Print(("STLAYER_IOC_ALLOCDATASECURE\n"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_AllocData_t *)arg, sizeof(STLAYER_Ioctl_AllocData_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_AllocDataSecure(UserParams.LayerHandle,&(UserParams.AllocDataParams),&(UserParams.Address_p));

                if((err = copy_to_user((STLAYER_Ioctl_AllocData_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_AllocData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }


            break;

        case STLAYER_IOC_FREEDATA:
           {
                STLAYER_Ioctl_FreeData_t UserParams;

                 STTBX_Print(( KERN_DEBUG "STLAYER_IOC_FreeData\n"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_FreeData_t *)arg, sizeof(STLAYER_Ioctl_FreeData_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_FreeData(UserParams.LayerHandle,UserParams.Address_p);

                if((err = copy_to_user((STLAYER_Ioctl_FreeData_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_FreeData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;


        case STLAYER_IOC_INFORMPICTURETOBEDECODED:
          {
                STLAYER_Ioctl_InformPictureToBeDecoded_t UserParams;

               STTBX_Print((KERN_DEBUG "STLAYER_IOC_INFORMPICTURETOBEDECODED"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_InformPictureToBeDecoded_t *)arg, sizeof(STLAYER_Ioctl_InformPictureToBeDecoded_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_InformPictureToBeDecoded(UserParams.VPHandle,&(UserParams.PictureInfos));

                if((err = copy_to_user((STLAYER_Ioctl_InformPictureToBeDecoded_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_InformPictureToBeDecoded_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;


        case STLAYER_IOC_COMMITVIEWPORTPARAMS:
          {
                STLAYER_Ioctl_CommitViewPortParams_t UserParams;

               STTBX_Print((KERN_DEBUG "STLAYER_IOC_COMMITVIEWPORTPARAMS"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_CommitViewPortParams_t *)arg, sizeof(STLAYER_Ioctl_CommitViewPortParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_CommitViewPortParams(UserParams.VPHandle);

                if((err = copy_to_user((STLAYER_Ioctl_CommitViewPortParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_CommitViewPortParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
        case STLAYER_IOC_GETVIDEODISPLAYPARAMS:
          {
                STLAYER_Ioctl_GetVideoDisplayParams_t UserParams;

               STTBX_Print((KERN_DEBUG "STLAYER_IOC_GETVIDEODISPLAYPARAMS"));

                if ((err = copy_from_user(&UserParams, (STLAYER_Ioctl_GetVideoDisplayParams_t *)arg, sizeof(STLAYER_Ioctl_GetVideoDisplayParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STLAYER_GetVideoDisplayParams(UserParams.VPHandle,&(UserParams.Params));

                if((err = copy_to_user((STLAYER_Ioctl_GetVideoDisplayParams_t*)arg, &UserParams, sizeof(STLAYER_Ioctl_GetVideoDisplayParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;
#endif /* STVID_DEBUG_GET_DISPLAY_PARAMS */

        default :
            /*** Inappropriate ioctl for the device ***/
            err = -ENOTTY;
            goto fail;
    }


    return (0);

    /**************************************************************************/

    /*** Clean up on error ***/

fail:
    return (err);
}

