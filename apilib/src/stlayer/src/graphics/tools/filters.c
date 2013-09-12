/*******************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2002

Source file name :  filters.c

Author :            Michel Bruant :     Creation 

This prg provides the coeff for a x phases - y tap digital filter 
It needs to be compiled in native on your host.
ex type 'gcc filters.c' if you have the gcc compiler.
*******************************************************************************/



#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define U32 unsigned long int
#define S32 signed long int

/* fcts */
static compute_filters(U32 size_in,U32 size_out,U32 number_of_taps,
                        U32 number_subpos,U32 quantization);
static int Calculate_Filter(double B0, int nbtaps, int nbphases,
     int quantization, double* coeff, S32 * int_coeff, int window_type);
static int find_param(char *);
int main (int argc, char *argv[])
{
    int quantization;
    int in;
    int out;
    int taps;
    int pos;
    if (argc != 5)
    {
        printf ("Bad command ! The command is : \n");
	    printf ("a.exe size-in size-out nb-taps nb-subpos \n");
        return 0;
    }
    in = find_param(argv[1]);
    out= find_param(argv[2]);
    taps=find_param(argv[3]);
    pos =find_param(argv[4]);
    printf("\nfilter coefs computed with : \n");
    printf("    size in     = %i\n",in);
    printf("    size out    = %i\n",out);
    printf("    nb taps     = %i\n",taps);
    printf("    nb sub pos  = %i\n",pos);
    quantization = 6;
    compute_filters(in,out,taps,pos,quantization);

	return 1;
}
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
static compute_filters(U32 size_in,U32 size_out,U32 number_of_taps,
                        U32 number_subpos,U32 quantization)
{
    double  B0;
    U32     i,j;
    U32     filterType = 0;
    double *coeff = NULL;
    S32    *filter = NULL;
    
    B0 = (double)size_out/(double)size_in;
    if (B0 > 1.0)
    {
        B0         = 1.0;
        filterType = 1; /* Blackmann (MFB sounds better !) */
    }
    else
    {
        B0 *= 1.2;
        if (B0 > 1.0) B0 = 1.0;
        filterType = 0; /* Rectangular */
    }

    coeff = (double *)malloc(sizeof(double)*(number_of_taps + 1));
    filter = (S32 *)malloc(sizeof(S32)*number_of_taps*number_subpos);

    Calculate_Filter(B0,number_of_taps,number_subpos,quantization,
            coeff,filter, filterType);

    /* print results */
    for (j=0; j<number_subpos ; j++)
    {
        for (i=0; i<number_of_taps;i++)
        {
            printf ("\t%i",(char)(filter[i+j*number_of_taps]));
        }
        printf ("\n");
    }
    printf ("-----------------------------------------\n");
    

    free(filter);
    free(coeff);
}
static int Calculate_Filter(double B0, int nbtaps, int nbphases,
        int quantization, double* coeff, S32 * int_coeff, int window_type)
{
	/* sinx/x filter with a rectangular weighting window
	   The filter is computed in real mode (coeff),
	   and for "nbphases" subpositions in integer mode, according to 
       the quantization parameter.
	   The array int_coeff contains the whole set of "nbphases" subfilters. */

	int i,j,k;
	int range, int_coeff_sum;
	double subposition, coeff_sum;
	double* sub_coeff;
	int error, quantization_error;
	double Blackmann,MaxFauqueBerthier;
	const double PI = 3.1415926535000;

	sub_coeff = (double *)malloc(sizeof(double) * nbtaps);
	quantization_error = 0;

	for ( k=0 ; k<nbphases ; k++ )
	{
				/* fractional offset */
        subposition = (double)k * 1.0/(double)nbphases;

		/* filter coefficients (floating-point) */
		for ( i= -nbtaps/2 ; i <= nbtaps/2 ; i++ )
			if ((subposition == 0.0) && ( i == 0))
			{			/* because undefined 0/0 */
				*(sub_coeff + nbtaps/2) = 1.0;	
			}
			else
			{
			*(sub_coeff + nbtaps/2 + i) = sin(PI*B0*((double)i+subposition)) 
                    / (PI*B0*((double)i+subposition));
			}

		/* Weighting window if required ( 0 = rectangular window) */
		if (window_type == 1)		/* Blackmann Window */
			for ( i= -nbtaps/2 ; i <= nbtaps/2 ; i++ )
			{
                Blackmann = 0.42 + 0.5 *cos((double)PI*((double)i
                         +(double)subposition)/(double)((double)nbtaps/2.0))
                         + (double)0.08*cos((double)2.0*(double)PI*((double)i
                         +subposition)/(double)((double)nbtaps/2.0));
				*(sub_coeff + nbtaps/2 + i) = (*(sub_coeff + nbtaps/2 + i))
                    * Blackmann;
			}
		if (window_type == 2)		/* Max-Fauque-Berthier Window */
			for ( i= -nbtaps/2 ; i <= nbtaps/2 ; i++ )
			{
				if ((subposition == 0.0) && ( i == 0))
				{
					MaxFauqueBerthier = 1.0;	
				}
				else
				{
                    MaxFauqueBerthier = sin(PI*(((double)i+subposition)
                                /(double)((double)nbtaps/2.0))) 
                                /(PI*((double)i+subposition)
                              /(double)((double)nbtaps/2.0));
				}
				*(sub_coeff + nbtaps/2 + i) = (*(sub_coeff + nbtaps/2 + i))
                    * MaxFauqueBerthier;
			}

		/* coefficients sum */
		coeff_sum = 0.0;
		for ( i=0 ; i<nbtaps ; i++ )
		{
			coeff_sum += sub_coeff[i];
		}

		if (subposition == 0.0)					
		{
			for ( i=0 ; i<nbtaps ; i++ )
			{
				*(coeff+i) = *(sub_coeff+i);
			}
			*(coeff+nbtaps) = coeff_sum;	
		}

		/* quantization : n bits */
		range = (int)pow(2.0, (double)quantization);
		for ( i=0 ; i<nbtaps ; i++ )
		{
			*(int_coeff + i + k*nbtaps) 
                = (int)(((sub_coeff[i] / coeff_sum) * (double)range)+0.5);
		}
		int_coeff_sum = 0;
		for ( i=0 ; i<nbtaps ; i++ )
		{
			int_coeff_sum += *(int_coeff + i + k*nbtaps);
		}
		/* correction may be needed to compensate computation errors,
		   so that the sum really match with the range. */
		error = int_coeff_sum - range;
		if (error > 0)			/* example : 257 for 256 */
		{
			if (error & 1)		/* odd error */
			{
				(*(int_coeff + nbtaps/2 + k*nbtaps)) --;
				error--;
			}
			j = 0;
			while ((error != 0) && (j<nbtaps/2))
			{
				if (j != (nbtaps/2 - 1))
				{
					(*(int_coeff + nbtaps/2 + j + k*nbtaps)) --;
					(*(int_coeff + nbtaps/2 - j + k*nbtaps)) --;
					error -= 2;
				}
				else
				{
					*(int_coeff + nbtaps/2 + j + k*nbtaps) -= error/2;
					*(int_coeff + nbtaps/2 - j + k*nbtaps) -= error/2;
					error = 0;
				}
				j++;
			}
		}
		if (error < 0)			/* example : 255 for 256 */
		{
			if (error & 1)		/* odd error */
			{
				(*(int_coeff + nbtaps/2 + k*nbtaps)) ++;
				error++;
			}
			j = 0;
			while ((error != 0) && (j<nbtaps/2))
			{
				if (j != (nbtaps/2 - 1))
				{
					(*(int_coeff + nbtaps/2 + j + k*nbtaps)) ++;
					(*(int_coeff + nbtaps/2 - j + k*nbtaps)) ++;
					error += 2;
				}
				else
				{
					*(int_coeff + nbtaps/2 + j + k*nbtaps) -= error/2;
					*(int_coeff + nbtaps/2 - j + k*nbtaps) -= error/2;
					error = 0;
				}
				j++;
			}
		}
		/* check that error correction efficiency */
		int_coeff_sum = 0;
		for ( i=0 ; i<nbtaps ; i++ )
		{
			int_coeff_sum += *(int_coeff + i + k*nbtaps);
		}
		if (int_coeff_sum != range)
		{
			quantization_error = 1;
		}
	}
	free((double *) sub_coeff);
	return (quantization_error);
}

