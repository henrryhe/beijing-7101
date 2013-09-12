#ifndef __UDP_IO__
#define __UDP_IO__

BOOL	Wce_UDPIO_Init(void);
void	Wce_UDPIO_Term(void);
int		Wce_UDPIO_Gets(char* str, int size);
int		Wce_UDPIO_Getchar(BOOL blocking	);
int		Wce_UDPIO_Printf(const char * format, ...);


#endif