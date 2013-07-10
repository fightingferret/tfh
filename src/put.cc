#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


void put_in         ( char_data*, thing_array*, obj_data* );
void put_obj_on     ( char_data*, char_data*, obj_data* );
void put_obj_on     ( char_data*, obj_data*,  obj_data* );
void wear_obj       ( char_data*, obj_data*, char_data* );

thing_data* cantwear( thing_data*, char_data*, thing_data* ); // in wear.cc

/*
 *   DO_PUT ROUTINE
 */


void do_put( char_data* ch, char* argument )
{
  char               arg  [ MAX_STRING_LENGTH ];
  thing_array*     array;
  obj_data*    container;
  char_data*      victim;
  obj_data*          obj = NULL ;

  if( contains_word( argument, "on", arg ) ) {
    if( ( victim = one_character( ch, argument, "put", ch->array ) ) != NULL ) {
      if ( (obj = one_object( ch, arg, "put", &ch->contents)) == NULL ) {
        send( ch, "You aren't holding anything like that.\n\r" );
      }
      else {
        put_obj_on( ch, victim, obj );
      }
    }
    return;
  }

  if( !two_argument( argument, "in", arg ) ) {
    send( ch, "Syntax: put <obj> [in] <container>.\n\r" );
    return;
    }

  if( ( container = one_object( ch, argument, "put in", &ch->wearing, 
                                &ch->contents, ch->array ) ) == NULL )
    return;

  if( ( array = several_things( ch, arg, "put", &ch->contents ) ) == NULL )
    return;

  /* PIPES */

  if( container->pIndexData->item_type == ITEM_PIPE ) {
    obj = object((*array)[0]);
    if ( obj == NULL ) {
      send( ch, "%s is not something you wish to smoke.\n\r", array[0] );
    }
    if( obj->selected != 1 || array->size > 1 ) {
      send( ch, "You can only put one item in a pipe at a time.\n\r" );
    }
    else if( container->contents.size > 0 ) {
      send( ch, "%s already contains %s.\n\r", container, 
            container->contents.list[0] );
    }
    else if( obj->pIndexData->item_type != ITEM_TOBACCO ) {
      send( ch, "%s is not something you wish to smoke.\n\r", obj );
    }
    else {
      put_in( ch, array, container );
    }
    return;
  }    

  /* BAGS & KEYRINGS */

  if( container->pIndexData->item_type != ITEM_CONTAINER
    && container->pIndexData->item_type != ITEM_KEYRING ) {
    fsend( ch,
      "You can only put items in containers and keyrings and %s is neither.",
      container );
    return;
    }

  if( is_set( &container->value[1], CONT_CLOSED) ) {
    send( ch, "%s is closed.\n\r", container );
    return;
    }

  // The container must be a singular (number == 1) object.  If it isn't,
  // split off a singular container and put stuff in that.
  if ( container->number > 1 && container->array != NULL ) {
     container->number--;
     container->array->number--;
     obj_data* new_container = duplicate( container );
     new_container->To( container->array );
     put_in( ch, array, new_container );
  }
  else {
     put_in( ch, array, container );
  }
}


/*
 *   BAGS 
 */


thing_data* wont_fit( thing_data* thing, char_data*, thing_data* container )
{
  int  n  = thing->Weight( thing->number );
  int  m  = container->Capacity( );

  // If thing is a TOBACCO object, and container is a PIPE object, 
  // just return thing.  Then put() will do the right thing.
  obj_data* obj_item = object(thing);
  obj_data* obj_container = object(container);
  if ( obj_item != NULL && obj_container != NULL )
     if ( obj_item->pIndexData->item_type == ITEM_TOBACCO &&
          obj_container->pIndexData->item_type == ITEM_PIPE )
        return thing;

  if( m >= n || n <= 0 )
    return thing;
  
  if( ( m = thing->selected*m/n ) <= 0 )
    return NULL;

  content_array* where = thing->array;

  thing = thing->From( m );
  thing->To( where );

  return thing;
}


thing_data* put( thing_data* thing, char_data* ch, thing_data* container )
{
  thing = thing->From( thing->selected );

  if ( container->array != NULL ) {
     char_data* container_holder = character( container->array->where );
     if( ch != container_holder ) {
        set_owner( (obj_data*) thing, NULL, ch );
     }
  }

  thing->To( container );

  return thing;
}


void put_in( char_data* ch, thing_array* array, obj_data* container )
{
  thing_array    subset  [ 4 ];
  thing_func*      func  [ 4 ]  = { same, cursed, wont_fit, put };

  sort_objects( ch, *array, container, 4, subset, func );
  
  msg_type = MSG_INVENTORY;

  page_priv( ch, NULL, empty_string );
  page_priv( ch, &subset[0], "can't fold", NULL, "into itself" );
  page_priv( ch, &subset[1], "can't let go of" );
  page_priv( ch, &subset[2], "doesn't have room for", container );
  page_publ( ch, &subset[3], "put", container, "into",
    container->array == ch->array ? " on the ground" : empty_string );

  consolidate( subset[3] );

  delete array;
}


/*
 *   HORSES
 */


void put_obj_on( char_data* ch, char_data* victim, obj_data* obj )
{
  if( victim->leader != ch || !is_set( &victim->status, STAT_PET ) ) {
    send( ch, "%s isn't a pet of yours.\n\r", victim );
    return;
    }

  // CAPABILITY LOST:  putting multiple objects on a horse at once.

  wear_obj( victim, obj, ch );

  return;
}


void wear_obj( char_data* victim, obj_data* obj, char_data* ch )
{
  thing_data* thing_worn = cantwear( obj, victim, NULL );
  if ( thing_worn == NULL )  // wear slot full or not valid wear slot
  {
     send( ch, "%s can't wear %s.\n\r", victim, obj );
     return;
  }

  thing_worn = needremove( obj, victim, NULL );
  if ( thing_worn == NULL )  // layering problem
  {
     send( ch, "You need to remove something from %s to equip %s.\n\r",
	   victim, obj );
     return;
  }

  // Object is worn.  Figure out where it went and say so.
  if ( can_wear( obj, ITEM_WEAR_HORSE_BACK ) ) {
    send( ch, "You place %s on the back of %s.\n\r", obj, victim );
    send_seen( ch, "%s places %s on the back of %s.\n\r", ch, obj, victim ); 
  }
  else if ( can_wear( obj, ITEM_WEAR_HORSE_BODY ) ) {
    send( ch, "You put %s on %s, protecting %s body.\n\r",
          obj, victim, victim->His_Her( ) );
    send_seen( ch, "%s puts %s on %s, protecting %s body.\n\r",
               ch, obj, victim, victim->His_Her( ) ); 
  }
  else if ( can_wear( obj, ITEM_WEAR_HORSE_FEET ) ) {
    send( ch, "You place %s on the feet of %s.\n\r", obj, victim );
    send_seen( ch, "%s puts %s on the feet of %s.\n\r", ch, obj, victim ); 
  }

  return;
}


/*
 *   TABLES
 */


void put_obj_on( char_data* ch, obj_data* container, list_data* list )
{
  /*
  obj_data*   obj;
  int           i;

  if( container->pIndexData->item_type != ITEM_TABLE ) {
    send( "You can only place items on tables.\n\r", ch );
    return;
    }

  for( i = 0; i < list->length; i++ ) {
    obj = (obj_data*) list->pntr[i];
    if( obj->array != ch )
      continue;
    if( container->where != ch->in_room && container->where != ch )
      break;
    if( !obj->droppable( ) ) {
      send( ch, "You can't let go of %s.\n\r", obj );
      continue;
      }
    act_to( "You place $p on $P.", ch, obj, container, ch );
    act_notchar( "$n places $p on $P.", ch, obj, container );
    obj = remove( obj, obj->selected );
    put_obj( obj, container );
    }
  
  */
  return;
}

