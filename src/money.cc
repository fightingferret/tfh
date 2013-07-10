#include "ctype.h"
#include "sys/types.h"
#include "stdio.h"
#include "stdlib.h"
#include "define.h"
#include "struct.h"


const int  coin_vnum  [] = { OBJ_COPPER, OBJ_SILVER, OBJ_GOLD, OBJ_PLATINUM }; 
const int   coin_value [] = { 1, 10, 100, 1000 };
const char*  coin_name [] = { "cp", "sp", "gp", "pp" };


/*
 *  VARIOUS MONEY ROUTINES
 */


int monetary_value( obj_data* obj )
{
  int i;

  if( obj->pIndexData->item_type == ITEM_MONEY ) 
    for( i = 0; i < MAX_COIN; i++ )
      if( obj->pIndexData->vnum == coin_vnum[i] ) 
        return obj->selected*coin_value[i];

  return 0;
}


char* coin_phrase( char_data* ch )
{
  int      coins  [ MAX_COIN ];
  obj_data*  obj;

  vzero( coins, MAX_COIN );

  for( int i = 0; i < ch->contents; i++ ) {
    if( ( obj = object( ch->contents[i] ) ) != NULL
      && obj->pIndexData->item_type == ITEM_MONEY ) 
      for( int j = 0; j < MAX_COIN; j++ )
        if( obj->pIndexData->vnum == coin_vnum[j] ) 
          coins[j] += obj->number;
    }

  return coin_phrase( coins );
}


char* coin_phrase( int* num )
{
  static char   buf  [ ONE_LINE ];
  bool         flag  = FALSE;
  int             i;
  int          last;
 
  for( last = 0; last < MAX_COIN; last++ )
    if( num[ last ] != 0 )
      break;

  buf[ 0 ] = '\0';

  for( i = MAX_COIN - 1; i >= last; i-- ) {
    if( num[ i ] == 0 )
      continue;
    sprintf( buf + strlen( buf ), "%s %s%d %s", flag ? "," : "",
      ( i == last && flag ) ? "and " : "", num[i], coin_name[i] );
    flag = TRUE;
    }

  if( !flag ) 
    sprintf( buf, " none" );

  return &buf[0];
}


int get_money( char_data* ch )
{
  obj_data*  obj;
  int        sum  = 0;

  for( int i = 0; i < ch->contents; i++ )
    for( int j = 0; j < 4; j++ )
      if( ( obj = object( ch->contents[i] ) ) != NULL
        && obj->pIndexData->vnum == coin_vnum[j] ) 
        sum += coin_value[j]*obj->number;

  return sum;
}


bool remove_silver( char_data* ch )
{
  obj_data* obj;

  if( ( obj = find_vnum( ch->contents, coin_vnum[1] ) ) != NULL ) {
    obj->Extract( 1 );
    return TRUE;
    }

  return FALSE;
}
 

void add_coins( char_data* ch, int amount, char* message )
{
  obj_data*  obj;
  int        num  [ 4 ];
  int          i;

  for( i = MAX_COIN - 1; i >= 0; i-- ) {
    if( ( num[i] = amount/coin_value[i] ) > 0 ) {
      amount -= num[i]*coin_value[i];
      obj = create( get_obj_index( coin_vnum[i] ), num[i] ); 
      obj->To( ch );
      consolidate( obj );
      }
    }

  if( message != NULL ) 
    send( ch, "%s%s.\n\r", message, coin_phrase( num ) );
}


bool remove_coins( char_data* ch, int amount, char* message ) 
{
  obj_data*       obj;
  obj_data*  coin_obj[MAX_COIN];      // coin objects in ch's inventory
  int           coins[MAX_COIN];      // number coins in ch's inventory
  int          number[MAX_COIN];      // number coins to spend (+) or recv (-)
  int             pos[MAX_COIN];      // coins spent
  int             neg[MAX_COIN];      // coins received as change
  int             dum;
  int               i;                // loop counter over coin types
  bool           flag  = FALSE;       // indicates some change received

  // Sanity check.  Free?  Yeah, ch can afford that.
  if ( amount <= 0 )
     return TRUE;

  vzero( coin_obj, MAX_COIN );
  vzero( coins,    MAX_COIN );
  for( i = 0; i < ch->contents; i++ ) {
    obj = (obj_data*) ch->contents[i];
    for( int j = 0; j < MAX_COIN; j++ ) {
      if( obj->pIndexData->vnum == coin_vnum[j] ) {
        coin_obj[j] = obj; 
        coins[j] = obj->number;
      }
    }
  }
 
  // First pass: try paying with just coppers.
  // Second pass: try paying with silvers and coppers, in that order.
  // Third pass: try paying with golds, silvers and coppers.
  // Fourth pass: try paying with all possible coins.
  int j;
  int save_amount = amount;
  for ( j = 0; j < MAX_COIN && amount > 0; j++ ) {
     amount = save_amount;
     vzero( number, MAX_COIN );   
     for( i = j; i >= 0 && amount > 0; i-- ) {
        if ( amount <= coins[i]*coin_value[i] ) { // coins of type i suffice
            number[i] = amount/coin_value[i];
            if ( amount % coin_value[i] != 0 ) {   // need to overshoot
	       number[i]++;
            }
            amount -= number[i]*coin_value[i]; // may go negative if need change
        }
        else { // coins of type i not enough, go down to cheaper coin type
           amount -= coins[i]*coin_value[i];
           number[i] = coins[i];
        }
     }
  }

  if( amount > 0 )  // player didn't have enough money
    return FALSE;

  amount = -amount;         // amount is now amount of change to give
   
  // i currently set to highest coin type that can be given out as change
  for( ; i >= 0; i-- ) {
    dum = amount/coin_value[i];
    amount -= dum*coin_value[i];
    number[i] -= dum;      // negative number[] indicates change to be given
  }
    
  // Done populating number array.  Extract spent coinage, create change
  // coinage.
  vzero( pos, MAX_COIN );   
  vzero( neg, MAX_COIN );   
  for( i = MAX_COIN - 1; i >= 0; i-- ) {
    if( number[i] > 0 ) {
      coin_obj[i]->Extract( number[i] );
      pos[i] = number[i];
    }
    else if ( number[i] < 0 ) {
      neg[i] = -number[i];
      if( coin_obj[i] == NULL ) {
         obj = create( get_obj_index( coin_vnum[i] ), neg[i] );
         obj->To( ch );
         consolidate( obj );
      }
      else
         coin_obj[i]->number += neg[i];
      flag = TRUE;
    }
  }

  if( message != NULL ) {
    fsend( ch, "%s%s.\n\r", message, coin_phrase( pos ) );
    if( flag ) 
      send( ch, "You receive%s in change.\n\r", coin_phrase( neg ) );
    }

  return TRUE;
}


void do_split( char_data* ch, char* argument )
{
  int amount;

  if( *argument == '\0' ) {
    send( ch, "What amount do you wish to split?\n\r" );
    return;
    }

  amount = atoi( argument );

  if( amount < 2 ) {
    send( ch, "It is difficult to split anything less than 2 cp.\n\r" );
    return;
    }
 
  if( get_money( ch ) < amount ) {
    send( ch, "You don't have enough coins to split that amount.\n\r" );
    return;
    }

  split_money( ch, amount, TRUE );
}


void split_money( char_data* ch, int amount, bool msg )
{

  if( amount == 0 )
    return;

  player_data* a_player;
  char_data* group  = NULL;
  int members = 1;
  for ( int j = 0; j < ch->in_room->contents.size; j++ ) {
     thing_data* a_thing = ch->in_room->contents[j];
     a_player = player( a_thing );
     if ( a_player != NULL ) {
        if ( a_player != ch && is_same_group(a_player, ch) ) {
           members++;
           a_player->next_list = group;
           group = a_player;
        }
     }
  }

  if( members < 2 ) {
     if( msg )
        send( ch, "There is noone here to split the coins with.\n\r" );
     return;
  }
  
  obj_data*  coin_obj[4];
  int        coins_held[4];
  for (int i1 = 0; i1 < 4; i1++ ) {
     coin_obj[i1]   = NULL;
     coins_held[i1] = 0;
  }

  obj_data* obj = NULL;
  for ( int k = 0; k < ch->contents.size; k++ ) {
     obj = object( ch->contents[k] );  
     if ( obj != NULL ) {
        for ( int i2 = 0; i2 < 4; i2++ ) {
	   if ( obj->pIndexData->vnum == coin_vnum[i2] ) {
              coin_obj[i2] = obj; 
              coins_held[i2] = obj->number;
           }
        }
     }
  }

  /*
  ** New splitting algorithm:
  **   Say there are 3 other members in your group, and you type 'split 400'
  **     Try to split plats 3 ways 
  **     Try to split golds 3 ways
  **     Try to split silvers 3 ways
  **     Try to split coppers 3 ways
  **   If people didn't get enough money, then:
  **     Try to split plats 2 ways 
  **     Try to split golds 2 ways
  **     Try to split silvers 2 ways
  **     Try to split coppers 2 ways
  **   Repeat.
  **   
  */
  bool anything = FALSE;
  int split = amount / members;
  int coins_split[4] = {0, 0, 0, 0};
  for ( int n_ways = members - 1; n_ways > 0 && split > 0; n_ways-- ) {
     bool found = FALSE;
     for ( int i = MAX_COIN-1; i >= 0 && split > 0; i-- ) {
        // Try to split this value of coins N ways
        coins_split[i] = min( coins_held[i]/n_ways, split/coin_value[i] );
	if ( coins_split[i] > 0 ) {
	   // Loop through the player list, give 'em all coins_split[i] coins.
	   char_data* gch;
	   int j;
	   for ( j = 0, gch = group ; j < n_ways && gch != NULL ;
                 j++, gch = gch->next_list ) {
	      coins_held[i] -= coins_split[i];
              thing_data* thing_taken = coin_obj[i]->From( coins_split[i] );
              if ( (obj = object( thing_taken )) != NULL ) {
                 obj->To( gch );
	         found = TRUE;
              }
           }
           split -= coins_split[i] * coin_value[i];
        }
     }
     if ( found ) { 
         char* phrase = coin_phrase( coins_split );
	 char_data* gch2;
	 int j2;
	 for ( j2 = 0, gch2 = group ; j2 < n_ways && gch2 != NULL ;
               j2++, gch2 = gch2->next_list ) {
            send( ch, "You give%s to %s.\n\r", phrase, gch2 );
            send( gch2, "%s gives%s to you.\n\r", ch, phrase );
            send( *ch->array, "%s gives%s to %s.\n\r", ch, phrase, gch2 );
         }
         anything = TRUE;
     }
  }

  if( !anything )
    send( ch, "You lack the correct coins to split that amount.\n\r" );

  return;
}




