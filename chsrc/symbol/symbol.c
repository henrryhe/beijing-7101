/*****************************************************************************

File name   : Symbol.C

Description : symbol registration

COPYRIGHT (C) ST-Microelectronics 1998.


Date          Modification                                    Initials
----          ------------                                    --------
04-oct-99     use stlite.h instead of os20.h                  FQ
10-mar-99     develop.h removed, report.h added               FQ
5-oct-98      Ported to 5510                                  FR
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */
#include <stdlib.h>              /* standard lib for symbol controls        */
#include <string.h>              /* for string manipulation                 */
#include "stlite.h"              /* OS20 for ever..                         */

#define STTBX_INPUT
#define STTBX_OUTPUT
#define STTBX_REPORT
#include "sttbx.h"


/* Local definitions ------------------------------------------------------ */
typedef struct symbol_s symbol_t;

struct symbol_s  {
    symbol_t 		*next;
    void		*value;
    char		 symbol[1];
  };

/* Local prototypes ------------------------------------------------------- */
static symbol_t     *root = NULL;
static semaphore_t*  psymbol_lock;
static symbol_t     *find_value  ( void *value        );
static symbol_t     *find_exact  ( const char *symbol );
static symbol_t     *find_inexact( const char *symbol );

/* ========================================================================
    Initialization function
=========================================================================== */
boolean symbol_init( void )
{
    psymbol_lock=semaphore_create_priority(1 );
    return false;
}
/* ========================================================================
    Registration function
=========================================================================== */
void symbol_register( const char  *symbol,
		      void 	  *value )
{
symbol_t  *symbol_structure;

    symbol_structure = find_exact( symbol );
    if( symbol_structure != NULL )    {
  STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Re-define of symbol `%s'", symbol ));
	symbol_structure->value = value;
    }
    else
    {
	    symbol_structure = malloc( sizeof(symbol_t)+strlen(symbol) );

	    if( symbol_structure == NULL )
        {
          STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Unable to aquire memory in symbol_register" ));
        }

        symbol_structure->next      = root;
        root                        = symbol_structure;
        symbol_structure->value     = value;
        strcpy( symbol_structure->symbol, symbol );
    }
}
/* ========================================================================
    Ask for a particular symbol, giving its value
=========================================================================== */
boolean symbol_enquire_symbol( const char  **symbol,
                               void        *value )
{
symbol_t  *symbol_structure;

    symbol_structure = find_value( value );
    if( symbol_structure == NULL )    {
	*symbol = NULL;
	return true;
    }
    else    {
	*symbol = symbol_structure->symbol;
	return false;
    }
}

/* ========================================================================
    Ask for a particular symbol value, giving part or all of its name
=========================================================================== */
boolean symbol_enquire_value_inexact( const char   *symbol,
		         		void	    **value )
{
symbol_t  *symbol_structure;

    symbol_structure = find_inexact( symbol );
    if( symbol_structure == NULL )    {
     *value = NULL;
     return true;
    }
    else    {
     *value = symbol_structure->value;
     return false;
    }
}

/* ========================================================================
    Ask for a particular symbol value, giving its full name
=========================================================================== */
boolean symbol_enquire_value(  const char  *symbol,
		                 void	    **value )
{
symbol_t  *symbol_structure;

    symbol_structure = find_exact( symbol );
    if( symbol_structure == NULL )    {
     *value = NULL;
     return true;
    }
    else    {
     *value = symbol_structure->value;
     return false;
    }
}

/* ========================================================================
   Find value function
=========================================================================== */
static symbol_t *find_value( void *value )
{
symbol_t   *current;

    semaphore_wait( psymbol_lock );
    for( current = root; current != NULL; current = current->next )
    {
	if( current->value == value ) break;
    }
    semaphore_signal( psymbol_lock );
    return current;
}

/* ========================================================================
   Find exact function
=========================================================================== */
static symbol_t *find_exact( const char *symbol )
{
symbol_t   *current;

    semaphore_wait( psymbol_lock );
    for( current = root; current != NULL; current = current->next )    {
	if( strcmp(current->symbol, symbol) == 0 ) break;
    }
    semaphore_signal( psymbol_lock );
    return current;
}

/* ========================================================================
   Find inexact function
=========================================================================== */
static symbol_t *find_inexact( const char *symbol )
{
symbol_t   *current;

    semaphore_wait( psymbol_lock );
    for( current = root; current != NULL; current = current->next )    {
	if( strncmp(current->symbol, symbol, strlen(symbol)) == 0 ) break;
    }
    semaphore_signal( psymbol_lock );
return current;
}

#ifdef __cplusplus
}
#endif
