
#if !defined(MODULE)
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#endif

#include "stos.h"


/* ----------  Constants  ----------------*/

/* ----------  Types & Variables  ----------------*/

/* Mappings */
#if !defined(MODULE)
int      LinuxPageSize = 0;

/* DencVtgDvoVos Shared mapping area */
int      DencVtgDvoVosMappingNumber = 0;
void    *DencVtgDvoVosVirtualAddress = NULL;
void    *AvmemVirtualBaseAddress = NULL;

/* Video mapping shared area */
int      VideoMappingNumber = 0;
void    *VideoVirtualAddress = NULL;
void    *VideoCDVirtualAddress = NULL;

/* Layer mapping address. Needed by the video to access registers... To be cleanned */
int      CompositorMappingNumber = 0;
void    *CompositorVirtualAddress = NULL;
void    *LayerVirtualAddress = NULL;

/* Disp0 Shared mapping area */
int      Disp0MappingNumber = 0;
void    *Disp0VirtualAddress = NULL;

/* Disp1 Shared mapping area */
int      Disp1MappingNumber = 0;
void    *Disp1VirtualAddress = NULL;

/* Blitter Shared mapping area */
int      BlitterMappingNumber = 0;
void    *BlitterVirtualAddress = NULL;
#endif


/* ----------  Functions  ----------------*/

#if !defined(MODULE)
/* *********** File manipulation ***************** */
/*
 * debugfilesize
 */
long int debugfilesize (long int FileDescriptor)
{
	struct stat StatBuf;

	fstat (FileDescriptor, &StatBuf);

	return (StatBuf.st_size);

}
#endif


#if defined( MODULE ) /* Kernel space */
/* Ioctl function */
/************************************************************************************************************************/
/* Function for shared IOCTL case called from the GETREVISION_IOCTL_IMPLEMENTATION macro.                               */
/* This function should never be directly called from the ioctl.                                                        */
/************************************************************************************************************************/
int ST_GetRevIoctlFunction(ST_RevisionStrings_t *arg, const ST_Revision_t Revision,ST_Revision_t(*func)(void))
{
    int err = 0;

    if((err = copy_to_user( arg->CoreRevString, func(), STCOMMON_REV_STRING_LEN ))) { 
        /* Invalid user space address */ 
        goto fail;
    } 
    if((err = copy_to_user( arg->IoctlRevString, Revision, STCOMMON_REV_STRING_LEN ))) {
        /* Invalid user space address */ 
        goto fail; 
    }

 fail:
    return err;
}

EXPORT_SYMBOL(ST_GetRevIoctlFunction);

#else /* User space */
static ST_RevisionStrings_t RevisionStrings;

/************************************************************************************************************************/
/* This is the shared ioctl userspace library function for making the ioctl call to get the revision.                   */
/* The function also checks that the revisions string reported by each component is the same.                           */
/* If they are not the revsions strings used by each components are reported as a warning on stderr.                    */ 
/* The function is called from the GETREVISION_LIB_IMPLEMENTATION macro.                                                */
/* This function should never be directly called from the ioctl.                                                        */
/************************************************************************************************************************/
ST_Revision_t ST_GetDriverRevision(const char *drivername, int request, const char *path, const char *Revision)
{
    int local_fd; /* Don't care whether previously opened or not */
    char *devpath;
        
    devpath = getenv(path);  /* get the path for the device */

    if( devpath ){
        local_fd = open( devpath, O_RDWR );  /* open it */
        if( local_fd == -1 ){
            perror(strerror(errno));
            return (NULL);
        }

    }
    else{
        fprintf(stderr,"The dev path enviroment variable is not defined: %s\n",path);
        return (NULL);
    }

    if( ioctl( local_fd, request, &RevisionStrings ) == -1 ){
        fprintf(stderr,"%s_GetRevision(): Failed\n",drivername);
        perror(strerror(errno));
        close( local_fd );
        return (NULL);
    }
    close( local_fd );

    /* Check that revision strings match */
    if( (strcmp(Revision,RevisionStrings.IoctlRevString)!= 0) ||
        (strcmp(Revision,RevisionStrings.CoreRevString)!= 0) ){
        fprintf(stderr,"WARNING: %s Version strings do not match\n",drivername);
        fprintf(stderr,"Library revision: %s\n",Revision);
        fprintf(stderr,"IOCTL revision  : %s\n",RevisionStrings.IoctlRevString);
        fprintf(stderr,"CORE revision   : %s\n",RevisionStrings.CoreRevString);
    }
    
    return (RevisionStrings.CoreRevString);    
    
}

#endif

/* End of File */
