/*******************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2003

Source file name :  

Author :            Michel Bruant :     Creation 

*******************************************************************************/
#include <stdio.h>

static int find_param(char * string)
{
    int i;
    int fact;
    int res;
    res = 0;
    fact = strlen(string);
    for (i=0; i<fact; i++)
    {
        res = res * 10 + (string[i] -48);
    }
    return res;
}

int main (int argc, char *argv[])
{
    char            luma_file[80];
    char            chroma_file[80];
    char            output_file[80];
    FILE            *luma_handle;
    FILE            *chroma_handle;
    FILE            *output_handle;
    fpos_t          pos;
    int             Width,Height,Sluma,Schroma,byte;
    struct
    {
        short Header_size;
        short Signature;
        short Type;
        short Properties;
        long  width;
        long  height;
        long  Sluma;   
        long  Schroma; 
    }Gamma_Header;

    if (argc != 6)
    {

        printf ("Bad command ! The command is : \n");
	    printf ("a.exe  luma.bin chroma.bin  w h dump.gam\n");
        return 0;
    }
    strcpy(luma_file,   argv[1]);
    strcpy(chroma_file, argv[2]);
    Width  = find_param(argv[3]);
    Height = find_param(argv[4]);
    strcpy(output_file, argv[5]);

    luma_handle   = fopen(luma_file, "rb");
    if(luma_handle == 0)
    {
        printf("luma file doesn't exist \r");
        return;
    }
    chroma_handle = fopen(chroma_file, "rb");
    if(chroma_handle == 0)
    {
        printf("chroma file doesn't exist \r");
        return;
    }
    output_handle = fopen(output_file, "wb");

    fwrite((void *)&Gamma_Header,sizeof(Gamma_Header),1, output_handle);

    Sluma = 0;
    byte = fgetc(luma_handle);
    while(byte != EOF)
    {
        fputc(byte,output_handle);
        Sluma++;
        byte = fgetc(luma_handle);
    }

    Schroma = 0;
    byte = fgetc(chroma_handle);
    while(byte != EOF)
    {
        fputc(byte,output_handle);
        Schroma++;
        byte = fgetc(chroma_handle);
    }
    
    Gamma_Header.Header_size = 0x6;
    Gamma_Header.Signature   = 0x420F;
    Gamma_Header.Properties  = 0x11;
    Gamma_Header.Type        = 0x94;
    Gamma_Header.width       = Width;
    Gamma_Header.height      = Height;
    Gamma_Header.Sluma       = Sluma;
    Gamma_Header.Schroma     = Schroma;
    pos = 0;
    fsetpos(output_handle,&pos);
    fwrite((void *)&Gamma_Header,sizeof(Gamma_Header),1, output_handle);
}
