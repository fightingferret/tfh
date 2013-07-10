#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


void spoil_hide( char_data* ch )
{
  char_data* rch;
 
  if( !is_set( &ch->status, STAT_HIDING ) && 
      !is_set( ch->affected_by, AFF_HIDE ) )
    return;

  remove_bit( &ch->status, STAT_HIDING );
  remove_bit( ch->affected_by, AFF_HIDE );

  // What do room denizens do?
  if ( ch->array != 0 ) {
     for( int i = ch->array->size - 1; i >= 0; i-- ) {
        rch = character( ch->array->list[i] );
        if( rch != NULL && ch != rch && ch->Seen( rch ) ) {

	   // They notice that ch is there.
           if (  !includes( ch->seen_by, rch ) ) {
              send( rch, "++ You notice %s hiding in the shadows! ++\n\r", ch );
              ch->seen_by += rch;
           }

           // If aggressive, they leap to attack.
           if ( is_aggressive( rch, ch ) ) {
	      init_attack( rch, ch );
           }
        }
     }
  }
}


