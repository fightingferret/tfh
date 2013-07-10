#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
#include "struct.h"


#define EXTRACT_BLOOD     0
#define MAX_EXTRACTION    1


const char* extract_name [] = { "blood" };


void do_dig( char_data* ch, char* )
{
  send( "-- Command under construction. --\n\r", ch );
  return;
}


void do_bury( char_data* ch, char* argument )
{
  obj_data* obj;

  if( *argument == '\0' ) {
    send( ch, "What do you wish to bury?\n\r" );
    return;
    }

  if( ( obj = one_object( ch, argument, "bury", &ch->contents ) ) == NULL ) 
    return;

  send( ch, "Command under construction.\n\r" );
  return;
}


void do_extract( char_data* ch, char* argument )
{
  char             arg  [ MAX_INPUT_LENGTH ];
  int                i;
  obj_data*        obj;
  obj_data*  container;

  argument = one_argument( argument, arg );

  for( i = 0; i < MAX_EXTRACTION; i++ )
    if( !strcasecmp( arg, extract_name[i] ) ) 
      break;

  if( i == MAX_EXTRACTION ) {
    send( "That isn't something you can extract from anything.\n\r", ch );
    return;
    }

  if( *argument == '\0' ) {
    send( ch, "From what do you wish to extract %s from?\n\r",
      extract_name[i] );
    return;
    }

  if( ( obj = one_object( ch, argument, "extract", ch->array ) ) == NULL ) {
    send( ch, "The object you wish to extract %s from is not here.\n\r",
      extract_name[i] );
    return;
    }

  if( i == EXTRACT_BLOOD ) {
      if( obj->pIndexData->item_type != ITEM_CORPSE ) {
         send( "You can only extract blood from corpses.\n\r", ch );
         return;
      }
      if ( ( (container = ch->Wearing(WEAR_HELD_R)) != NULL &&
	     container->pIndexData->item_type == ITEM_DRINK_CON ) ||
           ( (container = ch->Wearing(WEAR_HELD_L)) != NULL &&
             container->pIndexData->item_type == ITEM_DRINK_CON ) ) {
         /* do extraction here */
         fsend( ch,
              "You extract into %s as much blood as possible from %s.\n\r",
                container, obj );
      }
      else {
         send( "You need to be holding a liquid container to extract blood.\n\r", ch );
         return;
      }
    }

  return;
}
