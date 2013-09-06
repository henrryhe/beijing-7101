/******************************************************************************/
/*                                                                            */
/* File name   : SH4OUTBIN.C                                                  */
/*                                                                            */
/* Description : Extract infos from .out file for image flash generation      */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/*                                                                            */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 02/03/05           Created                      M.CHAPPELLIER              */
/*                                                                            */
/******************************************************************************/

/* Include */
/* ------- */
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Global variables */
/* ---------------- */
static unsigned int load_address            = 0; /* Where to load the code */
static unsigned int start_address           = 0; /* User code entry point  */
static unsigned int debug_connected_address = 0; /* Debug address          */ 
static unsigned int image_size              = 0; /* Image size             */ 

/* ========================================================================
   Name:        get_field
   Description: Return a field in a string content
   ======================================================================== */

static char *get_field(char *command,int match_field,char *match_string,int return_field)
{
 char  buffer[256];
 FILE *pipe;
 char *ptr;
 char *fields[16];
 int   i;
 char *return_string=NULL;

 /* Make field numbering zero based */
 /* ------------------------------- */
 match_field--;
 return_field--;

 /* Exec cmd and capture output via a pipe */
 /* -------------------------------------- */
 pipe=popen(command,"r");
 if (pipe==NULL)
  {
   return(NULL);
  }
  
 /* Parse all the lines */
 /* ------------------- */
 while(!feof(pipe))
  {
   if (fgets(buffer,sizeof(buffer),pipe))
    {
     ptr=strtok(buffer," \t\n\r");
     for(i=0;((i<16)&&(ptr!=NULL));i++)
      {
       fields[i]=ptr;
       ptr=strtok(NULL," \t\n\r");
      }
     if((match_field<i)&&(return_field<i))
      {
       if (strcmp(fields[match_field],match_string)==0)
	    {
	     return_string=strdup(fields[return_field]);
    	}
      }
    }
  }

 /* Close the pipe */
 /* -------------- */
 pclose(pipe);

 /* Return string */
 /* ------------- */
 return(return_string);
}

/* ========================================================================
   Name:        main
   Description: main is MAIN. Entry point of the application.
   ======================================================================== */

int main(int argc,char* argv[])
{
 char          dos_command[256];
 char         *ptr;
 struct stat   stat_buf;
 FILE         *file;

 /* Check arguments */
 /* --------------- */
 if (argc!=3) 
  {
   fprintf(stderr,"Usage: %s <main.out> <main.bin>\n",argv[0]);
   fprintf(stderr,"(c) ST MicroElectronics 2005 - Marc Chappellier\n");
   exit(-1);
  }

 /* Convert the .out file in .bin file */
 /* ---------------------------------- */
 sprintf(dos_command,"sh4objcopy -O binary %s m.bin",argv[1]);
 if (system(dos_command)!=0)
  {
   fprintf(stderr,"--> Unable to create the binary file of the application !\n");
   exit(-1);
  }
 if (stat("m.bin",&stat_buf))
  {
   fprintf(stderr,"--> Unable to get the file size of the %s file !\n",argv[2]);
   exit(-1);
  }
 image_size = stat_buf.st_size;

 /* Get load address */
 /* ---------------- */
 sprintf(dos_command,"sh4objdump -h %s",argv[1]);
 ptr = get_field(dos_command,1,"0",4); 
 load_address = strtoul(ptr,NULL,16);
 printf("LoadAddress           = 0x%08x\n",load_address); 
 printf("ImageSize             = 0x%08x\n",image_size); 

 /* Get start address */
 /* ----------------- */
 sprintf(dos_command,"sh4objdump -f %s",argv[1]);
 ptr = get_field(dos_command,1,"start",3); 
 start_address = strtoul(ptr,NULL,16);
 printf("StartAddress          = 0x%08x\n",start_address); 

 /* Get debug connected address */
 /* --------------------------- */
 sprintf(dos_command,"sh4objdump -t %s",argv[1]);
 ptr = get_field(dos_command,6,"__SH_DEBUGGER_CONNECTED",1); 
 debug_connected_address = strtoul(ptr,NULL,16);
 printf("DebugConnectedAddress = 0x%08x\n",debug_connected_address); 

 /* Create header file containing adress/size infos */
 /* ----------------------------------------------- */
 file=fopen("h.bin","wb");
 if (file==NULL)
  {
   fprintf(stderr,"--> Unable to create a tmp file !\n");
   exit(-1);
  }
 fwrite(&load_address           ,1,sizeof(int),file);
 fwrite(&image_size             ,1,sizeof(int),file);
 fwrite(&start_address          ,1,sizeof(int),file);
 fwrite(&debug_connected_address,1,sizeof(int),file);
 fclose(file);

 /* Merge header.bin & main.bin */
 /* --------------------------- */
 sprintf(dos_command,"copy >NUL: 2>NUL: /B h.bin+m.bin %s",argv[2],argv[2]);
 if (system(dos_command)!=0)
  {
   fprintf(stderr,"--> Unable to merge the headers in the main binary file !\n");
   exit(-1);
  }

 /* Remove h.bin */
 /* ------------ */
 sprintf(dos_command,"del >NUL: 2>NUL: h.bin");
 system(dos_command);

 /* Remove m.bin */
 /* ------------ */
 sprintf(dos_command,"del >NUL: 2>NUL: m.bin");
 system(dos_command);

 /* Quit */
 /* ---- */
 exit(0);
}

 
