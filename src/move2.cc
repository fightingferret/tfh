#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"


// ASSUMPTIONS:  Room light is only affected when characters remove worn lights.
//               Room light is _not_ affected when characters pick up lights.
void remove_weight( thing_data* thing, int i )
{
  content_array*  where;
  player_data*       pc; 
  int            w1, w2;
  int            l1, l2;
  bool do_picking_up = false;

  if( ( where = thing->array ) == NULL ) 
    return;

  // Special case: if we're removing an object from a room directly 
  // (e.g., pick up), then this does _not_ modify the room's light.
  room_data* a_room = Room(where->where);
  if ( (a_room != NULL) && (where == &a_room->contents)) {
     obj_data* an_object = object(thing);
     if (an_object != NULL)
        do_picking_up = true;
  }

  w1  = thing->Weight( i );
  l1  = thing->Light( i );
 
  where->number -= thing->Number( i );

  for( ; ; where = thing->array ) { 
    thing = where->where;   // picking up case: thing is a room 
    if( thing == NULL || 
        thing->array == NULL || 
        ( ( pc = player( thing ) ) != NULL && ( where == &pc->locker || 
					        where == &pc->junked ) ) ) {
      where->weight -= w1; 
      if ( !do_picking_up ) {
         where->light  -= l1; 
         if (where->light < 0) where->light = 0;
      }
      break;
    }

    w2 = thing->Weight( 1 ); 
    l2 = thing->Light( 1 );  
    where->weight -= w1;   
    char_data* ch = character(thing);
    if ( (ch != NULL) && (where == &ch->wearing) ) {
       where->light -= l1;  
       if (where->light < 0) where->light = 0;
    }
    else {
       l1 = 0;
    }

    // Special case 2: if we're removing an object from a bag of holding, then
    // the object's weight is halved, from the p.o.v. of all enclosing things
    // _except_ the bag of holding.
    obj_data* maybe_bag = object(thing);
    if ( maybe_bag != NULL && 
         maybe_bag->pIndexData->item_type == ITEM_CONTAINER
         && is_set( &maybe_bag->value[1], CONT_HOLDING ) ) {
       w1 /= 2;
    }
  }
}


// ASSUMPTIONS:  Room light is only affected when characters wear lights.
//               Room light is _not_ affected when characters drop lights.
void add_weight( thing_data* thing, int i )
{
  content_array*   where;
  player_data*        pc;
  int             w1, w2;
  int             l1, l2;
  bool do_dropping = false;

  if( ( where = thing->array ) == NULL )
    return;

  // Special case: if we're adding an object to a room directly (e.g., drop),
  // then this does _not_ modify the room's light.
  room_data* a_room = Room(where->where);
  if ( (a_room != NULL) && (where == &a_room->contents)) {
     obj_data* an_object = object(thing);
     if (an_object != NULL)
        do_dropping = true;
  }

  w1  = thing->Weight( i );
  l1  = thing->Light( i );
 
  where->number += thing->Number( i );

  for( ; ; where = thing->array ) {
    thing = where->where;  // 1x - thing is the room
    if( thing == NULL || 
        thing->array == NULL || 
        ( ( pc = player( thing ) ) != NULL && ( where == &pc->locker || 
						where == &pc->junked ) ) ) {
      where->weight += w1; 
      if ( !do_dropping ) where->light += l1; 
      break;
    }

    w2 = thing->Weight(1);
    l2 = thing->Light(1); 
    where->weight += w1;  
    char_data* ch = character(thing);
    if ( (ch != NULL) && (where == &ch->wearing) ) {
       where->light += l1;  
    }
    else {
       l1 = 0;
    }

    // Special case 2: if we're adding an object to a bag of holding, then the
    // object's weight is halved, from the p.o.v. of all enclosing things
    // _except_ the bag of holding.
    obj_data* maybe_bag = object(thing);
    if ( maybe_bag != NULL && 
         maybe_bag->pIndexData->item_type == ITEM_CONTAINER
         && is_set( &maybe_bag->value[1], CONT_HOLDING ) ) {
       w1 /= 2;
    }
  }
}
  

/*
 *   TRANSFER FROM
 */


thing_data* Thing_Data :: From( int )
{
  remove_weight( this, number );

  if ( array != NULL ) {
     *array -= this;
  }
  array = NULL;

  return this;
}


thing_data* Char_Data :: From( int )
{
  room_data* room;

  in_room = NULL;

  if( ( room = Room( array->where ) ) != NULL ) {
    if( pcdata != NULL )
      room->area->nplayer--;

    stop_fight( this );
    stop_events( this, execute_drown );
    }

  Thing_Data :: From( );

  return NULL;
}


thing_data* Obj_Data :: From( int i )
{
  char_data*         ch;
  obj_data*         obj;
  content_array*  where  = array;
  
  if( number > i ) {
    remove_weight( this, i );
    number        -= i;
    obj            = duplicate( this );   
    obj->number    = i;
    obj->selected  = i;
    return obj;
    }

  Thing_Data :: From( );

  if( ( ch = character( where->where ) ) != NULL
    && where == &ch->wearing )
    unequip( ch, this );

  for( int i = 0; i < *where; i++ ) 
    if( ( ch = character( where->list[i] ) ) != NULL 
      && ch->cast != NULL
      && ch->cast->target == this )
      disrupt_spell( ch ); 

  stop_events( this, execute_decay );

  return this;
}


/*
 *   TRANSFER TO
 */

  
void Thing_Data :: To( thing_data* thing )
{
  To( &thing->contents );
}


void Thing_Data :: To( content_array* where )
{
  if( array == NULL ) {
    *where += this;
    array   = where;
    }

  add_weight( this, number );
}


void Char_Data :: To( thing_data* thing )
{
  To( &thing->contents );
}


void Char_Data :: To( content_array* where )
{
  room_data*   room;
  wizard_data*  imm;
  char_data*    rch;

  if( array != NULL ) {
    roach( "Adding character somewhere which isn't nowhere." );
    roach( "-- Ch = %s", this );
    From( number );
    }

  Thing_Data :: To( where );
  // The character's 'array' is now pointing to current room's 'contents'.

  /* CHARACTER TO ROOM */

  if( ( room = Room( where->where ) ) != NULL ) {
    room_position = -1;
    in_room       = room;

    if( pcdata != NULL ) 
      room->area->nplayer++;

    if( ( imm = wizard( this ) ) != NULL ) {
      imm->custom_edit  = 0;
      imm->room_edit    = NULL;
      imm->action_edit  = NULL;
      imm->exit_edit    = NULL;
      }

    if( water_logged( room ) ) {
      remove_bit( &status, STAT_SNEAKING );
      remove_bit( &status, STAT_HIDING );
      remove_bit( &status, STAT_CAMOUFLAGED );
      remove_bit( affected_by, AFF_HIDE );
      remove_bit( affected_by, AFF_CAMOUFLAGE );
      }

    if( is_submerged( this ) )
      enter_water( this );

    for( int i = 0; i < room->contents; i++ ) {
      if( ( rch = character( room->contents[i] ) ) == NULL 
        || rch == this )
        continue;
 
      if( rch->species != NULL && species != NULL
        && rch->species->group == species->group
        && rch->species->group != GROUP_NONE ) {
        share_enemies( this, rch );   
        share_enemies( rch, this );   
      }

      // When walking into a room, build up array of potential targets.
      if( is_aggressive( this, rch ) ) {
	aggressive += rch;
      }

      // Room denizens, if aggressive, all leap to attack
      if( is_aggressive( rch, this ) ) 
        init_attack( rch, this );
    }

    // Pick a random guy to attack
    if ( aggressive.size > 0 ) {
       rch = aggressive[ number_range(0, aggressive.size-1) ];
       init_attack( this, rch );
    }
    
    return; 
  }

  roach( "Attempted transfer of character to non-room object." );
}


void Obj_Data :: To( thing_data* thing )
{
  To( &thing->contents );
}


void Obj_Data :: To( content_array* where )
{
  event_data* event;
  room_data*   room;
  char_data*     ch;
  obj_data*     obj;
  int             i;

  if( array != NULL ) {
    roach( "Adding object somewhere which isn't nowhere." );
    roach( "-- Obj = %s", this );
    From( number );
    }

  if( ( room = Room( where->where ) ) != NULL ) {
    if( pIndexData->item_type == ITEM_CORPSE
      && value[0] > 0 ) {
      event = new event_data( execute_decay, this );
      add_queue( event, value[0] ); 
      }
    }

  if( ( ch = character( where->where ) ) != NULL
    && where == &ch->wearing ) {
    equip( ch, this );
    for( i = 0; i < ch->wearing; i++ ) {
      obj = (obj_data*) ch->wearing[i];
      if( obj->position > position 
        || ( obj->position == position
        && obj->layer > layer ) ) 
        break;
      }
    insert( *where, this, i );
    array = where; // equipped object's 'array' points to character's 'wearing'
    }  

  Thing_Data :: To( where );
}


/*
 *  DECAY
 */


void execute_decay( event_data* event )
{
  obj_data* obj = (obj_data*) event->owner;

  obj->shown = 1;
  fsend( *obj->array, "%s rots, and is quickly eaten by a grue.", obj );

  obj->Extract( );
}








