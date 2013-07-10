#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include "define.h"
#include "struct.h"


/*
 *   LIST GENERATING FUNCTIONS
 */


int get_trust( char_data *ch )
{
  if( ch->pcdata == NULL )
    return LEVEL_HERO - 1;

  if( ch->pcdata->trust != 0 )
    return ch->pcdata->trust;

  return ch->shdata->level;
}


/*
 *   WEIRD ROUTINES
 */


char_data* rand_player( room_data* room )
{
  player_data* a_player;
  player_array potential_targets;
  int j;

  // Build a set of _all_ players in the room.  Seen, unseen, you name it!
  // Do we want to filter out anyone?  Imms, hiddens, invis?  Imms for now.
  for( j = 0; j < room->contents.size; j++ ) {
     a_player = player( room->contents.list[j] );
     if ( a_player != NULL && a_player->shdata != NULL && 
	  a_player->shdata->level < LEVEL_APPRENTICE ) {
	potential_targets += a_player;
     }
  }

  if( potential_targets.size == 0 )
    return NULL;

  // Pick a player at random from that set.
  j = number_range( 0, potential_targets.size - 1 );
  a_player = potential_targets.list[j];

  return a_player;
}


char_data* rand_victim( char_data* ch )
{
  char_data*  rch;
  char_array potential_targets;
  int j;

  if ( ch->array == NULL )
     return NULL;

  // Count # people that ch sees in the room, or that is fighting ch.
  for( j = 0; j < ch->array->size; j++ ) {
     rch = character( ch->array->list[j] ); 
     if ( rch != NULL && rch != ch &&
          (rch->Seen(ch) || rch->fighting == ch) ) {
	potential_targets += rch;
     }
  }

  if( potential_targets.size == 0 )
    return NULL;

  // Pick a character at random from that set.
  j = number_range( 0, potential_targets.size - 1 );
  rch = potential_targets.list[j];

  return rch;
} 
 
