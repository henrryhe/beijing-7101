/*****************************************************************************

File name   : symbol.h

Description : symbol management for development testtool

COPYRIGHT (C) ST-Microelectronics 1998.

Date               Modification              Name     Origin
----               ------------              ----     -------
5-oct-98           ported to 5510            FR       develop

*****************************************************************************/

#ifndef __SYMBOL_H
#define __SYMBOL_H

#ifdef __cplusplus
extern "C" {
#endif

boolean symbol_init                 ( void );
void    symbol_register             ( const char  *symbol,
                                      void        *value );
boolean symbol_enquire_symbol       ( const char  **symbol,
                                      void        *value  );
boolean symbol_enquire_value_inexact( const char  *symbol,
                                      void        **value );
boolean symbol_enquire_value        ( const char  *symbol,
                                      void        **value );
#ifdef __cplusplus
}
#endif

#endif /* ifndef __SYMBOL_H */
